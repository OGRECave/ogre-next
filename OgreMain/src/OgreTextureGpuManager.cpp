/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "OgreStableHeaders.h"

#include "OgreTextureGpuManager.h"

#include "OgreAsyncTextureTicket.h"
#include "OgreBitset.inl"
#include "OgreBitwise.h"
#include "OgreCommon.h"
#include "OgreException.h"
#include "OgreHlmsDatablock.h"
#include "OgreId.h"
#include "OgreImage2.h"
#include "OgreLogManager.h"
#include "OgreLwString.h"
#include "OgreObjCmdBuffer.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreProfiler.h"
#include "OgreRenderSystem.h"
#include "OgreResourceGroupManager.h"
#include "OgreStagingTexture.h"
#include "OgreString.h"
#include "OgreTextureFilters.h"
#include "OgreTextureGpu.h"
#include "OgreTextureGpuManagerListener.h"
#ifdef OGRE_PROFILING_TEXTURES
#    include "OgreTimer.h"
#endif
#include "Threading/OgreThreads.h"
#include "Vao/OgreVaoManager.h"

#include <fstream>

#if !OGRE_NO_JSON
#    include "OgreStringConverter.h"
#
#    if defined( __GNUC__ ) && !defined( __clang__ )
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wclass-memaccess"
#    endif
#    if defined( __clang__ )
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#        pragma clang diagnostic ignored "-Wdeprecated-copy"
#    endif
#    include "rapidjson/document.h"
#    include "rapidjson/error/en.h"
#    if defined( __clang__ )
#        pragma clang diagnostic pop
#    endif
#    if defined( __GNUC__ ) && !defined( __clang__ )
#        pragma GCC diagnostic pop
#    endif
#endif

//#define OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD 1
//#define OGRE_DEBUG_MEMORY_CONSUMPTION 1

#define TODO_grow_pool

namespace Ogre
{
    static const int c_mainThread = 0;
    static const int c_workerThread = 1;

    static DefaultTextureGpuManagerListener sDefaultTextureGpuManagerListener;

    unsigned long updateStreamingWorkerThread( ThreadHandle *threadHandle );
    THREAD_DECLARE( updateStreamingWorkerThread );
    unsigned long updateTextureMultiLoadWorkerThread( ThreadHandle *threadHandle );
    THREAD_DECLARE( updateTextureMultiLoadWorkerThread );

    TextureGpuManager::TextureGpuManager( VaoManager *vaoManager, RenderSystem *renderSystem ) :
        mDefaultMipmapGen( DefaultMipmapGen::HwMode ),
        mDefaultMipmapGenCubemaps( DefaultMipmapGen::SwMode ),
        mShuttingDown( false ),
        mUseMultiload( false ),
        mTryLockMutexFailureCount( 0u ),
        mTryLockMutexFailureLimit( 1200u ),
        mLoadRequestsCounter( 0u ),
        mLastUpdateIsStreamingDone( true ),
        mAddedNewLoadRequests( false ),
        mMultiLoadsSemaphore( 0u ),
        mPendingMultiLoads( 0u ),
        mEntriesToProcessPerIteration( 3u ),
        mMaxPreloadBytes( 256u * 1024u * 1024u ),  // A value of 512MB begins to shake driver bugs.
        mTextureGpuManagerListener( &sDefaultTextureGpuManagerListener ),
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS && OGRE_PLATFORM != OGRE_PLATFORM_ANDROID && \
    OGRE_ARCH_TYPE != OGRE_ARCHITECTURE_32
        mStagingTextureMaxBudgetBytes( 256u * 1024u * 1024u ),
#else
        mStagingTextureMaxBudgetBytes( 128u * 1024u * 1024u ),
#endif
        mDelayListenerCalls( false ),
        mIgnoreScheduledTasks( false ),
#ifdef OGRE_PROFILING_TEXTURES
        mProfilingLoadingTime( true ),
#endif
        mIgnoreSRgbPreference( false ),
        mVaoManager( vaoManager ),
        mRenderSystem( renderSystem )
    {
        memset( mErrorFallbackTexData, 0, sizeof( mErrorFallbackTexData ) );

        PixelFormatGpu format;
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS && OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
#    if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_32
        // 32-bit have tighter limited addresse memory. They pay the price
        // in slower streaming (more round trips between main and worker threads)
        mStreamingData.maxSplitResolution = 2048u;
#    else
        // 64-bit have plenty of virtual addresss to spare. We can reserve much more.
        mStreamingData.maxSplitResolution = 4096u;
#    endif
        const uint32 maxResolution = mStreamingData.maxSplitResolution;
        // 32MB / 128MB for RGBA8, that's two 4096x4096 / 2048x2048 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_RGBA8_UNORM );
        mBudget.push_back( BudgetEntry( format, maxResolution, 2u ) );
        // 4MB / 16MB for BC1, that's two 4096x4096 / 2048x2048 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_BC1_UNORM );
        mBudget.push_back( BudgetEntry( format, maxResolution, 2u ) );
        // 4MB / 16MB for BC3, that's one 4096x4096 / 2048x2048 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_BC3_UNORM );
        mBudget.push_back( BudgetEntry( format, maxResolution, 1u ) );
        // 4MB / 16MB for BC5, that's one 4096x4096 / 2048x2048 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_BC5_UNORM );
        mBudget.push_back( BudgetEntry( format, maxResolution, 1u ) );
#else
        mStreamingData.maxSplitResolution = 2048u;
        // Mobile platforms don't support compressed formats, and have tight memory constraints
        // 8MB for RGBA8, that's two 2048x2048 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_RGBA8_UNORM );
        mBudget.push_back( BudgetEntry( format, 2048u, 2u ) );
#endif

        // Sort in descending order.
        std::sort( mBudget.begin(), mBudget.end(), BudgetEntry() );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS || OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
        // Mobile platforms are tight on memory. Keep the limits low.
        mMaxPreloadBytes = 32u * 1024u * 1024u;
#else
#    if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_32
        // 32-bit architectures are more limited.
        // The default 256MB can cause Out of Memory conditions due to memory fragmentation.
        mMaxPreloadBytes = 128u * 1024u * 1024u;
#    endif
#endif

        // Starts as true so fullfillBudget can run at least once
        mStreamingData.workerThreadRan = true;
        mStreamingData.bytesPreloaded = 0;
        mStreamingData.maxPerStagingTextureRequestBytes = 64u * 1024u * 1024u;

        for( int i = 0; i < 2; ++i )
            mThreadData[i].objCmdBuffer = new ObjCmdBuffer();

#if OGRE_PLATFORM != OGRE_PLATFORM_EMSCRIPTEN && !OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD
        mWorkerThread = Threads::CreateThread( THREAD_GET( updateStreamingWorkerThread ), 0, this );
#endif
    }
    //-----------------------------------------------------------------------------------
    TextureGpuManager::~TextureGpuManager()
    {
        shutdown();

        assert( mAvailableStagingTextures.empty() && "Derived class didn't call destroyAll!" );
        assert( mUsedStagingTextures.empty() && "Derived class didn't call destroyAll!" );
        assert( mEntries.empty() && "Derived class didn't call destroyAll!" );
        assert( mTexturePool.empty() && "Derived class didn't call destroyAll!" );

        for( int i = 0; i < 2; ++i )
        {
            delete mThreadData[i].objCmdBuffer;
            mThreadData[i].objCmdBuffer = 0;
        }

        mTextureGpuManagerListener = 0;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::shutdown()
    {
        setMultiLoadPool( 0u );

        if( !mShuttingDown )
        {
            mShuttingDown = true;
#if OGRE_PLATFORM != OGRE_PLATFORM_EMSCRIPTEN && !OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD
            mWorkerWaitableEvent.wake();
            Threads::WaitForThreads( 1u, &mWorkerThread );
#endif
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAll()
    {
        mMutex.lock();
        abortAllRequests();
        destroyAllStagingBuffers();
        destroyAllAsyncTextureTicket();
        destroyAllTextures();
        destroyAllPools();
        mMutex.unlock();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::abortAllRequests()
    {
        ThreadData &workerData = mThreadData[c_workerThread];
        ThreadData &mainData = mThreadData[c_mainThread];
        mLoadRequestsMutex.lock();
        mainData.loadRequests
            .clear();  // TODO: if( loadRequest.autoDeleteImage ) delete loadRequest.image;
        mainData.objCmdBuffer->clear();
        mainData.usedStagingTex.clear();
        workerData.loadRequests
            .clear();  // TODO: if( loadRequest.autoDeleteImage ) delete loadRequest.image;
        workerData.objCmdBuffer->clear();
        workerData.usedStagingTex.clear();
        mLoadRequestsMutex.unlock();

        while( !mStreamingData.queuedImages.empty() )
        {
            TextureFilter::FilterBase::destroyFilters( mStreamingData.queuedImages.back().filters );
            mStreamingData.queuedImages.pop_back();
        }

        {
            // These partial images were supposed to transfer ownership of sysRamPtr to TextureGpu
            // But we now must free these ptrs ourselves
            PartialImageMap::const_iterator itor = mStreamingData.partialImages.begin();
            PartialImageMap::const_iterator endt = mStreamingData.partialImages.end();

            while( itor != endt )
            {
                if( itor->second.sysRamPtr )
                    OGRE_FREE_SIMD( itor->second.sysRamPtr, MEMCATEGORY_RESOURCE );
                ++itor;
            }
            mStreamingData.partialImages.clear();
        }

        mScheduledTasks.clear();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllStagingBuffers()
    {
        StagingTextureVec::iterator itor = mStreamingData.availableStagingTex.begin();
        StagingTextureVec::iterator endt = mStreamingData.availableStagingTex.end();

        while( itor != endt )
        {
            ( *itor )->stopMapRegion();
            ++itor;
        }

        mStreamingData.availableStagingTex.clear();

        itor = mAvailableStagingTextures.begin();
        endt = mAvailableStagingTextures.end();

        while( itor != endt )
        {
            destroyStagingTextureImpl( *itor );
            delete *itor;
            ++itor;
        }

        mAvailableStagingTextures.clear();

        itor = mUsedStagingTextures.begin();
        endt = mUsedStagingTextures.end();

        while( itor != endt )
        {
            destroyStagingTextureImpl( *itor );
            delete *itor;
            ++itor;
        }

        mUsedStagingTextures.clear();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllTextures()
    {
        ResourceEntryMap::const_iterator itor = mEntries.begin();
        ResourceEntryMap::const_iterator endt = mEntries.end();

        while( itor != endt )
        {
            const ResourceEntry &entry = itor->second;
            delete entry.texture;
            ++itor;
        }

        mEntries.clear();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllPools()
    {
        TexturePoolList::const_iterator itor = mTexturePool.begin();
        TexturePoolList::const_iterator endt = mTexturePool.end();

        while( itor != endt )
        {
            delete itor->masterTexture;
            ++itor;
        }

        mTexturePool.clear();
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *TextureGpuManager::reservePoolId( uint32 poolId, uint32 width, uint32 height,
                                                  uint32 numSlices, uint8 numMipmaps,
                                                  PixelFormatGpu pixelFormat )
    {
        IdType newId = Id::generateNewId<TextureGpuManager>();
        char tmpBuffer[64];
        LwString texName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
        texName.a( "_ReservedTex", newId );

        TexturePool newPool;
        newPool.masterTexture = createTextureImpl( GpuPageOutStrategy::Discard, texName.c_str(),
                                                   TextureFlags::PoolOwner, TextureTypes::Type2DArray );
        newPool.manuallyReserved = true;
        newPool.usedMemory = 0;
        newPool.usedSlots.reserve( numSlices );

        newPool.masterTexture->_setSourceType( TextureSourceType::PoolOwner );
        newPool.masterTexture->setResolution( width, height, numSlices );
        newPool.masterTexture->setPixelFormat( pixelFormat );
        newPool.masterTexture->setNumMipmaps( numMipmaps );
        newPool.masterTexture->setTexturePoolId( poolId );

        mTexturePool.push_back( newPool );

        newPool.masterTexture->_transitionTo( GpuResidency::Resident, 0 );
        newPool.masterTexture->notifyDataIsReady();

        return newPool.masterTexture;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::hasPoolId( uint32 poolId, uint32 width, uint32 height, uint8 numMipmaps,
                                       PixelFormatGpu pixelFormat ) const
    {
        bool retVal = false;

        TexturePoolList::const_iterator itPool = mTexturePool.begin();
        TexturePoolList::const_iterator enPool = mTexturePool.end();

        while( itPool != enPool && !retVal )
        {
            const TexturePool &pool = *itPool;
            if( pool.masterTexture->getWidth() == width && pool.masterTexture->getHeight() == height &&
                pool.masterTexture->getPixelFormat() == pixelFormat &&
                pool.masterTexture->getNumMipmaps() == numMipmaps &&
                pool.masterTexture->getTexturePoolId() == poolId )
            {
                retVal = true;
            }

            ++itPool;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *TextureGpuManager::createTexture( const String &name, const String &aliasName,
                                                  GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                                  uint32 textureFlags,
                                                  TextureTypes::TextureTypes initialType,
                                                  const String &resourceGroup, uint32 filters,
                                                  uint32 poolId )
    {
        OgreProfileExhaustive( "TextureGpuManager::createTexture" );

        IdString idName( aliasName );

        if( mEntries.find( aliasName ) != mEntries.end() )
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "A texture with name '" + aliasName + "' already exists. (Real tex name: '" +
                             name + "')",
                         "TextureGpuManager::createTexture" );
        }

        if( initialType != TextureTypes::Unknown && initialType != TextureTypes::Type2D &&
            textureFlags & TextureFlags::AutomaticBatching )
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "Only Type2D textures can use TextureFlags::AutomaticBatching.",
                         "TextureGpuManager::createTexture" );
        }

        if( mIgnoreSRgbPreference )
            textureFlags &= static_cast<uint32>( ~TextureFlags::PrefersLoadingFromFileAsSRGB );

        filters = mTextureGpuManagerListener->getFiltersFor( name, aliasName, filters );

        TextureGpu *retVal = createTextureImpl( pageOutStrategy, idName, textureFlags, initialType );
        retVal->setTexturePoolId( poolId );
        retVal->_setSourceType( TextureSourceType::Standard );

        mEntriesMutex.lock();
        mEntries[idName] = ResourceEntry( name, aliasName, resourceGroup, retVal, filters );
        mEntriesMutex.unlock();

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *TextureGpuManager::createTexture( const String &name,
                                                  GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                                  uint32 textureFlags,
                                                  TextureTypes::TextureTypes initialType,
                                                  const String &resourceGroup, uint32 filters,
                                                  uint32 poolId )
    {
        return createTexture( name, name, pageOutStrategy, textureFlags, initialType, resourceGroup,
                              filters, poolId );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *TextureGpuManager::createOrRetrieveTexture(
        const String &name, const String &aliasName,
        GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, uint32 textureFlags,
        TextureTypes::TextureTypes initialType, const String &resourceGroup, uint32 filters,
        uint32 poolId )
    {
        TextureGpu *retVal = 0;

        IdString idName( aliasName );
        ResourceEntryMap::const_iterator itor = mEntries.find( idName );
        if( itor != mEntries.end() && !itor->second.destroyRequested )
        {
            retVal = itor->second.texture;
        }
        else
        {
            if( itor != mEntries.end() )
            {
                // The use requested to destroy the texture. It will soon become a dangling pointer
                // and invalidate the iterator. Wait for that to happen.
                // We can't use TextureGpu::waitForData because 'this' will become dangling while
                // inside that function
                waitForStreamingCompletion();
            }

            retVal = createTexture( name, aliasName, pageOutStrategy, textureFlags, initialType,
                                    resourceGroup, filters, poolId );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *TextureGpuManager::createOrRetrieveTexture(
        const String &name, GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy, uint32 textureFlags,
        TextureTypes::TextureTypes initialType, const String &resourceGroup, uint32 filters,
        uint32 poolId )
    {
        return createOrRetrieveTexture( name, name, pageOutStrategy, textureFlags, initialType,
                                        resourceGroup, filters, poolId );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *TextureGpuManager::createOrRetrieveTexture(
        const String &name, const String &aliasName,
        GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
        CommonTextureTypes::CommonTextureTypes type, const String &resourceGroup, uint32 poolId )
    {
        uint32 textureFlags = TextureFlags::AutomaticBatching;
        uint32 filters = TextureFilter::TypeGenerateDefaultMipmaps;
        TextureTypes::TextureTypes texType = TextureTypes::Type2D;
        if( type == CommonTextureTypes::Diffuse || type == CommonTextureTypes::EnvMap )
            textureFlags |= TextureFlags::PrefersLoadingFromFileAsSRGB;
        if( type == CommonTextureTypes::NormalMap )
            filters |= TextureFilter::TypePrepareForNormalMapping;
        if( type == CommonTextureTypes::Monochrome )
            filters |= TextureFilter::TypeLeaveChannelR;
        if( type == CommonTextureTypes::EnvMap )
        {
            textureFlags &= ~static_cast<uint32>( TextureFlags::AutomaticBatching );
            texType = TextureTypes::TypeCube;
        }

        return createOrRetrieveTexture( name, aliasName, pageOutStrategy, textureFlags, texType,
                                        resourceGroup, filters, poolId );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *TextureGpuManager::createOrRetrieveTexture(
        const String &name, GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
        CommonTextureTypes::CommonTextureTypes type, const String &resourceGroup, uint32 poolId )
    {
        return createOrRetrieveTexture( name, name, pageOutStrategy, type, resourceGroup, poolId );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *TextureGpuManager::findTextureNoThrow( IdString name ) const
    {
        TextureGpu *retVal = 0;
        ResourceEntryMap::const_iterator itor = mEntries.find( name );

        if( itor != mEntries.end() && !itor->second.destroyRequested )
            retVal = itor->second.texture;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyTextureImmediate( TextureGpu *texture )
    {
        OgreProfileExhaustive( "TextureGpuManager::destroyTexture" );

        ResourceEntryMap::iterator itor = mEntries.find( texture->getName() );

        if( itor == mEntries.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Texture with name '" + texture->getName().getFriendlyText() +
                             "' not found. Perhaps already destroyed?",
                         "TextureGpuManager::destroyTextureImmediate" );
        }

        texture->notifyAllListenersTextureChanged( TextureGpuListener::Deleted );

        BarrierSolver &barrierSolver = mRenderSystem->getBarrierSolver();
        barrierSolver.textureDeleted( texture );

        delete texture;
        mEntriesMutex.lock();
        mEntries.erase( itor );
        mEntriesMutex.unlock();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyTexture( TextureGpu *texture )
    {
        if( !texture->isPoolOwner() )
        {
            // Almost all textures
            ResourceEntryMap::iterator itor = mEntries.find( texture->getName() );

            if( itor == mEntries.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Texture with name '" + texture->getName().getFriendlyText() +
                                 "' not found. Perhaps already destroyed?",
                             "TextureGpuManager::destroyTexture" );
            }

            if( itor->second.destroyRequested )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "Texture with name '" + texture->getName().getFriendlyText() +
                                 "' has already been scheduled for destruction!",
                             "TextureGpuManager::destroyTexture" );
            }

            itor->second.destroyRequested = true;

            ScheduledTasks task;
            task.tasksType = TaskTypeDestroyTexture;

            if( texture->getPendingResidencyChanges() == 0 )
            {
                // If the TextureGpu is in the worker thread, the following may be true:
                //  1. Texture is not yet Resident. Thus getPendingResidencyChanges cannot be 0
                //  2. Texture is Resident, but being loaded. Thus getPendingResidencyChanges will be 0
                //     but _isDataReadyImpl returns false
                //  3. Texture will become OnSystemRam, after it finishes loading. Thus
                //     getPendingResidencyChanges cannot be 0
                //
                // Thus we know for sure the TextureGpu is not in the worker thread with this if
                // statement
                if( texture->_isDataReadyImpl() ||
                    texture->getResidencyStatus() != GpuResidency::Resident )
                {
                    // There are no pending tasks. We can execute it right now
                    executeTask( texture, TextureGpuListener::ReadyForRendering, task );
                }
                else
                {
                    // No pending tasks, but the texture is being loaded. Delay execution
                    texture->_addPendingResidencyChanges( 1u );
                    mScheduledTasks[texture].push_back( task );
                }
            }
            else
            {
                texture->_addPendingResidencyChanges( 1u );
                mScheduledTasks[texture].push_back( task );
            }
        }
        else
        {
            // Textures that are owners of a pool that were created
            // with reservePoolId. Texture pool owners that weren't
            // created with that function (i.e. automatically / on demand)
            // are released automatically in _releaseSlotFromTexture
            TexturePoolList::iterator itor = mTexturePool.begin();
            TexturePoolList::iterator endt = mTexturePool.end();

            while( itor != endt && itor->masterTexture != texture )
                ++itor;

            if( itor == mTexturePool.end() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Texture with name '" + texture->getName().getFriendlyText() +
                                 "' owner of a TexturePool not found. Perhaps already destroyed?",
                             "TextureGpuManager::destroyTexture" );
            }

            OGRE_ASSERT_LOW( itor->manuallyReserved &&
                             "Pools that were created automatically "
                             "should not be destroyed manually via TextureGpuManager::destroyTexture."
                             " These pools will be destroyed automatically once they're empty" );

            if( !itor->empty() )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Texture with name '" + texture->getName().getFriendlyText() +
                                 "' cannot be deleted! It's a TexturePool and it still has "
                                 "live textures using it! You must release those first by "
                                 "removing them from being Resident",
                             "TextureGpuManager::destroyTexture" );
            }

            delete itor->masterTexture;
            mTexturePool.erase( itor );
        }
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::hasTextureResource( const String &aliasName,
                                                const String &resourceGroup ) const
    {
        if( findTextureNoThrow( aliasName ) != 0 )
            return true;
        ResourceGroupManager &resourceGroupManager = ResourceGroupManager::getSingleton();
        ResourceLoadingListener *loadingListener = resourceGroupManager.getLoadingListener();
        if( loadingListener )
        {
            if( loadingListener->grouplessResourceExists( aliasName ) )
                return true;
        }
        return resourceGroupManager.resourceExists( resourceGroup, aliasName );
    }
    //-----------------------------------------------------------------------------------
    StagingTexture *TextureGpuManager::getStagingTexture( uint32 width, uint32 height, uint32 depth,
                                                          uint32 slices, PixelFormatGpu pixelFormat,
                                                          size_t minConsumptionRatioThreshold )
    {
        OgreProfileExhaustive( "TextureGpuManager::getStagingTexture" );
        assert( minConsumptionRatioThreshold <= 100u && "Invalid consumptionRatioThreshold value!" );

#if OGRE_DEBUG_MEMORY_CONSUMPTION
        {
            char tmpBuffer[512];
            LwString text( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
            text.a( "TextureGpuManager::getStagingTexture: ", width, "x", height, "x" );
            text.a( depth, "x", slices, " ", PixelFormatGpuUtils::toString( pixelFormat ) );
            text.a( " (",
                    (uint32)PixelFormatGpuUtils::getSizeBytes( width, height, depth, slices, pixelFormat,
                                                               4u ) /
                        1024u / 1024u,
                    " MB)" );
            LogManager::getSingleton().logMessage( text.c_str() );
        }
#endif

        StagingTextureVec::iterator bestCandidate = mAvailableStagingTextures.end();
        StagingTextureVec::iterator itor = mAvailableStagingTextures.begin();
        StagingTextureVec::iterator endt = mAvailableStagingTextures.end();

        while( itor != endt )
        {
            StagingTexture *stagingTexture = *itor;

            if( stagingTexture->supportsFormat( width, height, depth, slices, pixelFormat ) &&
                ( bestCandidate == endt || stagingTexture->isSmallerThan( *bestCandidate ) ) )
            {
                if( !stagingTexture->uploadWillStall() )
                    bestCandidate = itor;
            }

            ++itor;
        }

        StagingTexture *retVal = 0;

        if( bestCandidate != endt && minConsumptionRatioThreshold != 0u )
        {
            const size_t requiredSize =
                PixelFormatGpuUtils::getSizeBytes( width, height, depth, slices, pixelFormat, 4u );
            const size_t ratio = ( requiredSize * 100u ) / ( *bestCandidate )->_getSizeBytes();
            if( ratio < minConsumptionRatioThreshold )
                bestCandidate = endt;
        }

        if( bestCandidate != endt )
        {
            retVal = *bestCandidate;
            mUsedStagingTextures.push_back( *bestCandidate );
            mAvailableStagingTextures.erase( bestCandidate );
        }
        else
        {
            // Couldn't find an existing StagingTexture that could handle our request.
            // Check that our memory budget isn't exceeded.
            retVal = checkStagingTextureLimits( width, height, depth, slices, pixelFormat,
                                                minConsumptionRatioThreshold );
            if( !retVal )
            {
                // We haven't yet exceeded our budget, or we did exceed it and
                // checkStagingTextureLimits freed some memory. Either way, create a new one.
                retVal = createStagingTextureImpl( width, height, depth, slices, pixelFormat );
                mUsedStagingTextures.push_back( retVal );
            }
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::removeStagingTexture( StagingTexture *stagingTexture )
    {
        // Reverse search to speed up since most removals are
        // likely to remove what has just been requested.
        StagingTextureVec::reverse_iterator ritor =
            std::find( mUsedStagingTextures.rbegin(), mUsedStagingTextures.rend(), stagingTexture );
        assert( ritor != mUsedStagingTextures.rend() &&
                "StagingTexture does not belong to this TextureGpuManager or already removed" );

        StagingTextureVec::iterator itor = ritor.base() - 1u;
        efficientVectorRemove( mUsedStagingTextures, itor );

        mAvailableStagingTextures.push_back( stagingTexture );
    }
    //-----------------------------------------------------------------------------------
    AsyncTextureTicket *TextureGpuManager::createAsyncTextureTicket( uint32 width, uint32 height,
                                                                     uint32 depthOrSlices,
                                                                     TextureTypes::TextureTypes texType,
                                                                     PixelFormatGpu pixelFormatFamily )
    {
        pixelFormatFamily = PixelFormatGpuUtils::getFamily( pixelFormatFamily );
        AsyncTextureTicket *retVal =
            createAsyncTextureTicketImpl( width, height, depthOrSlices, texType, pixelFormatFamily );

        mAsyncTextureTickets.push_back( retVal );
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAsyncTextureTicket( AsyncTextureTicket *ticket )
    {
        // Reverse search to speed up since most removals are
        // likely to remove what has just been requested.
        AsyncTextureTicketVec::reverse_iterator ritor =
            std::find( mAsyncTextureTickets.rbegin(), mAsyncTextureTickets.rend(), ticket );

        assert( ritor != mAsyncTextureTickets.rend() &&
                "AsyncTextureTicket does not belong to this TextureGpuManager or already removed" );

        OGRE_DELETE ticket;

        AsyncTextureTicketVec::iterator itor = ritor.base() - 1u;
        efficientVectorRemove( mAsyncTextureTickets, itor );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllAsyncTextureTicket()
    {
        AsyncTextureTicketVec::const_iterator itor = mAsyncTextureTickets.begin();
        AsyncTextureTicketVec::const_iterator endt = mAsyncTextureTickets.end();

        while( itor != endt )
        {
            OGRE_DELETE *itor;
            ++itor;
        }

        mAsyncTextureTickets.clear();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::saveTexture( TextureGpu *texture, const String &folderPath,
                                         set<String>::type &savedTextures, bool saveOitd,
                                         bool saveOriginal, HlmsTextureExportListener *listener )
    {
        String resourceName = texture->getRealResourceNameStr();

        // Render Targets are... complicated. Let's not, for now.
        if( savedTextures.find( resourceName ) != savedTextures.end() || texture->isRenderToTexture() )
            return;

        DataStreamPtr inFile;
        if( saveOriginal )
        {
            const String aliasName = texture->getNameStr();
            String savingFilename = resourceName;
            if( listener )
            {
                listener->savingChangeTextureNameOriginal( aliasName, resourceName, savingFilename );
            }

            try
            {
                String resourceGroup = texture->getResourceGroupStr();
                if( resourceGroup.empty() )
                    resourceGroup = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;

                inFile =
                    ResourceGroupManager::getSingleton().openResource( resourceName, resourceGroup );
            }
            catch( FileNotFoundException &e )
            {
                // Try opening as an absolute path
                std::fstream *ifs = OGRE_NEW_T( std::fstream, MEMCATEGORY_GENERAL )(
                    resourceName.c_str(), std::ios::binary | std::ios::in );

                if( ifs->is_open() )
                {
                    inFile = DataStreamPtr( OGRE_NEW FileStreamDataStream( resourceName, ifs, true ) );
                }
                else
                {
                    LogManager::getSingleton().logMessage( "WARNING: Could not find texture file " +
                                                           aliasName + " (" + resourceName +
                                                           ") for copying to export location. "
                                                           "Error: " +
                                                           e.getFullDescription() );
                }
            }
            catch( Exception &e )
            {
                LogManager::getSingleton().logMessage( "WARNING: Could not find texture file " +
                                                       aliasName + " (" + resourceName +
                                                       ") for copying to export location. "
                                                       "Error: " +
                                                       e.getFullDescription() );
            }

            if( inFile )
            {
                size_t fileSize = inFile->size();
                vector<uint8>::type fileData;
                fileData.resize( fileSize );
                inFile->read( &fileData[0], fileData.size() );
                std::ofstream outFile( ( folderPath + "/" + savingFilename ).c_str(),
                                       std::ios::binary | std::ios::out );
                outFile.write( (const char *)&fileData[0],
                               static_cast<std::streamsize>( fileData.size() ) );
                outFile.close();
            }
        }

        if( saveOitd )
        {
            String texName = resourceName;
            if( listener )
                listener->savingChangeTextureNameOitd( texName, texture );

            if( texture->getNextResidencyStatus() == GpuResidency::Resident )
            {
                Image2 image;
                image.convertFromTexture( texture, 0u, texture->getNumMipmaps(), true );

                image.save( folderPath + "/" + texName + ".oitd", 0, image.getNumMipmaps() );
            }
        }

        savedTextures.insert( resourceName );
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::checkSupport( PixelFormatGpu format, TextureTypes::TextureTypes,
                                          uint32 textureFlags ) const
    {
        OGRE_ASSERT_LOW(
            textureFlags != TextureFlags::NotTexture &&
            "Invalid textureFlags combination. Asking to check if format is supported to do nothing" );

        if( textureFlags & TextureFlags::AllowAutomipmaps )
        {
            if( !PixelFormatGpuUtils::supportsHwMipmaps( format ) )
                return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------------------
    TextureGpuManager::MetadataCacheEntry::MetadataCacheEntry() :
        width( 0 ),
        height( 0 ),
        depthOrSlices( 0 ),
        pixelFormat( PFG_UNKNOWN ),
        poolId( 0 ),
        textureType( TextureTypes::Unknown ),
        numMipmaps( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::applyMetadataCacheTo( TextureGpu *texture )
    {
        bool retVal = false;
        MetadataCacheMap::const_iterator itor = mMetadataCache.find( texture->getName() );
        if( itor != mMetadataCache.end() )
        {
            MetadataCacheEntry cacheEntry = itor->second;
            texture->setResolution( cacheEntry.width, cacheEntry.height, cacheEntry.depthOrSlices );
            texture->setNumMipmaps( cacheEntry.numMipmaps );
            texture->setTextureType( cacheEntry.textureType );
            texture->setPixelFormat( cacheEntry.pixelFormat );
            texture->setTexturePoolId( cacheEntry.poolId );
            retVal = true;
        }
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_updateMetadataCache( TextureGpu *texture )
    {
        ResourceEntryMap::const_iterator itor = mEntries.find( texture->getName() );

        if( itor != mEntries.end() )
        {
            MetadataCacheEntry entry;
            entry.aliasName = itor->second.alias;
            // entry.resourceName = itor->second.name;
            entry.width = texture->getWidth();
            entry.height = texture->getHeight();
            entry.depthOrSlices = texture->getDepthOrSlices();
            entry.pixelFormat = texture->getPixelFormat();
            entry.poolId = texture->getTexturePoolId();
            entry.textureType = texture->getTextureType();
            entry.numMipmaps = texture->getNumMipmaps();

            mMetadataCache[texture->getName()] = entry;
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_removeMetadataCacheEntry( TextureGpu *texture )
    {
        mMetadataCache.erase( texture->getName() );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::importTextureMetadataCache( const String &filename, const char *jsonString,
                                                        bool bCreateReservedPools )
    {
#if !OGRE_NO_JSON
        rapidjson::Document d;
        d.Parse( jsonString );

        if( d.HasParseError() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "TextureGpuManager::importTextureMetadataCache",
                         "Invalid JSON string in file " + filename + " at line " +
                             StringConverter::toString( d.GetErrorOffset() ) +
                             " Reason: " + rapidjson::GetParseError_En( d.GetParseError() ) );
        }

        rapidjson::Value::ConstMemberIterator itor;
        itor = d.FindMember( "reserved_pool_ids" );

        if( itor != d.MemberEnd() && itor->value.IsArray() && bCreateReservedPools )
        {
            const rapidjson::Value &jsonVal = itor->value;
            const rapidjson::SizeType arraySize = jsonVal.Size();
            for( rapidjson::SizeType i = 0; i < arraySize; ++i )
            {
                if( jsonVal[i].IsObject() )
                {
                    MetadataCacheEntry entry;

                    itor = jsonVal[i].FindMember( "poolId" );
                    if( itor != jsonVal[i].MemberEnd() && itor->value.IsUint() )
                        entry.poolId = itor->value.GetUint();

                    itor = jsonVal[i].FindMember( "resolution" );
                    if( itor != jsonVal[i].MemberEnd() && itor->value.IsArray() &&
                        itor->value.Size() >= 3u && itor->value[0].IsUint() && itor->value[1].IsUint() &&
                        itor->value[2].IsUint() )
                    {
                        entry.width = itor->value[0].GetUint();
                        entry.height = itor->value[1].GetUint();
                        entry.depthOrSlices = itor->value[2].GetUint();
                    }

                    entry.numMipmaps = 1u;
                    itor = jsonVal[i].FindMember( "mipmaps" );
                    if( itor != jsonVal[i].MemberEnd() && itor->value.IsUint() )
                        entry.numMipmaps = static_cast<uint8>( itor->value.GetUint() );

                    itor = jsonVal[i].FindMember( "format" );
                    if( itor != jsonVal[i].MemberEnd() && itor->value.IsString() )
                    {
                        entry.pixelFormat =
                            PixelFormatGpuUtils::getFormatFromName( itor->value.GetString() );
                    }

                    if( entry.width > 0u && entry.height > 0u && entry.depthOrSlices > 0u &&
                        entry.pixelFormat != PFG_UNKNOWN &&
                        !hasPoolId( entry.poolId, entry.width, entry.height, entry.numMipmaps,
                                    entry.pixelFormat ) )
                    {
                        reservePoolId( entry.poolId, entry.width, entry.height, entry.depthOrSlices,
                                       entry.numMipmaps, entry.pixelFormat );
                    }
                }
            }
        }

        itor = d.FindMember( "textures" );
        if( itor != d.MemberEnd() && itor->value.IsObject() )
        {
            rapidjson::Value::ConstMemberIterator itTex = itor->value.MemberBegin();
            rapidjson::Value::ConstMemberIterator enTex = itor->value.MemberEnd();

            while( itTex != enTex )
            {
                if( itTex->value.IsObject() )
                {
                    IdString aliasName = itTex->name.GetString();
                    MetadataCacheEntry entry;

                    entry.aliasName = itTex->name.GetString();

                    itor = itTex->value.FindMember( "poolId" );
                    if( itor != itTex->value.MemberEnd() && itor->value.IsUint() )
                        entry.poolId = itor->value.GetUint();

                    entry.textureType = TextureTypes::Type2D;
                    itor = itTex->value.FindMember( "texture_type" );
                    if( itor != itTex->value.MemberEnd() && itor->value.IsUint() )
                    {
                        entry.textureType = static_cast<TextureTypes::TextureTypes>(
                            Math::Clamp<uint32>( itor->value.GetUint(), 0u, TextureTypes::Type3D ) );
                    }

                    itor = itTex->value.FindMember( "resolution" );
                    if( itor != itTex->value.MemberEnd() && itor->value.IsArray() &&
                        itor->value.Size() >= 3u && itor->value[0].IsUint() && itor->value[1].IsUint() &&
                        itor->value[2].IsUint() )
                    {
                        entry.width = itor->value[0].GetUint();
                        entry.height = itor->value[1].GetUint();
                        entry.depthOrSlices = itor->value[2].GetUint();
                    }

                    entry.numMipmaps = 1u;
                    itor = itTex->value.FindMember( "mipmaps" );
                    if( itor != itTex->value.MemberEnd() && itor->value.IsUint() )
                        entry.numMipmaps = static_cast<uint8>( itor->value.GetUint() );

                    itor = itTex->value.FindMember( "format" );
                    if( itor != itTex->value.MemberEnd() && itor->value.IsString() )
                    {
                        entry.pixelFormat =
                            PixelFormatGpuUtils::getFormatFromName( itor->value.GetString() );
                    }

                    mMetadataCache[aliasName] = entry;
                }

                ++itTex;
            }
        }
#else
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL,
                     "Ogre must be built with JSON support to call this function!",
                     "TextureGpuManager::importTextureMetadataCache" );
#endif
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::exportTextureMetadataCache( String &outJson )
    {
        char tmpBuffer[4096];
        LwString jsonStr( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        jsonStr.a( "{" );
        jsonStr.a( "\n\t\"reserved_pool_ids\" :\n\t[" );

        bool firstIteration = true;
        {
            TexturePoolList::const_iterator itor = mTexturePool.begin();
            TexturePoolList::const_iterator endt = mTexturePool.end();

            while( itor != endt )
            {
                const TexturePool &pool = *itor;
                if( pool.manuallyReserved )
                {
                    if( !firstIteration )
                        jsonStr.a( "," );
                    jsonStr.a( "\n\t\t{\n\t\t\t\"poolId\" : ", pool.masterTexture->getTexturePoolId() );
                    jsonStr.a( ",\n\t\t\t\"resolution\" : [", pool.masterTexture->getWidth(), ", ",
                               pool.masterTexture->getHeight(), ", ",
                               pool.masterTexture->getDepthOrSlices(), "]" );
                    jsonStr.a( ",\n\t\t\t\"mipmaps\" : ", pool.masterTexture->getNumMipmaps() );
                    jsonStr.a( ",\n\t\t\t\"format\" : \"",
                               PixelFormatGpuUtils::toString( pool.masterTexture->getPixelFormat() ),
                               "\"" );
                    jsonStr.a( "\n\t\t}" );
                    firstIteration = false;

                    outJson += jsonStr.c_str();
                    jsonStr.clear();
                }
                ++itor;
            }
        }

        jsonStr.a( "\n\t],\n\t\"textures\" :\n\t{" );
        firstIteration = true;
        MetadataCacheMap::const_iterator itor = mMetadataCache.begin();
        MetadataCacheMap::const_iterator endt = mMetadataCache.end();

        while( itor != endt )
        {
            const MetadataCacheEntry &entry = itor->second;

            if( !firstIteration )
                jsonStr.a( "," );

            jsonStr.a( "\n\t\t\"", entry.aliasName.c_str(), "\" : \n\t\t{" );
            // jsonStr.a( "\n\t\t\t\"resource\" : \"", entry.resourceName.c_str(), "\"" );
            jsonStr.a( "\n\t\t\t\"resolution\" : [", entry.width, ", ", entry.height, ", ",
                       entry.depthOrSlices, "]" );
            jsonStr.a( ",\n\t\t\t\"mipmaps\" : ", entry.numMipmaps );
            jsonStr.a( ",\n\t\t\t\"format\" : \"", PixelFormatGpuUtils::toString( entry.pixelFormat ),
                       "\"" );
            jsonStr.a( ",\n\t\t\t\"texture_type\" : ", (int)entry.textureType );
            jsonStr.a( ",\n\t\t\t\"poolId\" : ", entry.poolId );
            jsonStr.a( "\n\t\t}" );

            outJson += jsonStr.c_str();
            jsonStr.clear();

            firstIteration = false;

            ++itor;
        }
        jsonStr.a( "\n\t}\n}" );
        outJson += jsonStr.c_str();
        jsonStr.clear();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::getMemoryStats( size_t &outTextureBytesCpu, size_t &outTextureBytesGpu,
                                            size_t &outUsedStagingTextureBytes,
                                            size_t &outAvailableStagingTextureBytes )
    {
        outUsedStagingTextureBytes = getConsumedMemoryByStagingTextures( mAvailableStagingTextures );
        outAvailableStagingTextureBytes = getConsumedMemoryByStagingTextures( mUsedStagingTextures );

        size_t textureBytesCpu = 0;
        size_t textureBytesGpu = 0;

        ResourceEntryMap::const_iterator itor = mEntries.begin();
        ResourceEntryMap::const_iterator endt = mEntries.end();

        while( itor != endt )
        {
            const ResourceEntry &entry = itor->second;
            GpuResidency::GpuResidency residency = entry.texture->getResidencyStatus();
            if( residency != GpuResidency::OnStorage )
            {
                const size_t sizeBytes = entry.texture->getSizeBytes();
                if( residency == GpuResidency::Resident )
                    textureBytesGpu += sizeBytes;

                if( residency == GpuResidency::OnSystemRam ||
                    entry.texture->getGpuPageOutStrategy() ==
                        GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
                {
                    textureBytesCpu += sizeBytes;
                }
            }

            ++itor;
        }

        outTextureBytesCpu = textureBytesCpu;
        outTextureBytesGpu = textureBytesGpu;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::dumpStats() const
    {
        char tmpBuffer[512];
        LwString text( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        LogManager &logMgr = LogManager::getSingleton();

        const size_t availSizeBytes = getConsumedMemoryByStagingTextures( mAvailableStagingTextures );
        const size_t usedSizeBytes = getConsumedMemoryByStagingTextures( mUsedStagingTextures );

        text.clear();
        text.a( "Available Staging Textures\t|", (uint32)mAvailableStagingTextures.size(), "|",
                (uint32)availSizeBytes / ( 1024u * 1024u ), " MB\t\t |In use:\t|",
                (uint32)usedSizeBytes / ( 1024u * 1024u ), " MB" );
        logMgr.logMessage( text.c_str() );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::dumpMemoryUsage( Log *log, uint32 mask ) const
    {
        Log *logActual = log == NULL ? LogManager::getSingleton().getDefaultLog() : log;

        logActual->logMessage(
            "==============================="
            "Start dump of TextureGpuManager"
            "===============================",
            LML_CRITICAL );

        logActual->logMessage( "== Dumping Pools ==" );

        logActual->logMessage(
            "||Width|Height|Format|Mipmaps|Size in bytes|"
            "Num. active textures|Total texture capacity|Pool ID|Texture Names",
            LML_CRITICAL );

        size_t bytesInPoolInclWaste = 0;
        size_t bytesInPoolExclWaste = 0;

        vector<char>::type tmpBuffer;
        tmpBuffer.resize( 512 * 1024 );  // 512kb per line should be way more than enough
        LwString text( LwString::FromEmptyPointer( &tmpBuffer[0], tmpBuffer.size() ) );

        TexturePoolList::const_iterator itPool = mTexturePool.begin();
        TexturePoolList::const_iterator enPool = mTexturePool.end();

        while( itPool != enPool )
        {
            const TexturePool &pool = *itPool;
            text.clear();
            text.a( "||", pool.masterTexture->getWidth(), "|", pool.masterTexture->getHeight(), "|" );
            text.a( PixelFormatGpuUtils::toString( pool.masterTexture->getPixelFormat() ), "|",
                    pool.masterTexture->getNumMipmaps(), "|" );
            const size_t bytesInPool = pool.masterTexture->getSizeBytes();
            text.a( (uint32)bytesInPool, "|" );
            text.a( pool.usedMemory - (uint16)pool.availableSlots.size(), "|",
                    pool.masterTexture->getDepthOrSlices() );
            text.a( "|", pool.masterTexture->getTexturePoolId() );

            bytesInPoolInclWaste += bytesInPool;

            TextureGpuVec::const_iterator itTex = pool.usedSlots.begin();
            TextureGpuVec::const_iterator enTex = pool.usedSlots.end();

            while( itTex != enTex )
            {
                TextureGpu *texture = *itTex;
                text.a( "|", texture->getNameStr().c_str() );
                bytesInPoolExclWaste += texture->getSizeBytes();
                ++itTex;
            }

            logActual->logMessage( text.c_str(), LML_CRITICAL );

            ++itPool;
        }

        size_t bytesOutsidePool = 0;

        logActual->logMessage(
            "|Alias|Resource Name|Width|Height|Depth|Num Slices|Format|Mipmaps|MSAA|Size in bytes|"
            "RTT|UAV|Manual|MSAA Explicit|Reinterpretable|AutomaticBatched|Residency",
            LML_CRITICAL );

        ResourceEntryMap::const_iterator itEntry = mEntries.begin();
        ResourceEntryMap::const_iterator enEntry = mEntries.end();

        while( itEntry != enEntry )
        {
            const ResourceEntry &entry = itEntry->second;
            text.clear();

            if( !( ( 1u << entry.texture->getResidencyStatus() ) & mask ) )
            {
                ++itEntry;
                continue;
            }

            const size_t bytesTexture = entry.texture->getSizeBytes();

            text.a( "|", entry.texture->getNameStr().c_str() );
            text.a( "|", entry.texture->getRealResourceNameStr().c_str() );
            text.a( "|", entry.texture->getWidth(), "|", entry.texture->getHeight(), "|" );
            text.a( entry.texture->getDepth(), "|", entry.texture->getNumSlices(), "|" );
            text.a( PixelFormatGpuUtils::toString( entry.texture->getPixelFormat() ), "|",
                    entry.texture->getNumMipmaps(), "|" );
            text.a( entry.texture->getSampleDescription().getColourSamples(), "|", (uint32)bytesTexture,
                    "|" );
            text.a( entry.texture->isRenderToTexture(), "|", entry.texture->isUav(), "|",
                    entry.texture->_isManualTextureFlagPresent(), "|" );
            text.a( entry.texture->hasMsaaExplicitResolves(), "|", entry.texture->isReinterpretable(),
                    "|", entry.texture->hasAutomaticBatching(), "|",
                    GpuResidency::toString( entry.texture->getResidencyStatus() ) );

            if( !entry.texture->hasAutomaticBatching() )
                bytesOutsidePool += bytesTexture;

            logActual->logMessage( text.c_str(), LML_CRITICAL );

            ++itEntry;
        }

        float fMBytesInPoolInclWaste = float( bytesInPoolInclWaste ) / ( 1024.0f * 1024.0f );
        float fMBytesInPoolExclWaste = float( bytesInPoolExclWaste ) / ( 1024.0f * 1024.0f );
        float fMBytesOutsidePool = float( bytesOutsidePool ) / ( 1024.0f * 1024.0f );

        text.clear();
        text.a( "\n|MBs in pools (excluding waste):|", LwString::Float( fMBytesInPoolExclWaste, 2 ),
                "\n|MBs in pools (including waste):|", LwString::Float( fMBytesInPoolInclWaste, 2 ) );
        text.a( "\n|MBs outside of pools:|", LwString::Float( fMBytesOutsidePool, 2 ),
                "\n|Total MBs (excl. waste):|",
                LwString::Float( fMBytesInPoolExclWaste + fMBytesOutsidePool, 2 ) );
        text.a( "\n|Total MBs (incl. waste):|",
                LwString::Float( fMBytesInPoolInclWaste + fMBytesOutsidePool, 2 ) );
        logActual->logMessage( text.c_str(), LML_CRITICAL );

        dumpStats();

        logActual->logMessage(
            "============================="
            "End dump of TextureGpuManager"
            "=============================",
            LML_CRITICAL );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::setTextureGpuManagerListener( TextureGpuManagerListener *listener )
    {
        mTextureGpuManagerListener = listener;
        if( !listener )
            mTextureGpuManagerListener = &sDefaultTextureGpuManagerListener;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::setStagingTextureMaxBudgetBytes( size_t stagingTextureMaxBudgetBytes )
    {
        mStagingTextureMaxBudgetBytes = stagingTextureMaxBudgetBytes;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::setWorkerThreadMaxPreloadBytes( size_t maxPreloadBytes )
    {
        assert( maxPreloadBytes > 0 && "maxPreloadBytes cannot be 0!" );
        mMaxPreloadBytes = std::max<size_t>( 1u, maxPreloadBytes );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::setWorkerThreadMaxPerStagingTextureRequestBytes(
        size_t maxPerStagingTextureRequestBytes )
    {
        assert( maxPerStagingTextureRequestBytes > 0 && "Value cannot be 0!" );
        mStreamingData.maxPerStagingTextureRequestBytes =
            std::max<size_t>( 1u, maxPerStagingTextureRequestBytes );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::setMultiLoadPool( uint32 numThreads )
    {
#if OGRE_PLATFORM != OGRE_PLATFORM_EMSCRIPTEN && !OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD
        if( mMultiLoadWorkerThreads.size() == numThreads )
            return;

        // Stop the threadpool
        mUseMultiload = false;
        if( !mMultiLoadWorkerThreads.empty() )
        {
            mMultiLoadsSemaphore.increment( static_cast<uint32_t>( mMultiLoadWorkerThreads.size() ) );
            Threads::WaitForThreads( mMultiLoadWorkerThreads.size(), mMultiLoadWorkerThreads.data() );
            mMultiLoadWorkerThreads.clear();
        }

        if( numThreads > 0u )
        {
            mUseMultiload = true;
            mMultiLoadWorkerThreads.resize( numThreads );
            for( size_t i = 0u; i < numThreads; ++i )
            {
                mMultiLoadWorkerThreads[i] =
                    Threads::CreateThread( THREAD_GET( updateTextureMultiLoadWorkerThread ), i, this );
            }
        }
#endif
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::setWorkerThreadMinimumBudget( const BudgetEntryVec &budget,
                                                          uint32 maxSplitResolution )
    {
        if( maxSplitResolution == 0 )
            maxSplitResolution = mStreamingData.maxSplitResolution;

        BudgetEntryVec::const_iterator itor = budget.begin();
        BudgetEntryVec::const_iterator endt = budget.end();

        while( itor != endt )
        {
            if( ( itor->minNumSlices > 2u && itor->minResolution >= maxSplitResolution ) ||
                ( itor->minNumSlices > 1u && itor->minResolution > maxSplitResolution ) )
            {
                LogManager::getSingleton().logMessage(
                    "[WARNING] setWorkerThreadMinimumBudget called with minNumSlices = " +
                        StringConverter::toString( itor->minNumSlices ) +
                        " and minResolution = " + StringConverter::toString( itor->minResolution ) +
                        " which can be "
                        "suboptimal given that maxSplitResolution = " +
                        StringConverter::toString( maxSplitResolution ) +
                        "\n"
                        "See "
                        "https://ogrecave.github.io/ogre-next/api/2.2/"
                        "hlms.html#setWorkerThreadMinimumBudget or "
                        "https://github.com/OGRECave/ogre-next/issues/198",
                    LML_CRITICAL );
            }
            ++itor;
        }

        mStreamingData.maxSplitResolution = maxSplitResolution;

        mBudget = budget;
        // Sort in descending order.
        std::sort( mBudget.begin(), mBudget.end(), BudgetEntry() );
    }
    //-----------------------------------------------------------------------------------
    const TextureGpuManager::BudgetEntryVec &TextureGpuManager::getBudget() const { return mBudget; }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::setTrylockMutexFailureLimit( uint32 tryLockFailureLimit )
    {
        mTryLockMutexFailureLimit = tryLockFailureLimit;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::setProfileLoadingTime( bool bProfile )
    {
#ifdef OGRE_PROFILING_TEXTURES
        mProfilingLoadingTime = bProfile;
        if( !bProfile )
            mProfilingData.clear();
#else
        if( bProfile )
        {
            LogManager::getSingleton().logMessage(
                "[WARN] TextureGpuManager::setProfileLoadingTime ignored. Ogre not compiled with "
                "OGRE_PROFILING_TEXTURES" );
        }
#endif
    }
    //-----------------------------------------------------------------------------------
    const String *TextureGpuManager::findAliasNameStr( IdString idName ) const
    {
        const String *retVal = 0;

        mEntriesMutex.lock();
        ResourceEntryMap::const_iterator itor = mEntries.find( idName );

        if( itor != mEntries.end() )
            retVal = &itor->second.alias;
        mEntriesMutex.unlock();

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    const String *TextureGpuManager::findResourceNameStr( IdString idName ) const
    {
        const String *retVal = 0;

        mEntriesMutex.lock();
        ResourceEntryMap::const_iterator itor = mEntries.find( idName );

        if( itor != mEntries.end() )
            retVal = &itor->second.name;
        mEntriesMutex.unlock();

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    const String *TextureGpuManager::findResourceGroupStr( IdString idName ) const
    {
        const String *retVal = 0;

        mEntriesMutex.lock();
        ResourceEntryMap::const_iterator itor = mEntries.find( idName );

        if( itor != mEntries.end() )
            retVal = &itor->second.resourceGroup;
        mEntriesMutex.unlock();

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::taskLoadToSysRamOrResident( TextureGpu *texture, const ScheduledTasks &task )
    {
        OGRE_ASSERT_MEDIUM( task.tasksType == TaskTypeResidencyTransition );

        const GpuResidency::GpuResidency targetResidency = task.residencyTransitionTask.targetResidency;

        if( texture->getResidencyStatus() == GpuResidency::OnStorage ||
            task.residencyTransitionTask.reuploadOnly )
        {
            OGRE_ASSERT_MEDIUM( targetResidency == GpuResidency::Resident ||
                                targetResidency == GpuResidency::OnSystemRam );
            OGRE_ASSERT_MEDIUM( !task.residencyTransitionTask.reuploadOnly ||
                                texture->getResidencyStatus() == GpuResidency::Resident );

            scheduleLoadRequest( texture, task.residencyTransitionTask.image,
                                 task.residencyTransitionTask.autoDeleteImage,
                                 targetResidency == GpuResidency::OnSystemRam,
                                 task.residencyTransitionTask.reuploadOnly,
                                 task.residencyTransitionTask.bSkipMultiload );
        }
        else
        {
            OGRE_ASSERT_MEDIUM( targetResidency == GpuResidency::Resident );
            scheduleLoadFromRam( texture );
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::taskToUnloadOrDestroy( TextureGpu *texture, const ScheduledTasks &task )
    {
        OGRE_ASSERT_MEDIUM( task.tasksType == TaskTypeResidencyTransition ||
                            task.tasksType == TaskTypeDestroyTexture );
        if( task.tasksType == TaskTypeResidencyTransition )
        {
            const GpuResidency::GpuResidency targetResidency =
                task.residencyTransitionTask.targetResidency;
            OGRE_ASSERT_MEDIUM( targetResidency == GpuResidency::OnStorage ||
                                ( texture->getResidencyStatus() == GpuResidency::Resident &&
                                  targetResidency == GpuResidency::OnSystemRam ) );

            texture->_transitionTo( targetResidency, texture->_getSysRamCopy( 0 ) );
        }
        else if( task.tasksType == TaskTypeDestroyTexture )
        {
            destroyTextureImmediate( texture );
        }
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::executeTask( TextureGpu *texture, TextureGpuListener::Reason reason,
                                         const ScheduledTasks &task )
    {
        OGRE_ASSERT_MEDIUM( task.tasksType == TaskTypeResidencyTransition ||
                            task.tasksType == TaskTypeDestroyTexture );

        bool taskExecuted = true;
        switch( reason )
        {
        case TextureGpuListener::FromStorageToSysRam:
            // Possible transitions we can do from here:
            // OnSystemRam   -> OnStorage
            // OnSystemRam   -> Resident

            if( task.tasksType == TaskTypeResidencyTransition )
            {
                OGRE_ASSERT_MEDIUM( task.residencyTransitionTask.targetResidency !=
                                    GpuResidency::OnSystemRam );
                if( task.residencyTransitionTask.targetResidency == GpuResidency::Resident )
                    taskLoadToSysRamOrResident( texture, task );
                else
                    taskToUnloadOrDestroy( texture, task );
            }
            else if( task.tasksType == TaskTypeDestroyTexture )
                taskToUnloadOrDestroy( texture, task );
            break;

        case TextureGpuListener::FromSysRamToStorage:
            // Possible transitions we can do from here:
            // OnStorage     -> OnSystemRam
            // OnStorage     -> Resident
            if( task.tasksType == TaskTypeResidencyTransition )
            {
                if( task.residencyTransitionTask.targetResidency == GpuResidency::Resident ||
                    task.residencyTransitionTask.targetResidency == GpuResidency::OnSystemRam )
                {
                    taskLoadToSysRamOrResident( texture, task );
                }
            }
            else if( task.tasksType == TaskTypeDestroyTexture )
                taskToUnloadOrDestroy( texture, task );
            break;

        case TextureGpuListener::LostResidency:
            // Possible transitions we can do from here:
            // OnStorage     -> OnSystemRam
            // OnStorage     -> Resident
            // OnSystemRam   -> OnStorage
            // OnSystemRam   -> Resident
            if( task.tasksType == TaskTypeResidencyTransition )
            {
                if( task.residencyTransitionTask.targetResidency == GpuResidency::Resident ||
                    task.residencyTransitionTask.targetResidency == GpuResidency::OnSystemRam )
                {
                    taskLoadToSysRamOrResident( texture, task );
                }
                else
                    taskToUnloadOrDestroy( texture, task );
            }
            else if( task.tasksType == TaskTypeDestroyTexture )
                taskToUnloadOrDestroy( texture, task );
            break;

        case TextureGpuListener::ResidentToSysRamSync:
        case TextureGpuListener::ReadyForRendering:
            // Possible transitions we can do from here:
            // Resident      -> OnSystemRam
            // Resident      -> OnStorage
            if( task.tasksType == TaskTypeResidencyTransition ||
                task.tasksType == TaskTypeDestroyTexture )
            {
                taskToUnloadOrDestroy( texture, task );
            }
            break;

        default:
            taskExecuted = false;
            break;
        }

        return taskExecuted;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                                  void *extraData )
    {
        notifyTextureChanged( texture, reason, false );
        mTextureGpuManagerListener->notifyTextureChanged( texture, reason, extraData );

#ifdef OGRE_PROFILING_TEXTURES
        if( mProfilingLoadingTime && !texture->isPoolOwner() &&
            texture->getName() != "___tempMipmapTexture" )
        {
            switch( reason )
            {
            case TextureGpuListener::GainedResidency:
                mProfilingData[texture->getName()] = Ogre::Timer();
                break;
            case TextureGpuListener::ReadyForRendering:
            {
                std::map<IdString, Timer>::iterator itor = mProfilingData.find( texture->getName() );
                if( itor != mProfilingData.end() )
                {
                    uint64 timeMs = itor->second.getMilliseconds();
                    LogManager::getSingleton().logMessage(
                        "[LATENCY PROFILE] Took " + std::to_string( timeMs ) +
                        " ms to go from Resident to Ready. Actual loading time may be lower if system "
                        "was occupied with another texture. Texture: " +
                        texture->getNameStr() );
                }
                break;
            }
            case TextureGpuListener::ExceptionThrown:
            case TextureGpuListener::LostResidency:
            case TextureGpuListener::Deleted:
                mProfilingData.erase( texture->getName() );
                break;
            default:
                break;
            }
        }
#endif
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::notifyTextureChanged( TextureGpu *texture, TextureGpuListener::Reason reason,
                                                  bool ignoreDelay )
    {
        if( mIgnoreScheduledTasks )
            return;

        if( mDelayListenerCalls && !ignoreDelay )
        {
            // Nested notifyTextureChanged calls is a problem. We will execute them later
            mMissedListenerCalls.push_back( MissedListenerCall( texture, reason ) );
        }
        else
        {
            ScheduledTasksMap::iterator itor = mScheduledTasks.find( texture );

            if( itor != mScheduledTasks.end() )
            {
                mDelayListenerCalls = true;
                ScheduledTasksVec::iterator itTask = itor->second.begin();
                bool taskExecuted = executeTask( texture, reason, *itTask );
                if( taskExecuted )
                {
                    itor->second.erase( itTask );
                    if( itor->second.empty() )
                        mScheduledTasks.erase( itor );

                    // If ignoreDelay == true, then our caller is already executing this loop.
                    // Leave the task of executing those delayed calls to our caller. If we
                    // do it, we'll corrupt mMissedListenerCallsTmp/mMissedListenerCalls
                    if( !ignoreDelay )
                    {
                        mMissedListenerCallsTmp.swap( mMissedListenerCalls );
                        while( !mMissedListenerCallsTmp.empty() )
                        {
                            const MissedListenerCall &missed = mMissedListenerCallsTmp.front();
                            this->notifyTextureChanged( missed.texture, missed.reason, true );
                            mMissedListenerCallsTmp.pop_front();
                            // This iteration may have added more entries to mMissedListenerCalls
                            // We need to execute them right after this entry.
                            mMissedListenerCallsTmp.insert( mMissedListenerCallsTmp.begin(),
                                                            mMissedListenerCalls.begin(),
                                                            mMissedListenerCalls.end() );
                            mMissedListenerCalls.clear();
                        }
                    }
                }
                mDelayListenerCalls = false;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    RenderSystem *TextureGpuManager::getRenderSystem() const { return mRenderSystem; }
    //-----------------------------------------------------------------------------------
    VaoManager *TextureGpuManager::getVaoManager() const { return mVaoManager; }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::scheduleLoadRequest( TextureGpu *texture, const String &name,
                                                 const String &resourceGroup, uint32 filters,
                                                 Image2 *image, bool autoDeleteImage, bool toSysRam,
                                                 bool reuploadOnly, bool bSkipMultiload,
                                                 bool skipMetadataCache, uint32 sliceOrDepth )
    {
        // These two can't be true at the same time
        OGRE_ASSERT_MEDIUM( !( toSysRam && reuploadOnly ) );

        Archive *archive = 0;
        ResourceLoadingListener *loadingListener = 0;
        if( resourceGroup != BLANKSTRING )
        {
            bool providedByListener = false;
            ResourceGroupManager &resourceGroupManager = ResourceGroupManager::getSingleton();
            loadingListener = resourceGroupManager.getLoadingListener();
            if( loadingListener )
            {
                if( loadingListener->grouplessResourceExists( name ) )
                    providedByListener = true;
            }

            if( !providedByListener )
            {
                try
                {
                    archive = resourceGroupManager._getArchiveToResource( name, resourceGroup );
                }
                catch( Exception &e )
                {
                    // Log the exception (probably file not found)
                    LogManager::getSingleton().logMessage( e.getFullDescription() );
                    texture->notifyAllListenersTextureChanged( TextureGpuListener::ExceptionThrown, &e );

                    archive = 0;

                    if( !image )
                    {
                        image = new Image2();

                        PixelFormatGpu fallbackFormat = PFG_RGBA8_UNORM_SRGB;

                        // Filters will complain if they need to run but Image2::getAutoDelete
                        // returns false. So we tell them it's already in the
                        // format they expect
                        if( filters & TextureFilter::TypeLeaveChannelR )
                            fallbackFormat = PFG_R8_UNORM;
                        else if( filters & TextureFilter::TypePrepareForNormalMapping )
                            fallbackFormat = PFG_RG8_SNORM;

                        // Continue loading using a fallback
                        image->loadDynamicImage( mErrorFallbackTexData, 2u, 2u, 1u,
                                                 texture->getTextureType(), fallbackFormat, false, 1u );
                        autoDeleteImage = true;
                    }
                }
            }
        }

        if( !skipMetadataCache && !toSysRam && !reuploadOnly &&
            texture->getGpuPageOutStrategy() != GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
        {
            bool metadataSuccess = applyMetadataCacheTo( texture );
            if( metadataSuccess )
                texture->_transitionTo( GpuResidency::Resident, 0 );
        }

        mAddedNewLoadRequests = true;
        ++mLoadRequestsCounter;
        if( !bSkipMultiload && mUseMultiload && !image &&
            sliceOrDepth == std::numeric_limits<uint32>::max() )
        {
            // Send to multiload threadpool.
            mMultiLoadsMutex.lock();
            ++mPendingMultiLoads;
            mMultiLoads.push_back( LoadRequest( name, archive, loadingListener, image, texture,
                                                sliceOrDepth, filters, autoDeleteImage, toSysRam ) );
            mMultiLoadsMutex.unlock();
            mMultiLoadsSemaphore.increment();
        }
        else
        {
            // Send to background streaming thread, to be fully processed serially.
            ThreadData &mainData = mThreadData[c_mainThread];
            mLoadRequestsMutex.lock();
            mainData.loadRequests.push_back( LoadRequest( name, archive, loadingListener, image, texture,
                                                          sliceOrDepth, filters, autoDeleteImage,
                                                          toSysRam ) );
            mLoadRequestsMutex.unlock();
            mWorkerWaitableEvent.wake();
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_scheduleUpdate( TextureGpu *texture, uint32 filters, Image2 *image,
                                             bool autoDeleteImage, bool skipMetadataCache,
                                             uint32 sliceOrDepth )
    {
        Archive *archive = 0;
        ResourceLoadingListener *loadingListener = 0;

        mAddedNewLoadRequests = true;
        ++mLoadRequestsCounter;
        ThreadData &mainData = mThreadData[c_mainThread];
        mLoadRequestsMutex.lock();
        mainData.loadRequests.push_back( LoadRequest( "", archive, loadingListener, image, texture,
                                                      sliceOrDepth, filters, autoDeleteImage, false ) );
        mLoadRequestsMutex.unlock();
        mWorkerWaitableEvent.wake();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::scheduleLoadRequest( TextureGpu *texture, Image2 *image,
                                                 bool autoDeleteImage, bool toSysRam, bool reuploadOnly,
                                                 bool bSkipMultiload )
    {
        OGRE_ASSERT_LOW( ( texture->getResidencyStatus() == GpuResidency::OnStorage && !reuploadOnly ) ||
                         ( texture->getResidencyStatus() == GpuResidency::Resident && reuploadOnly ) );

        String name, resourceGroup;
        uint32 filters = 0;
        ResourceEntryMap::const_iterator itor = mEntries.find( texture->getName() );
        if( itor != mEntries.end() )
        {
            name = itor->second.name;
            resourceGroup = itor->second.resourceGroup;
            filters = itor->second.filters;
        }

        if( texture->getTextureType() != TextureTypes::TypeCube )
        {
            scheduleLoadRequest( texture, name, resourceGroup, filters, image, autoDeleteImage, toSysRam,
                                 reuploadOnly, bSkipMultiload );
        }
        else
        {
            String baseName;
            String ext;
            String::size_type pos = name.find_last_of( '.' );
            if( pos != String::npos )
            {
                baseName = name.substr( 0, pos );
                ext = name.substr( pos + 1u );
            }
            else
            {
                baseName = name;
            }

            String lowercaseExt = ext;
            StringUtil::toLowerCase( lowercaseExt );

            if( lowercaseExt == "dds" || lowercaseExt == "ktx" || lowercaseExt == "oitd" )
            {
                // XX HACK there should be a better way to specify whether
                // all faces are in the same file or not
                scheduleLoadRequest( texture, name, resourceGroup, filters, image, autoDeleteImage,
                                     toSysRam, reuploadOnly, bSkipMultiload );
            }
            else
            {
                static const String suffixes[6] = { "_rt.", "_lf.", "_up.", "_dn.", "_fr.", "_bk." };

                for( uint32 i = 0; i < 6u; ++i )
                {
                    const bool skipMetadataCache = i != 0;
                    scheduleLoadRequest( texture, baseName + suffixes[i] + ext, resourceGroup, filters,
                                         i == 0 ? image : 0, autoDeleteImage, toSysRam, reuploadOnly,
                                         bSkipMultiload, skipMetadataCache, i );
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::scheduleLoadFromRam( TextureGpu *texture )
    {
        OGRE_ASSERT_LOW( texture->getResidencyStatus() == GpuResidency::OnSystemRam );

        Image2 *image = new Image2();
        const bool autoDeleteInternalPtr =
            texture->getGpuPageOutStrategy() != GpuPageOutStrategy::AlwaysKeepSystemRamCopy;
        uint8 *rawBuffer = texture->_getSysRamCopy( 0 );
        image->loadDynamicImage( rawBuffer, autoDeleteInternalPtr, texture );

        String name;
        uint32 filters = 0;
        ResourceEntryMap::const_iterator itor = mEntries.find( texture->getName() );
        if( itor != mEntries.end() )
        {
            name = itor->second.name;
            filters = itor->second.filters;
        }

        // Only allow applying mipmap generation filter, since it's the only filter
        // that may have been skipped when loading from OnStorage -> OnSystemRam
        filters &= TextureFilter::TypeGenerateDefaultMipmaps;
        if( filters & TextureFilter::TypeGenerateDefaultMipmaps )
        {
            // We will transition to Resident, we must ensure the number of mipmaps is set
            // as the HW mipmap filter cannot change it from the background thread.
            uint8 numMipmaps = texture->getNumMipmaps();
            PixelFormatGpu pixelFormat = texture->getPixelFormat();
            TextureFilter::FilterBase::simulateFiltersForCacheConsistency( filters, *image, this,
                                                                           numMipmaps, pixelFormat );
            if( texture->getNumMipmaps() != numMipmaps )
            {
                const bool oldValue = mIgnoreScheduledTasks;
                mIgnoreScheduledTasks = true;
                // These _transitionTo calls are unscheduled and will decrease
                // mPendingResidencyChanges to wrong values that would cause _scheduleTransitionTo to
                // think the TextureGpu is done, thus we need to counter that.
                texture->_addPendingResidencyChanges( 2u );
                texture->_transitionTo( GpuResidency::OnStorage, rawBuffer, false );
                texture->setNumMipmaps( numMipmaps );
                texture->_transitionTo( GpuResidency::OnSystemRam, rawBuffer, false );
                mIgnoreScheduledTasks = oldValue;
            }
        }

        texture->_transitionTo( GpuResidency::Resident, texture->_getSysRamCopy( 0 ), false );

        mAddedNewLoadRequests = true;
        ++mLoadRequestsCounter;
        ThreadData &mainData = mThreadData[c_mainThread];
        mLoadRequestsMutex.lock();
        mainData.loadRequests.push_back(
            LoadRequest( name, 0, 0, image, texture, 0, filters, true, false ) );
        mLoadRequestsMutex.unlock();
        mWorkerWaitableEvent.wake();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_scheduleTransitionTo( TextureGpu *texture,
                                                   GpuResidency::GpuResidency targetResidency,
                                                   Image2 *image, bool autoDeleteImage,
                                                   bool reuploadOnly, bool bSkipMultiload )
    {
        ScheduledTasks task;
        task.tasksType = TaskTypeResidencyTransition;
        task.residencyTransitionTask.init( targetResidency, image, autoDeleteImage, reuploadOnly,
                                           bSkipMultiload );

        // getPendingResidencyChanges should be > 1 because it gets incremented by caller
        OGRE_ASSERT_MEDIUM( texture->getPendingResidencyChanges() != 0u || reuploadOnly );

        if( texture->getPendingResidencyChanges() == 1u ||
            ( reuploadOnly && texture->getPendingResidencyChanges() == 0u ) || mIgnoreScheduledTasks )
        {
            // If we're here, there are no pending tasks that will perform further work
            // on the texture (with one exception: if _isDataReadyImpl does not return true; which
            // means the texture is still in the worker thread and will later get stuffed with
            // the actual data)

            if( targetResidency == GpuResidency::Resident )
            {
                // If we're here then we're doing one of the following transitions:
                // OnStorage     -> Resident
                // OnSystemRam   -> Resident
                // If we're going to Resident, then we're currently not. Start loading
                executeTask( texture, TextureGpuListener::LostResidency, task );
            }
            else if( targetResidency == GpuResidency::OnSystemRam )
            {
                const GpuResidency::GpuResidency currentResidency = texture->getResidencyStatus();
                if( currentResidency == GpuResidency::OnStorage )
                {
                    // OnStorage     -> OnSystemRam
                    executeTask( texture, TextureGpuListener::FromSysRamToStorage, task );
                }
                else if( currentResidency == GpuResidency::Resident )
                {
                    // Resident      -> OnSystemRam
                    if( texture->_isDataReadyImpl() || mIgnoreScheduledTasks )
                        executeTask( texture, TextureGpuListener::ReadyForRendering, task );
                    else
                    {
                        // No pending tasks, but the texture is being loaded. Delay execution
                        mScheduledTasks[texture].push_back( task );
                    }
                }
            }
            else  // if( targetResidency == GpuResidency::OnStorage )
            {
                const GpuResidency::GpuResidency currentResidency = texture->getResidencyStatus();
                if( currentResidency == GpuResidency::OnSystemRam )
                {
                    // OnSystemRam   -> OnStorage
                    executeTask( texture, TextureGpuListener::FromStorageToSysRam, task );
                }
                else if( currentResidency == GpuResidency::Resident )
                {
                    // Resident      -> OnStorage
                    if( texture->_isDataReadyImpl() || mIgnoreScheduledTasks )
                        executeTask( texture, TextureGpuListener::ReadyForRendering, task );
                    else
                    {
                        // No pending tasks, but the texture is being loaded. Delay execution
                        mScheduledTasks[texture].push_back( task );
                    }
                }
            }
        }
        else
        {
            ScheduledTasksMap::iterator itor = mScheduledTasks.find( texture );

            if( itor == mScheduledTasks.end() )
            {
                mScheduledTasks[texture] = ScheduledTasksVec();
                itor = mScheduledTasks.find( texture );
            }

            itor->second.push_back( task );
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_queueDownloadToRam( TextureGpu *texture, bool resyncOnly )
    {
        DownloadToRamEntry entry;

        entry.texture = texture;

        const size_t sizeBytes = texture->getSizeBytes();
        if( !resyncOnly )
        {
            entry.sysRamPtr =
                reinterpret_cast<uint8 *>( OGRE_MALLOC_SIMD( sizeBytes, MEMCATEGORY_RESOURCE ) );
        }
        else
            entry.sysRamPtr = texture->_getSysRamCopy( 0 );
        entry.resyncOnly = resyncOnly;

        const uint8 numMips = texture->getNumMipmaps();

        for( uint8 mip = 0; mip < numMips; ++mip )
        {
            uint32 width = std::max( texture->getWidth() >> mip, 1u );
            uint32 height = std::max( texture->getHeight() >> mip, 1u );
            uint32 depthOrSlices = std::max( texture->getDepth() >> mip, 1u );
            depthOrSlices = std::max( depthOrSlices, texture->getNumSlices() );

            AsyncTextureTicket *asyncTicket = createAsyncTextureTicket(
                width, height, depthOrSlices, texture->getTextureType(), texture->getPixelFormat() );
            asyncTicket->download( texture, mip, false, 0, true );
            entry.asyncTickets.push_back( asyncTicket );
        }

        mDownloadToRamQueue.push_back( entry );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_setIgnoreScheduledTasks( bool ignoreSchedTasks )
    {
        mIgnoreScheduledTasks = ignoreSchedTasks;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::setDefaultMipmapGeneration(
        DefaultMipmapGen::DefaultMipmapGen defaultMipmapGen,
        DefaultMipmapGen::DefaultMipmapGen defaultMipmapGenCubemaps )
    {
        mDefaultMipmapGen = defaultMipmapGen;
        mDefaultMipmapGenCubemaps = defaultMipmapGenCubemaps;
    }
    //-----------------------------------------------------------------------------------
    DefaultMipmapGen::DefaultMipmapGen TextureGpuManager::getDefaultMipmapGeneration() const
    {
        return mDefaultMipmapGen;
    }
    //-----------------------------------------------------------------------------------
    DefaultMipmapGen::DefaultMipmapGen TextureGpuManager::getDefaultMipmapGenerationCubemaps() const
    {
        return mDefaultMipmapGenCubemaps;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_reserveSlotForTexture( TextureGpu *texture )
    {
        bool matchFound = false;

        TexturePoolList::iterator itor = mTexturePool.begin();
        TexturePoolList::iterator endt = mTexturePool.end();

        while( itor != endt && !matchFound )
        {
            const TexturePool &pool = *itor;

            matchFound = pool.hasFreeSlot() && pool.masterTexture->getWidth() == texture->getWidth() &&
                         pool.masterTexture->getHeight() == texture->getHeight() &&
                         pool.masterTexture->getPixelFormat() == texture->getPixelFormat() &&
                         pool.masterTexture->getNumMipmaps() == texture->getNumMipmaps() &&
                         pool.masterTexture->getTexturePoolId() == texture->getTexturePoolId();

            TODO_grow_pool;

            if( !matchFound )
                ++itor;
        }

        if( itor == endt )
        {
            IdType newId = Id::generateNewId<TextureGpuManager>();
            char tmpBuffer[64];
            LwString texName( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
            texName.a( "_InternalTex", newId );

            TexturePool newPool;
            newPool.masterTexture =
                createTextureImpl( GpuPageOutStrategy::Discard, texName.c_str(), TextureFlags::PoolOwner,
                                   TextureTypes::Type2DArray );
            const uint16 numSlices =
                (uint16)mTextureGpuManagerListener->getNumSlicesFor( texture, this );
            newPool.masterTexture->_setSourceType( TextureSourceType::PoolOwner );

            newPool.manuallyReserved = false;
            newPool.usedMemory = 0;
            newPool.usedSlots.reserve( numSlices );

            newPool.masterTexture->setResolution( texture->getWidth(), texture->getHeight(), numSlices );
            newPool.masterTexture->setPixelFormat( texture->getPixelFormat() );
            newPool.masterTexture->setNumMipmaps( texture->getNumMipmaps() );
            newPool.masterTexture->setTexturePoolId( texture->getTexturePoolId() );

            mTexturePool.push_back( newPool );
            itor = --mTexturePool.end();

            itor->masterTexture->_transitionTo( GpuResidency::Resident, 0 );
            itor->masterTexture->notifyDataIsReady();
        }

        uint16 sliceIdx = 0;
        // See if we can reuse a slot that was previously acquired and released
        if( !itor->availableSlots.empty() )
        {
            sliceIdx = itor->availableSlots.back();
            itor->availableSlots.pop_back();
        }
        else
        {
            sliceIdx = itor->usedMemory++;
        }
        itor->usedSlots.push_back( texture );
        texture->_notifyTextureSlotChanged( &( *itor ), sliceIdx );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_releaseSlotFromTexture( TextureGpu *texture )
    {
        // const_cast? Yes. We own it. We could do a linear search to mTexturePool;
        // but it's O(N) vs O(1); and O(N) can quickly turn into O(N!).
        TexturePool *texturePool = const_cast<TexturePool *>( texture->getTexturePool() );
        TextureGpuVec::iterator itor =
            std::find( texturePool->usedSlots.begin(), texturePool->usedSlots.end(), texture );
        assert( itor != texturePool->usedSlots.end() );
        efficientVectorRemove( texturePool->usedSlots, itor );

        const uint16 internalSliceStart = texture->getInternalSliceStart();
        if( texturePool->usedMemory == internalSliceStart + 1u )
            --texturePool->usedMemory;
        else
            texturePool->availableSlots.push_back( internalSliceStart );

        if( texturePool->empty() && !texturePool->manuallyReserved )
        {
            // Destroy the pool if it's no longer needed
            delete texturePool->masterTexture;
            TexturePoolList::iterator itPool = mTexturePool.begin();
            TexturePoolList::iterator enPool = mTexturePool.end();
            while( itPool != enPool && &( *itPool ) != texturePool )
                ++itPool;
            mTexturePool.erase( itPool );
        }

        texture->_notifyTextureSlotChanged( 0, 0 );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::fulfillUsageStats()
    {
        UsageStatsVec::iterator itStats = mStreamingData.prevStats.begin();
        UsageStatsVec::iterator enStats = mStreamingData.prevStats.end();

        while( itStats != enStats )
        {
            --itStats->loopCount;
            if( itStats->loopCount == 0 )
            {
                // This record has been here too long without the worker thread touching it.
                // Remove it.
                itStats = efficientVectorRemove( mStreamingData.prevStats, itStats );
                enStats = mStreamingData.prevStats.end();
            }
            else
            {
                const uint32 rowAlignment = 4u;
                size_t oneSliceBytes = PixelFormatGpuUtils::getSizeBytes(
                    itStats->width, itStats->height, 1u, 1u, itStats->formatFamily, rowAlignment );

                // Round up.
                const uint32 numSlices =
                    uint32( ( itStats->accumSizeBytes + oneSliceBytes - 1u ) / oneSliceBytes );

                bool isSupported = false;

                StagingTextureVec::iterator itor = mStreamingData.availableStagingTex.begin();
                StagingTextureVec::iterator endt = mStreamingData.availableStagingTex.end();

                while( itor != endt && !isSupported )
                {
                    // Check if the free StagingTextures can take the current usage load.
                    isSupported = ( *itor )->supportsFormat( itStats->width, itStats->height, 1u,
                                                             numSlices, itStats->formatFamily );

                    if( isSupported )
                    {
                        mTmpAvailableStagingTex.push_back( *itor );
                        itor = mStreamingData.availableStagingTex.erase( itor );
                        endt = mStreamingData.availableStagingTex.end();
                    }
                    else
                    {
                        ++itor;
                    }
                }

                if( !isSupported )
                {
                    // It cannot. We need a bigger StagingTexture (or one that supports a specific
                    // format)
                    StagingTexture *newStagingTexture = getStagingTexture(
                        itStats->width, itStats->height, 1u, numSlices, itStats->formatFamily, 50u );
                    newStagingTexture->startMapRegion();
                    mTmpAvailableStagingTex.push_back( newStagingTexture );
                }

                ++itStats;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::fulfillMinimumBudget()
    {
        BudgetEntryVec::const_iterator itBudget = mBudget.begin();
        BudgetEntryVec::const_iterator enBudget = mBudget.end();

        while( itBudget != enBudget )
        {
            bool isSupported = false;

            StagingTextureVec::iterator itor = mStreamingData.availableStagingTex.begin();
            StagingTextureVec::iterator endt = mStreamingData.availableStagingTex.end();

            while( itor != endt && !isSupported )
            {
                if( ( *itor )->getFormatFamily() == itBudget->formatFamily )
                {
                    isSupported = ( *itor )->supportsFormat( itBudget->minResolution,      //
                                                             itBudget->minResolution, 1u,  //
                                                             itBudget->minNumSlices,       //
                                                             itBudget->formatFamily );
                }

                if( isSupported )
                {
                    mTmpAvailableStagingTex.push_back( *itor );
                    itor = mStreamingData.availableStagingTex.erase( itor );
                    endt = mStreamingData.availableStagingTex.end();
                }
                else
                {
                    ++itor;
                }
            }

            // We now have to look in mTmpAvailableStagingTex in case fulfillUsageStats
            // already created a staging texture that fulfills the minimum budget.
            itor = mTmpAvailableStagingTex.begin();
            endt = mTmpAvailableStagingTex.end();

            while( itor != endt && !isSupported )
            {
                if( ( *itor )->getFormatFamily() == itBudget->formatFamily )
                {
                    isSupported = ( *itor )->supportsFormat( itBudget->minResolution,      //
                                                             itBudget->minResolution, 1u,  //
                                                             itBudget->minNumSlices,       //
                                                             itBudget->formatFamily );
                }

                ++itor;
            }

            if( !isSupported )
            {
                StagingTexture *newStagingTexture = getStagingTexture( itBudget->minResolution,      //
                                                                       itBudget->minResolution, 1u,  //
                                                                       itBudget->minNumSlices,       //
                                                                       itBudget->formatFamily, 50u );
                newStagingTexture->startMapRegion();
                mTmpAvailableStagingTex.push_back( newStagingTexture );
            }

            ++itBudget;
        }
    }
    //-----------------------------------------------------------------------------------
    bool OrderByStagingTexture( const StagingTexture *_l, const StagingTexture *_r )
    {
        return _l->isSmallerThan( _r );
    }
    void TextureGpuManager::fullfillBudget()
    {
        OgreProfileExhaustive( "TextureGpuManager::fullfillBudget" );

        // Ensure availableStagingTex is sorted in ascending order
        std::sort( mStreamingData.availableStagingTex.begin(), mStreamingData.availableStagingTex.end(),
                   OrderByStagingTexture );

        fulfillUsageStats();
        fulfillMinimumBudget();

        {
            // The textures that are left are wasting memory, thus can be removed.
            StagingTextureVec::const_iterator itor = mStreamingData.availableStagingTex.begin();
            StagingTextureVec::const_iterator endt = mStreamingData.availableStagingTex.end();

            while( itor != endt )
            {
                ( *itor )->stopMapRegion();
                removeStagingTexture( *itor );
                ++itor;
            }

            mStreamingData.availableStagingTex.clear();
        }

        mStreamingData.availableStagingTex.insert( mStreamingData.availableStagingTex.end(),
                                                   mTmpAvailableStagingTex.begin(),
                                                   mTmpAvailableStagingTex.end() );
        mTmpAvailableStagingTex.clear();
        mStreamingData.bytesPreloaded = 0;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::mergeUsageStatsIntoPrevStats()
    {
        // The sole purpose of this function is to perform a moving average
        //(https://en.wikipedia.org/wiki/Moving_average) between past records
        // of requests and new ones, where the newest request is given full
        // weight if it needs more memory than past records.
        // This allows us to accomodate to spikes of RAM demand (i.e. we suddenly
        // have a lot of textures to load), while slowly dying off the memory
        // we reserve over time if no more new requests are seen.
        //
        // There may be more than one entry with the same formatFamily (due to
        // mStreamingData.maxSplitResolution & maxPerStagingTextureRequestBytes)
        // but we only perform moving average on one of the entries, while
        // letting all the other entries to die off quickly (by setting a very
        // low loopCount)
        uint32 c_loopResetValue = 15u;

        UsageStatsVec::const_iterator itor = mStreamingData.usageStats.begin();
        UsageStatsVec::const_iterator endt = mStreamingData.usageStats.end();

        while( itor != endt )
        {
            UsageStatsVec::iterator itPrev = mStreamingData.prevStats.begin();
            UsageStatsVec::iterator enPrev = mStreamingData.prevStats.end();

            // Look for an older entry that is the same pixel format as itor,
            // and that isn't about to be destroyed (if loopCount <= 2 then
            // this entry is very old and should be abandoned, OR it
            // was a special case due to maxSplitResolution; in any case, skip)
            while( itPrev != enPrev &&
                   ( itPrev->formatFamily != itor->formatFamily || itPrev->loopCount <= 2u ) )
            {
                ++itPrev;
            }

            if( itPrev != enPrev )
            {
                const uint32 blockWidth =
                    PixelFormatGpuUtils::getCompressedBlockWidth( itPrev->formatFamily, false );
                const uint32 blockHeight =
                    PixelFormatGpuUtils::getCompressedBlockHeight( itPrev->formatFamily, false );

                // Average current stats with the previous one.
                // But if current one's was bigger, keep current.
                if( blockWidth != 0 )
                {
                    itPrev->width = std::max( itor->width, ( itPrev->width + itor->width ) >> 1u );
                    itPrev->width = alignToNextMultiple( itPrev->width, blockWidth );
                }
                else
                {
                    itPrev->width = itor->width;
                }
                if( blockHeight != 0 )
                {
                    itPrev->height = std::max( itor->height, ( itPrev->height + itor->height ) >> 1u );
                    itPrev->height = alignToNextMultiple( itPrev->height, blockHeight );
                }
                else
                {
                    itPrev->height = itor->height;
                }
                itPrev->accumSizeBytes = std::max(
                    itor->accumSizeBytes, ( itPrev->accumSizeBytes + itor->accumSizeBytes ) >> 1u );
                itPrev->loopCount = c_loopResetValue;
            }
            else
            {
                mStreamingData.prevStats.push_back( *itor );
                if( itor->width <= mStreamingData.maxSplitResolution ||
                    itor->height <= mStreamingData.maxSplitResolution )
                {
                    mStreamingData.prevStats.back().loopCount = c_loopResetValue;
                }
                else
                {
                    mStreamingData.prevStats.back().loopCount = 2u;
                }
            }

            ++itor;
        }

        mStreamingData.usageStats.clear();
    }
    //-----------------------------------------------------------------------------------
    TextureBox TextureGpuManager::getStreaming( ThreadData &workerData, StreamingData &streamingData,
                                                const TextureBox &box, PixelFormatGpu pixelFormat,
                                                StagingTexture **outStagingTexture )
    {
        // No need to check if streamingData.bytesPreloaded >= mMaxPreloadBytes because
        // our caller's caller already does that. Besides this is a static function.
        // This gives us slightly broader granularity control over memory consumption
        //(we may to try to preload all the mipmaps even if mMaxPreloadBytes is exceeded)
        TextureBox retVal;

        StagingTextureVec::iterator itor = workerData.usedStagingTex.begin();
        StagingTextureVec::iterator endt = workerData.usedStagingTex.end();

        while( itor != endt && !retVal.data )
        {
            // supportsFormat will return false if it could never fit, or the format is not
            // compatible.
            if( ( *itor )->supportsFormat( box.width, box.height, box.depth, box.numSlices,
                                           pixelFormat ) )
            {
                retVal =
                    ( *itor )->mapRegion( box.width, box.height, box.depth, box.numSlices, pixelFormat );
                // retVal.data may be null if there's not enough free space (e.g. it's half empty).
                if( retVal.data )
                    *outStagingTexture = *itor;
            }

            ++itor;
        }

        itor = streamingData.availableStagingTex.begin();
        endt = streamingData.availableStagingTex.end();

        while( itor != endt && !retVal.data )
        {
            if( ( *itor )->supportsFormat( box.width, box.height, box.depth, box.numSlices,
                                           pixelFormat ) )
            {
                retVal =
                    ( *itor )->mapRegion( box.width, box.height, box.depth, box.numSlices, pixelFormat );
                if( retVal.data )
                {
                    *outStagingTexture = *itor;

                    // We need to move this to the 'used' textures
                    workerData.usedStagingTex.push_back( *itor );
                    itor = efficientVectorRemove( streamingData.availableStagingTex, itor );
                    endt = streamingData.availableStagingTex.end();
                }
                else
                {
                    ++itor;
                }
            }
            else
            {
                ++itor;
            }
        }

        // Keep track of requests so main thread knows our current workload.
        const PixelFormatGpu formatFamily = PixelFormatGpuUtils::getFamily( pixelFormat );
        UsageStatsVec::iterator itStats = streamingData.usageStats.begin();
        UsageStatsVec::iterator enStats = streamingData.usageStats.end();

        // Always split tracking of textures that are bigger than c_maxSplitResolution in any
        // dimension
        if( box.width >= streamingData.maxSplitResolution ||
            box.height >= streamingData.maxSplitResolution )
        {
            itStats = enStats;
        }

        while( itStats != enStats && itStats->formatFamily != formatFamily &&
               itStats->accumSizeBytes < streamingData.maxPerStagingTextureRequestBytes )
        {
            ++itStats;
        }

        const uint32 rowAlignment = 4u;
        const size_t requiredBytes = PixelFormatGpuUtils::getSizeBytes(
            box.width, box.height, box.depth, box.numSlices, formatFamily, rowAlignment );

        if( itStats == enStats )
        {
            streamingData.usageStats.push_back(
                UsageStats( box.width, box.height, box.getDepthOrSlices(), formatFamily ) );
        }
        else
        {
            itStats->width = std::max( itStats->width, box.width );
            itStats->height = std::max( itStats->height, box.height );
            itStats->accumSizeBytes += requiredBytes;
        }

        streamingData.bytesPreloaded += requiredBytes;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::processQueuedImage( QueuedImage &queuedImage, ThreadData &workerData,
                                                StreamingData &streamingData )
    {
        OgreProfileExhaustive( "TextureGpuManager::processQueuedImage" );

#ifdef OGRE_PROFILING_TEXTURES
        Timer profilingTimer;
#endif

        Image2 &img = queuedImage.image;
        TextureGpu *texture = queuedImage.dstTexture;
        ObjCmdBuffer *commandBuffer = workerData.objCmdBuffer;

        const bool is3DVolume = img.getDepth() > 1u;

        const uint8 firstMip = queuedImage.getMinMipLevel();
        const uint8 numMips = queuedImage.getMaxMipLevelPlusOne();

        for( uint8 i = firstMip; i < numMips; ++i )
        {
            TextureBox srcBox = img.getData( i );
            const uint32 imgDepthOrSlices = srcBox.getDepthOrSlices();

            OGRE_ASSERT_MEDIUM( imgDepthOrSlices < std::numeric_limits<uint8>::max() );

            for( uint32 z = 0; z < imgDepthOrSlices; ++z )
            {
                if( queuedImage.isMipSliceQueued( i, (uint8)z ) )
                {
                    srcBox.z = is3DVolume ? z : 0;
                    srcBox.sliceStart = is3DVolume ? 0 : z;
                    srcBox.depth = 1u;
                    srcBox.numSlices = 1u;

                    StagingTexture *stagingTexture = 0;
                    TextureBox dstBox = getStreaming( workerData, streamingData, srcBox,
                                                      img.getPixelFormat(), &stagingTexture );
                    if( dstBox.data )
                    {
                        // Upload to staging area. CPU -> GPU
                        dstBox.copyFrom( srcBox );
                        if( queuedImage.dstSliceOrDepth != std::numeric_limits<uint32>::max() )
                        {
                            if( !is3DVolume )
                                srcBox.sliceStart += queuedImage.dstSliceOrDepth;
                            else
                                srcBox.z += queuedImage.dstSliceOrDepth;
                        }

                        // Schedule a command to copy from staging to final texture, GPU -> GPU
                        ObjCmdBuffer::UploadFromStagingTex *uploadCmd =
                            commandBuffer->addCommand<ObjCmdBuffer::UploadFromStagingTex>();
                        new( uploadCmd ) ObjCmdBuffer::UploadFromStagingTex( stagingTexture, dstBox,
                                                                             texture, srcBox, i );
                        // This mip has been processed, flag it as done.
                        queuedImage.unqueueMipSlice( i, (uint8)z );
                    }
                }
            }
        }

        if( queuedImage.empty() )
        {
            // We're done uploading this image. Time to run NotifyDataIsReady,
            // unless there's more QueuedImage like us because the Texture is
            // being loaded from multiple files.
            PartialImageMap::iterator itor = streamingData.partialImages.find( texture );
            PartialImageMap::iterator endt = streamingData.partialImages.end();

            if( itor != endt )
                itor->second.numProcessedDepthOrSlices += img.getDepthOrSlices();

            if( itor == endt || itor->second.numProcessedDepthOrSlices == texture->getDepthOrSlices() )
            {
                if( itor != endt )
                {
                    if( itor->second.sysRamPtr )
                    {
                        // We couldn't transition earlier, so we have to do it now that we're done
                        addTransitionToLoadedCmd( commandBuffer, texture, itor->second.sysRamPtr,
                                                  itor->second.toSysRam );
                    }
                    streamingData.partialImages.erase( itor );
                }

                // Filters will be destroyed by NotifyDataIsReady in main thread
                ObjCmdBuffer::NotifyDataIsReady *cmd =
                    commandBuffer->addCommand<ObjCmdBuffer::NotifyDataIsReady>();
                new( cmd ) ObjCmdBuffer::NotifyDataIsReady( texture, queuedImage.filters );
            }
            else
            {
                TextureFilter::FilterBase::destroyFilters( queuedImage.filters );
            }

#ifdef OGRE_PROFILING_TEXTURES
            queuedImage.microsecondsTaken += profilingTimer.getMicroseconds();
            ObjCmdBuffer::LogProfilingData *cmd =
                commandBuffer->addCommand<ObjCmdBuffer::LogProfilingData>();
            new( cmd ) ObjCmdBuffer::LogProfilingData( texture, queuedImage.dstSliceOrDepth,
                                                       queuedImage.microsecondsTaken );
#endif

            // We don't restore bytesPreloaded because it gets reset to 0 by worker thread.
            // Doing so could increase throughput of data we can preload. However it can
            // cause a positive feedback effect where limits don't get respected at all
            //(it keeps preloading more and more)
            // if( streamingData.bytesPreloaded >= queuedImage.image.getSizeBytes() )
            //    streamingData.bytesPreloaded -= queuedImage.image.getSizeBytes();
            queuedImage.destroy();
        }
#ifdef OGRE_PROFILING_TEXTURES
        else
        {
            queuedImage.microsecondsTaken += profilingTimer.getMicroseconds();
        }
#endif
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::addTransitionToLoadedCmd( ObjCmdBuffer *commandBuffer, TextureGpu *texture,
                                                      void *sysRamCopy, bool toSysRam )
    {
        ObjCmdBuffer::TransitionToLoaded *transitionCmd =
            commandBuffer->addCommand<ObjCmdBuffer::TransitionToLoaded>();
        const GpuResidency::GpuResidency targetResidency =
            toSysRam ? GpuResidency::OnSystemRam : GpuResidency::Resident;
        new( transitionCmd ) ObjCmdBuffer::TransitionToLoaded( texture, sysRamCopy, targetResidency );
    }
    //-----------------------------------------------------------------------------------
    unsigned long updateTextureMultiLoadWorkerThread( ThreadHandle *threadHandle )
    {
        Threads::SetThreadName(
            threadHandle, "TxtreLoad#" + StringConverter::toString( threadHandle->getThreadIdx() ) );

        TextureGpuManager *textureManager =
            reinterpret_cast<TextureGpuManager *>( threadHandle->getUserParam() );
        return textureManager->_updateTextureMultiLoadWorkerThread( threadHandle );
    }
    //-----------------------------------------------------------------------------------
    unsigned long TextureGpuManager::_updateTextureMultiLoadWorkerThread( ThreadHandle * )
    {
        LoadRequest loadRequest( "", 0, 0, 0, 0, 0, 0, false, false );
        bool bStillHasWork = false;
        bool bUseMultiload = true;

        while( bUseMultiload || bStillHasWork )
        {
            mMultiLoadsSemaphore.decrementOrWait();

            bool bWorkGrabbed = false;

            mMultiLoadsMutex.lock();
            if( !mMultiLoads.empty() )
            {
                loadRequest = std::move( mMultiLoads.back() );
                mMultiLoads.pop_back();
                bWorkGrabbed = true;
            }
            bStillHasWork = !mMultiLoads.empty();
            // We use memory_order_relaxed because the mutex already guarantees ordering.
            bUseMultiload = mUseMultiload.load( std::memory_order_relaxed );
            mMultiLoadsMutex.unlock();

            if( bWorkGrabbed )
            {
                OGRE_ASSERT_LOW( !loadRequest.image );

                if( ogre_unlikely( !loadRequest.archive && !loadRequest.loadingListener ) )
                {
                    LogManager::getSingleton().logMessage(
                        "ERROR: Did you call createTexture with a valid resourceGroup? "
                        "Texture: " +
                            loadRequest.name,
                        LML_CRITICAL );
                }

                DataStreamPtr data;
                if( !loadRequest.archive )
                    data = loadRequest.loadingListener->grouplessResourceLoading( loadRequest.name );
                else
                {
                    try
                    {
                        data = loadRequest.archive->open( loadRequest.name );
                        if( loadRequest.loadingListener )
                        {
                            loadRequest.loadingListener->grouplessResourceOpened(
                                loadRequest.name, loadRequest.archive, data );
                        }
                    }
                    catch( Exception & )
                    {
                        // We won't deal with it and just carry it on straight to the streaming thread
                        // as if multiload was turned off. It will handle this error.
                        data.reset();
                    }
                }

                Image2 *img = 0;

                if( data )
                {
                    img = new Image2();

                    try
                    {
                        img->load2( data, loadRequest.name );
                    }
                    catch( Exception & )
                    {
                        // See exception handler above. Let streaming thread figure this out.
                        delete img;
                        img = 0;
                        data.reset();
                    }
                }

                ThreadData &mainData = mThreadData[c_mainThread];
                mLoadRequestsMutex.lock();
                mainData.loadRequests.push_back( loadRequest );
                if( data )
                {
                    mainData.loadRequests.back().image = img;
                    mainData.loadRequests.back().autoDeleteImage = true;
                }
                mLoadRequestsMutex.unlock();
                mWorkerWaitableEvent.wake();

                --mPendingMultiLoads;
            }
        }
        return 0;
    }
    //-----------------------------------------------------------------------------------
    unsigned long updateStreamingWorkerThread( ThreadHandle *threadHandle )
    {
        Threads::SetThreadName(
            threadHandle, "TexStream#" + StringConverter::toString( threadHandle->getThreadIdx() ) );

        TextureGpuManager *textureManager =
            reinterpret_cast<TextureGpuManager *>( threadHandle->getUserParam() );
        return textureManager->_updateStreamingWorkerThread( threadHandle );
    }
    //-----------------------------------------------------------------------------------
    unsigned long TextureGpuManager::_updateStreamingWorkerThread( ThreadHandle *threadHandle )
    {
        while( !mShuttingDown )
        {
            mWorkerWaitableEvent.wait();
            _updateStreaming();
        }

        return 0;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::processLoadRequest( ObjCmdBuffer *commandBuffer, ThreadData &workerData,
                                                const LoadRequest &loadRequest )
    {
        OgreProfileExhaustive( "TextureGpuManager::processLoadRequest LoadRequest for first time" );

        bool wasRescheduled = false;

        // WARNING: loadRequest.texture->isMetadataReady and
        // loadRequest.texture->getResidencyStatus are NOT thread safe
        // if it's in mStreamingData.rescheduledTextures and
        // loadRequest.sliceOrDepth != 0 or uint32::max
        set<TextureGpu *>::type::iterator itReschedule =
            mStreamingData.rescheduledTextures.find( loadRequest.texture );
        if( itReschedule != mStreamingData.rescheduledTextures.end() )
        {
            if( ( loadRequest.sliceOrDepth == std::numeric_limits<uint32>::max() ||
                  loadRequest.sliceOrDepth == 0 ) )
            {
                // This is the original first load request that is making it's
                // roundtrip back to us: Worker -> Main -> Worker
                // because the metadata cache lied the last time we parsed it
                mStreamingData.rescheduledTextures.erase( itReschedule );
            }
            else
            {
                // The first slice was already rescheduled. We cannot process this request
                // further until the first slice comes back to us (which will also request
                // all the slices again). Drop the whole thing, do not load anything.
                wasRescheduled = true;
            }
        }

        if( ogre_unlikely( !loadRequest.archive && !loadRequest.loadingListener && !loadRequest.image ) )
        {
            LogManager::getSingleton().logMessage(
                "ERROR: Did you call createTexture with a valid resourceGroup? "
                "Texture: " +
                    loadRequest.name,
                LML_CRITICAL );
        }

        DataStreamPtr data;
        if( !loadRequest.archive && !loadRequest.image )
            data = loadRequest.loadingListener->grouplessResourceLoading( loadRequest.name );
        else if( !loadRequest.image )
        {
            try
            {
                data = loadRequest.archive->open( loadRequest.name );
                if( loadRequest.loadingListener )
                {
                    loadRequest.loadingListener->grouplessResourceOpened( loadRequest.name,
                                                                          loadRequest.archive, data );
                }
            }
            catch( Exception &e )
            {
                // Log the exception
                LogManager::getSingleton().logMessage( e.getFullDescription() );
                // Tell the main thread this happened
                ObjCmdBuffer::ExceptionThrown *exceptionCmd =
                    commandBuffer->addCommand<ObjCmdBuffer::ExceptionThrown>();
                new( exceptionCmd ) ObjCmdBuffer::ExceptionThrown( loadRequest.texture, e );

                data.reset();
            }
        }

        // Load the image from file into system RAM
        Image2 imgStack;
        Image2 *img = loadRequest.image;

#ifdef OGRE_PROFILING_TEXTURES
        Timer profilingTimer;
#endif

        if( !img )
        {
            img = &imgStack;
            if( !wasRescheduled )
            {
                try
                {
                    if( data )
                        img->load2( data, loadRequest.name );
                }
                catch( Exception &e )
                {
                    // Log the exception
                    LogManager::getSingleton().logMessage( e.getFullDescription() );
                    // Tell the main thread this happened
                    ObjCmdBuffer::ExceptionThrown *exceptionCmd =
                        commandBuffer->addCommand<ObjCmdBuffer::ExceptionThrown>();
                    new( exceptionCmd ) ObjCmdBuffer::ExceptionThrown( loadRequest.texture, e );

                    data.reset();
                }

                if( !data )
                {
                    PixelFormatGpu fallbackFormat = PFG_RGBA8_UNORM_SRGB;

                    // Filters will complain if they need to run but Image2::getAutoDelete
                    // returns false. So we tell them it's already in the
                    // format they expect
                    if( loadRequest.filters & TextureFilter::TypeLeaveChannelR )
                        fallbackFormat = PFG_R8_UNORM;
                    else if( loadRequest.filters & TextureFilter::TypePrepareForNormalMapping )
                        fallbackFormat = PFG_RG8_SNORM;

                    // Continue loading using a fallback
                    img->loadDynamicImage( mErrorFallbackTexData, 2u, 2u, 1u,
                                           loadRequest.texture->getTextureType(), fallbackFormat, false,
                                           1u );
                }
            }
        }

        if( ( loadRequest.sliceOrDepth == std::numeric_limits<uint32>::max() ||
              loadRequest.sliceOrDepth == 0 ) &&
            loadRequest.texture->getResidencyStatus() != GpuResidency::OnStorage )
        {
            uint8 numMipmaps = img->getNumMipmaps();
            PixelFormatGpu pixelFormat = img->getPixelFormat();
            if( loadRequest.texture->prefersLoadingFromFileAsSRGB() )
                pixelFormat = PixelFormatGpuUtils::getEquivalentSRGB( pixelFormat );
            TextureFilter::FilterBase::simulateFiltersForCacheConsistency(
                loadRequest.filters, *img, this, numMipmaps, pixelFormat );

            // Check the metadata cache was not out of date
            if( loadRequest.texture->getWidth() != img->getWidth() ||
                loadRequest.texture->getHeight() != img->getHeight() ||
                ( loadRequest.texture->getDepthOrSlices() != img->getDepthOrSlices() &&
                  loadRequest.sliceOrDepth == std::numeric_limits<uint32>::max() ) ||
                loadRequest.texture->getPixelFormat() != pixelFormat ||
                loadRequest.texture->getNumMipmaps() != numMipmaps ||
                ( loadRequest.texture->getTextureType() != img->getTextureType() &&
                  loadRequest.sliceOrDepth == std::numeric_limits<uint32>::max() &&
                  ( img->getHeight() != 1u ||
                    loadRequest.texture->getTextureType() != TextureTypes::Type1D ) ) )
            {
                // It's out of date. Send it back to the main thread to remove residency,
                // and they can send it back to us. A ping pong.
                ObjCmdBuffer::OutOfDateCache *transitionCmd =
                    commandBuffer->addCommand<ObjCmdBuffer::OutOfDateCache>();
                new( transitionCmd ) ObjCmdBuffer::OutOfDateCache( loadRequest.texture, *img );
                mStreamingData.rescheduledTextures.insert( loadRequest.texture );
                wasRescheduled = true;

                LogManager::getSingleton().logMessage(
                    "[INFO] Texture Metadata cache out of date for " + loadRequest.name +
                    " (Alias: " + loadRequest.texture->getNameStr() + ")" );
            }
        }

        if( !wasRescheduled )
        {
            FilterBaseArray filters;
            TextureFilter::FilterBase::createFilters( loadRequest.filters, filters, loadRequest.texture,
                                                      *img, loadRequest.toSysRam );

            if( loadRequest.sliceOrDepth == std::numeric_limits<uint32>::max() ||
                loadRequest.sliceOrDepth == 0 )
            {
                // Single texture or the 1st slice in a cubemap made up of multiple pictures.
                // We must setup a lot of stuff first (like pixel format, resolution, etc)
                if( loadRequest.texture->getResidencyStatus() == GpuResidency::OnStorage )
                {
                    loadRequest.texture->setResolution( img->getWidth(), img->getHeight(),
                                                        img->getDepthOrSlices() );
                    if( loadRequest.sliceOrDepth == std::numeric_limits<uint32>::max() )
                    {
                        // If the texture had already been set it to 1D
                        // and it is viable, then keep the 1D setting.
                        if( img->getHeight() != 1u ||
                            loadRequest.texture->getTextureType() != TextureTypes::Type1D )
                        {
                            loadRequest.texture->setTextureType( img->getTextureType() );
                        }
                    }
                    loadRequest.texture->setPixelFormat( img->getPixelFormat() );
                    loadRequest.texture->setNumMipmaps( img->getNumMipmaps() );
                }

                FilterBaseArray::const_iterator itFilters = filters.begin();
                FilterBaseArray::const_iterator enFilters = filters.end();
                while( itFilters != enFilters )
                {
                    ( *itFilters )->_executeStreaming( *img, loadRequest.texture );
                    ++itFilters;
                }

                const bool needsMultipleImages =
                    img->getTextureType() != loadRequest.texture->getTextureType() &&
                    loadRequest.texture->getTextureType() != TextureTypes::Type1D;
                const bool mustKeepSysRamPtr =
                    loadRequest.toSysRam || loadRequest.texture->getGpuPageOutStrategy() ==
                                                GpuPageOutStrategy::AlwaysKeepSystemRamCopy;

                OGRE_ASSERT_MEDIUM( !needsMultipleImages ||
                                    ( needsMultipleImages &&
                                      loadRequest.sliceOrDepth != std::numeric_limits<uint32>::max() ) );

                void *sysRamCopy = 0;
                if( mustKeepSysRamPtr )
                {
                    if( !needsMultipleImages &&
                        img->getNumMipmaps() == loadRequest.texture->getNumMipmaps() )
                    {
                        // Pass the raw pointer and transfer ownership to TextureGpu
                        sysRamCopy = img->getData( 0 ).data;
                        img->_setAutoDelete( false );
                    }
                    else
                    {
                        // Posibility 1:
                        //  We're loading this texture in parts, i.e. loading each cubemap face
                        //  from multiple files. Thus the pointer in img is not big enough to hold
                        //  all faces.
                        //
                        // Posibility 2:
                        //  The texture will use a HW mipmap filter. We need to reallocate sysRamCopy
                        //  into something much bigger that can hold all mips.
                        //  img & imgDst will still think they can hold just 1 mipmap, but the
                        //  internal pointer from sysRamCopy has room for when it gets passed
                        //  to the TextureGpu
                        //
                        // Both possibilities can happen at the same time
                        const size_t sizeBytes = loadRequest.texture->getSizeBytes();
                        sysRamCopy = reinterpret_cast<uint8 *>(
                            OGRE_MALLOC_SIMD( sizeBytes, MEMCATEGORY_RESOURCE ) );

                        Image2 imgDst;
                        imgDst.loadDynamicImage( sysRamCopy, false, img );

                        const uint8 numMips = img->getNumMipmaps();

                        for( uint8 mip = 0; mip < numMips; ++mip )
                        {
                            TextureBox srcBox = img->getData( mip );
                            TextureBox dstBox = imgDst.getData( mip );
                            dstBox.copyFrom( srcBox );
                        }
                    }
                }

                if( needsMultipleImages )
                {
                    // We'll need more than one Image to load this texture, so track progress
                    mStreamingData.partialImages[loadRequest.texture] =
                        PartialImage( sysRamCopy, loadRequest.toSysRam );
                }

                // Note cannot transition yet if this is loaded using multiple images
                // and we must keep the SysRamPtr from the worker thread
                if( loadRequest.texture->getResidencyStatus() == GpuResidency::OnStorage &&
                    ( !needsMultipleImages || !mustKeepSysRamPtr ) )
                {
                    // We have enough to transition the texture to OnSystemRam / Resident.
                    //
                    // This doesn't mean we are loaded. It means there is RAM/VRAM allocated for
                    // this texture and shaders can start using it. But its pixels are not yet filled.
                    // That's what NotifyDataIsReady is for.
                    addTransitionToLoadedCmd( commandBuffer, loadRequest.texture, sysRamCopy,
                                              loadRequest.toSysRam );
                }
            }
            else
            {
                // Loading a cubemap made of 6 files (the last 5 files will take this path)
                FilterBaseArray::const_iterator itFilters = filters.begin();
                FilterBaseArray::const_iterator enFilters = filters.end();
                while( itFilters != enFilters )
                {
                    ( *itFilters )->_executeStreaming( *img, loadRequest.texture );
                    ++itFilters;
                }

                if( loadRequest.toSysRam || loadRequest.texture->getGpuPageOutStrategy() ==
                                                GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
                {
                    PartialImageMap::iterator itPartImg =
                        mStreamingData.partialImages.find( loadRequest.texture );

                    OGRE_ASSERT_LOW( itPartImg != mStreamingData.partialImages.end() );
                    OGRE_ASSERT_LOW( itPartImg->second.sysRamPtr );

                    Image2 imgDst;
                    imgDst.loadDynamicImage( itPartImg->second.sysRamPtr, false, img );

                    const uint8 numMips = img->getNumMipmaps();

                    for( uint8 mip = 0; mip < numMips; ++mip )
                    {
                        TextureBox srcBox = img->getData( mip );
                        TextureBox dstBox = imgDst.getData( mip );
                        if( img->getTextureType() != TextureTypes::Type3D )
                            dstBox.sliceStart = loadRequest.sliceOrDepth;
                        else
                            dstBox.z = loadRequest.sliceOrDepth;
                        dstBox.copyFrom( srcBox );
                    }

                    if( loadRequest.toSysRam )
                    {
                        itPartImg->second.numProcessedDepthOrSlices += img->getDepthOrSlices();

                        if( itPartImg->second.numProcessedDepthOrSlices ==
                            loadRequest.texture->getDepthOrSlices() )
                        {
                            // We couldn't transition earlier, so we have to do it now that we're
                            // done
                            addTransitionToLoadedCmd( commandBuffer, loadRequest.texture,
                                                      itPartImg->second.sysRamPtr, true );
                            mStreamingData.partialImages.erase( itPartImg );

                            // Filters will be destroyed by NotifyDataIsReady in main thread
                            ObjCmdBuffer::NotifyDataIsReady *cmd =
                                commandBuffer->addCommand<ObjCmdBuffer::NotifyDataIsReady>();
                            new( cmd ) ObjCmdBuffer::NotifyDataIsReady( loadRequest.texture, filters );
                        }
                        else
                        {
                            TextureFilter::FilterBase::destroyFilters( filters );
                        }
                    }
                }
            }

            if( !loadRequest.toSysRam )
            {
                // Queue the image for upload to GPU.
                mStreamingData.queuedImages.push_back( QueuedImage( *img, loadRequest.texture,
                                                                    loadRequest.sliceOrDepth, filters
#ifdef OGRE_PROFILING_TEXTURES
                                                                    ,
                                                                    profilingTimer.getMicroseconds()
#endif
                                                                        ) );
                if( loadRequest.autoDeleteImage )
                    delete loadRequest.image;

                // Try to upload the queued image right now (all of its mipmaps).
                processQueuedImage( mStreamingData.queuedImages.back(), workerData, mStreamingData );

                if( mStreamingData.queuedImages.back().empty() )
                    mStreamingData.queuedImages.pop_back();
            }
            else
            {
                if( loadRequest.autoDeleteImage )
                    delete loadRequest.image;
            }
        }
        else
        {
            if( loadRequest.autoDeleteImage )
                delete loadRequest.image;
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_updateStreaming()
    {
        OgreProfileExhaustive( "TextureGpuManager::_updateStreaming" );

        /*
        Thread Input                Thread Output
        ------------------------------------------
        Fresh StagingTextures       Used StagingTextures
        Load Requests               Filled memory
        Empty CommandBuffers        Set textures (resolution, type, pixel format)
                                    Upload commands
                                    Rare Requests

        Load Requests are protected by mLoadRequestsMutex (short lock) to prevent
        blocking main thread every time a texture is created.

        Set textures is not protected, so reading pixel format, resolution or type
        could potentially invoke a race condition.

        The rest is protected by mMutex, which takes longer. That means the worker
        thread processes a batch of textures together and when it cannot continue
        (whether it's because it ran out of space or it ran out of work) it delivers
        the commands to the main thread.
        */

        mMutex.lock();

        mStreamingData.workerThreadRan = true;

        ThreadData &workerData = mThreadData[c_workerThread];
        ThreadData &mainData = mThreadData[c_mainThread];

        mLoadRequestsMutex.lock();
        // Lock while inside mMutex because _update has access to our
        // workerData.loadRequests. We still need mLoadRequestsMutex
        // to keep our access to mainData.loadRequests as short as possible
        //(we don't want to block the main thread for long).
        if( workerData.loadRequests.empty() )
        {
            workerData.loadRequests.swap( mainData.loadRequests );
        }
        else
        {
            workerData.loadRequests.insert( workerData.loadRequests.end(), mainData.loadRequests.begin(),
                                            mainData.loadRequests.end() );
            mainData.loadRequests.clear();
        }
        mLoadRequestsMutex.unlock();

        ObjCmdBuffer *commandBuffer = workerData.objCmdBuffer;

        const bool processedAnyImage =
            !workerData.loadRequests.empty() || !mStreamingData.queuedImages.empty();

        // First, try to upload the queued images that failed in the previous iteration.
        QueuedImageVec::iterator itQueue = mStreamingData.queuedImages.begin();
        QueuedImageVec::iterator enQueue = mStreamingData.queuedImages.end();

        while( itQueue != enQueue && mStreamingData.bytesPreloaded < mMaxPreloadBytes )
        {
            processQueuedImage( *itQueue, workerData, mStreamingData );
            if( itQueue->empty() )
            {
                itQueue = efficientVectorRemove( mStreamingData.queuedImages, itQueue );
                enQueue = mStreamingData.queuedImages.end();
            }
            else
            {
                ++itQueue;
            }
        }

        const size_t entriesToProcessPerIteration = mEntriesToProcessPerIteration;
        size_t entriesProcessed = 0;
        // Now process new requests from main thread
        LoadRequestVec::const_iterator itor = workerData.loadRequests.begin();
        LoadRequestVec::const_iterator endt = workerData.loadRequests.end();

        while( itor != endt && entriesProcessed < entriesToProcessPerIteration &&
               mStreamingData.bytesPreloaded < mMaxPreloadBytes )
        {
            processLoadRequest( commandBuffer, workerData, *itor );
            ++entriesProcessed;
            ++itor;
        }

        // Two cases:
        //  1. We did something this iteration, and finished 100%.
        //     Main thread could be waiting for us. Let them know.
        //  2. We couldn't do everything in this iteration, which means
        //     we need something from main thread. Wake it up.
        // Note that normally main thread isn't sleeping, but it could be if
        // waitForStreamingCompletion was called.
        bool wakeUpMainThread = false;
        if( ( processedAnyImage && mStreamingData.queuedImages.empty() ) ||
            !mStreamingData.queuedImages.empty() )
        {
            wakeUpMainThread = true;
        }

        workerData.loadRequests.erase(
            workerData.loadRequests.begin(),
            workerData.loadRequests.begin() + static_cast<ptrdiff_t>( entriesProcessed ) );
        mergeUsageStatsIntoPrevStats();
        mMutex.unlock();

        // Wake up outside mMutex to avoid unnecessary contention.
        if( wakeUpMainThread )
            mRequestToMainThreadEvent.wake();
    }
    //-----------------------------------------------------------------------------------
    size_t TextureGpuManager::getConsumedMemoryByStagingTextures(
        const StagingTextureVec &stagingTextures ) const
    {
        size_t totalSizeBytes = 0;
        StagingTextureVec::const_iterator itor = stagingTextures.begin();
        StagingTextureVec::const_iterator endt = stagingTextures.end();
        while( itor != endt )
        {
            totalSizeBytes += ( *itor )->_getSizeBytes();
            ++itor;
        }

        return totalSizeBytes;
    }
    //-----------------------------------------------------------------------------------
    StagingTexture *TextureGpuManager::checkStagingTextureLimits( uint32 width, uint32 height,
                                                                  uint32 depth, uint32 slices,
                                                                  PixelFormatGpu pixelFormat,
                                                                  size_t minConsumptionRatioThreshold )
    {
        const size_t requiredSize =
            PixelFormatGpuUtils::getSizeBytes( width, height, depth, slices, pixelFormat, 4u );

        size_t consumedBytes = getConsumedMemoryByStagingTextures( mAvailableStagingTextures );

        if( consumedBytes + requiredSize < mStagingTextureMaxBudgetBytes )
            return 0;  // We are OK, below limits

        LogManager::getSingleton().logMessage( "Texture memory budget exceeded. Stalling GPU." );

        // NVIDIA driver can let the staging textures accumulate and skyrocket the
        // memory consumption until the process runs out of memory and crashes
        //(if it has a lot of textures to load).
        // Worst part this only repros in some machines, not driver specific.
        // Flushing here fixes it.
        mRenderSystem->_clearStateAndFlushCommandBuffer();

        set<uint32>::type waitedFrames;

        // Before freeing memory, check if we can make some of
        // the existing staging textures available for use.
        StagingTextureVec::iterator bestCandidate = mAvailableStagingTextures.end();
        StagingTextureVec::iterator itor = mAvailableStagingTextures.begin();
        StagingTextureVec::iterator endt = mAvailableStagingTextures.end();
        while( itor != endt && bestCandidate == endt )
        {
            StagingTexture *stagingTexture = *itor;
            const uint32 frameUsed = stagingTexture->getLastFrameUsed();

            if( waitedFrames.find( frameUsed ) == waitedFrames.end() )
            {
                mVaoManager->waitForSpecificFrameToFinish( frameUsed );
                waitedFrames.insert( frameUsed );
            }

            if( stagingTexture->supportsFormat( width, height, depth, slices, pixelFormat ) &&
                ( bestCandidate == endt || stagingTexture->isSmallerThan( *bestCandidate ) ) )
            {
                const size_t ratio = ( requiredSize * 100u ) / ( *itor )->_getSizeBytes();
                if( ratio >= minConsumptionRatioThreshold )
                    bestCandidate = itor;
            }

            ++itor;
        }

        StagingTexture *retVal = 0;

        if( bestCandidate == endt )
        {
            LogManager::getSingleton().logMessage( "Stalling was not enough. Freeing memory." );

            // Could not find any best candidate even after stalling.
            // Start deleting staging textures until we've freed enough space.
            itor = mAvailableStagingTextures.begin();
            while( itor != endt && ( consumedBytes + requiredSize > mStagingTextureMaxBudgetBytes ) )
            {
                consumedBytes -= ( *itor )->_getSizeBytes();
                destroyStagingTextureImpl( *itor );
                delete *itor;
                ++itor;
            }

            mAvailableStagingTextures.erase( mAvailableStagingTextures.begin(), itor );

            mRenderSystem->_clearStateAndFlushCommandBuffer();
        }
        else
        {
            retVal = *bestCandidate;
            mUsedStagingTextures.push_back( *bestCandidate );
            mAvailableStagingTextures.erase( bestCandidate );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::processDownloadToRamQueue()
    {
        DownloadToRamEntryVec readyTextures;

        DownloadToRamEntryVec::iterator itor = mDownloadToRamQueue.begin();
        DownloadToRamEntryVec::iterator endt = mDownloadToRamQueue.end();

        while( itor != endt )
        {
            Image2 image;  // Use an Image2 as helper for calculating offsets
            image.loadDynamicImage( itor->sysRamPtr, false, itor->texture );

            bool hasPendingTransfers = false;
            AsyncTextureTicketVec::iterator itTicket = itor->asyncTickets.begin();
            AsyncTextureTicketVec::iterator enTicket = itor->asyncTickets.end();

            while( itTicket != enTicket )
            {
                if( *itTicket )
                {
                    if( !( *itTicket )->queryIsTransferDone() )
                        hasPendingTransfers = true;
                    else
                    {
                        AsyncTextureTicket *asyncTicket = *itTicket;

                        const uint8 currentMip =
                            static_cast<uint8>( itTicket - itor->asyncTickets.begin() );
                        TextureBox dstBox = image.getData( currentMip );

                        if( asyncTicket->canMapMoreThanOneSlice() )
                        {
                            const TextureBox srcBox = asyncTicket->map( 0 );
                            dstBox.copyFrom( srcBox );
                            asyncTicket->unmap();
                        }
                        else
                        {
                            const uint32 numSlices = itor->texture->getNumSlices();

                            for( uint32 i = 0; i < numSlices; ++i )
                            {
                                const TextureBox srcBox = asyncTicket->map( i );
                                dstBox.copyFrom( srcBox );
                                dstBox.data = dstBox.at( 0, 0, 1u );
                                --dstBox.numSlices;
                                asyncTicket->unmap();
                            }
                        }

                        destroyAsyncTextureTicket( asyncTicket );

                        *itTicket = 0;
                        asyncTicket = 0;
                    }
                }

                ++itTicket;
            }

            if( !hasPendingTransfers )
            {
                itor->asyncTickets.clear();
                readyTextures.push_back( *itor );
                itor = mDownloadToRamQueue.erase( itor );
                endt = mDownloadToRamQueue.end();
            }
            else
            {
                ++itor;
            }
        }

        itor = readyTextures.begin();
        endt = readyTextures.end();

        while( itor != endt )
        {
            itor->texture->_notifySysRamDownloadIsReady( itor->sysRamPtr, itor->resyncOnly );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::_update( bool syncWithWorkerThread )
    {
        OgreProfileExhaustive( "TextureGpuManager::_update" );

        mAddedNewLoadRequests = false;

#if OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN || OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD
        _updateStreaming();
#endif
        // mPendingMultiLoads must be checked BEFORE checking mThreadData[c_workerThread]
        // Otherwise we may see:
        //  1. Main thread: workerData.loadRequests.empty()
        //  2. Multiload thread: Pushes to workerData.loadRequests, decreases mPendingMultiLoads
        //  3. Main thread: mPendingMultiLoads == 0u
        //
        // Thus main thread would see that we are done, when it's not true.
        const bool isPendingMultiLoadDone = mPendingMultiLoads == 0u;
        bool isDone = false;

        ThreadData &mainData = mThreadData[c_mainThread];
        {
            ThreadData &workerData = mThreadData[c_workerThread];
            bool lockSucceeded = false;

            if( mTryLockMutexFailureCount >= mTryLockMutexFailureLimit &&
                mTryLockMutexFailureLimit != std::numeric_limits<uint32>::max() )
            {
                syncWithWorkerThread = true;
                LogManager::getSingleton().logMessage(
                    "WARNING: We failed " + StringConverter::toString( mTryLockMutexFailureCount ) +
                        " times to acquire lock from texture background streaming thread. "
                        "Stalling. If you see this message more than once, something is going "
                        "terribly wrong, or disk loading is incredibly slow. "
                        "See TextureGpuManager::setTrylockMutexFailureLimit documentation",
                    LML_CRITICAL );
            }

            if( !syncWithWorkerThread )
            {
                lockSucceeded = mMutex.tryLock();
            }
            else
            {
                lockSucceeded = true;
                mMutex.lock();
            }

            if( lockSucceeded )
            {
                mTryLockMutexFailureCount = 0;
                std::swap( mainData.objCmdBuffer, workerData.objCmdBuffer );
                mainData.usedStagingTex.swap( workerData.usedStagingTex );
                if( mStreamingData.workerThreadRan )
                {
                    fullfillBudget();
                    mStreamingData.workerThreadRan = false;
                }

                isDone = mainData.loadRequests.empty() && workerData.loadRequests.empty() &&
                         mStreamingData.queuedImages.empty() && isPendingMultiLoadDone;
                mMutex.unlock();
            }
            else
            {
                ++mTryLockMutexFailureCount;
            }
        }

        {
            StagingTextureVec::const_iterator itor = mainData.usedStagingTex.begin();
            StagingTextureVec::const_iterator endt = mainData.usedStagingTex.end();

            while( itor != endt )
            {
                ( *itor )->stopMapRegion();
                ++itor;
            }
        }

        {
            OgreProfileExhaustive( "TextureGpuManager::_update destroy old StagingTextures" );

            StagingTextureVec::iterator itor = mAvailableStagingTextures.begin();
            StagingTextureVec::iterator endt = mAvailableStagingTextures.end();

            const uint32 numFramesThreshold = mVaoManager->getDynamicBufferMultiplier() + 2u;

            // They're kept in order.
            while( itor != endt &&
                   mVaoManager->getFrameCount() - ( *itor )->getLastFrameUsed() > numFramesThreshold )
            {
                destroyStagingTextureImpl( *itor );
                delete *itor;
                ++itor;
            }

            mAvailableStagingTextures.erase( mAvailableStagingTextures.begin(), itor );
        }

        {
            OgreProfileExhaustive( "TextureGpuManager::_update cmd buffer execution" );
            mainData.objCmdBuffer->execute();
            mainData.objCmdBuffer->clear();
        }

        {
            StagingTextureVec::const_iterator itor = mainData.usedStagingTex.begin();
            StagingTextureVec::const_iterator endt = mainData.usedStagingTex.end();

            while( itor != endt )
            {
                removeStagingTexture( *itor );
                ++itor;
            }

            mainData.usedStagingTex.clear();
        }

        processDownloadToRamQueue();

        // After we've checked mainData.loadRequests.empty() inside the lock;
        // we may have added more entries to it due to pending ScheduledTasks that got
        // flushed either by mainData.objCmdBuffer or processDownloadToRamQueue,
        // thus the worker thread now has more work to do and we can't return
        // isDone = true.
        if( mAddedNewLoadRequests )
            isDone = false;

        mWorkerWaitableEvent.wake();

#if OGRE_DEBUG_MODE && 0
        dumpStats();
#endif

        mLastUpdateIsStreamingDone = isDone;

        return isDone;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::isDoneStreaming() const
    {
        return mLastUpdateIsStreamingDone && !mAddedNewLoadRequests && mDownloadToRamQueue.empty() &&
               mScheduledTasks.empty();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::waitForStreamingCompletion()
    {
        OgreProfileExhaustive( "TextureGpuManager::waitForStreamingCompletion" );

        bool bDone = false;
        while( !bDone )
        {
            bool workerThreadDone = _update( true );
            bDone = workerThreadDone && mDownloadToRamQueue.empty() && mScheduledTasks.empty();
            if( !bDone )
            {
                mVaoManager->_update();
                if( !workerThreadDone )
                {
                    // We're waiting for worker thread to finish loading from disk/ram into GPU
                    mRequestToMainThreadEvent.wait();
                }
                else
                {
                    // We're waiting for GPU -> CPU transfers or for the next task to be executed
                    Threads::Sleep( 1 );
                }
            }
#if OGRE_DEBUG_MEMORY_CONSUMPTION
            dumpStats();
#endif
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_waitFor( TextureGpu *texture, bool metadataOnly )
    {
        bool bDone = false;
        while( !bDone )
        {
            bool workerThreadDone = _update( true );
            bDone = workerThreadDone && mDownloadToRamQueue.empty() && mScheduledTasks.empty();
            if( !bDone )
            {
                if( texture->getPendingResidencyChanges() == 0u )
                {
                    if( texture->isDataReady() )
                        bDone = true;
                    else if( metadataOnly && texture->isMetadataReady() )
                        bDone = true;
                }

                if( !bDone )
                {
                    mVaoManager->_update();
                    if( !workerThreadDone )
                        mRequestToMainThreadEvent.wait();
                    else
                        Threads::Sleep( 1 );
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_waitForPendingGpuToCpuSyncs( TextureGpu *texture )
    {
        bool bDone = false;
        while( !bDone )
        {
            DownloadToRamEntryVec::iterator itor = mDownloadToRamQueue.begin();
            DownloadToRamEntryVec::iterator endt = mDownloadToRamQueue.end();

            // Only stall for the texture we're looking for, where resyncOnly == true;
            // since those are from textures currently Resident that will remain Resident.
            // The cases where resyncOnly == false are handled by the residency transition
            while( itor != endt && itor->texture != texture && !itor->resyncOnly )
                ++itor;

            if( itor == endt )
                bDone = true;
            else
            {
                _update( true );
                mVaoManager->_update();
                Threads::Sleep( 1 );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    bool TexturePool::hasFreeSlot() const
    {
        return !availableSlots.empty() || usedMemory < masterTexture->getNumSlices();
    }
    //-----------------------------------------------------------------------------------
    bool TexturePool::empty() const
    {
        const size_t numSlices = masterTexture->getNumSlices();
        return ( availableSlots.size() + ( numSlices - usedMemory ) ) == numSlices;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    TextureGpuManager::UsageStats::UsageStats( uint32 _width, uint32 _height, uint32 _depthOrSlices,
                                               PixelFormatGpu _formatFamily ) :
        width( _width ),
        height( _height ),
        formatFamily( _formatFamily ),
        accumSizeBytes( PixelFormatGpuUtils::getSizeBytes( _width, _height, _depthOrSlices, 1u,
                                                           _formatFamily, 4u ) ),
        loopCount( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    TextureGpuManager::QueuedImage::QueuedImage( Image2 &srcImage, TextureGpu *_dstTexture,
                                                 uint32 _dstSliceOrDepth, FilterBaseArray &inOutFilters
#ifdef OGRE_PROFILING_TEXTURES
                                                 ,
                                                 uint64 _microsecondsTaken
#endif
                                                 ) :
        dstTexture( _dstTexture ),
        autoDeleteImage( srcImage.getAutoDelete() ),
        dstSliceOrDepth( _dstSliceOrDepth )
#ifdef OGRE_PROFILING_TEXTURES
        ,
        microsecondsTaken( _microsecondsTaken )
#endif
    {
        assert( srcImage.getDepthOrSlices() >= 1u );

        filters.swap( inOutFilters );

        // Prevent destroying the internal data in srcImage if QueuedImageVec
        // holding us gets resized. (we do not follow the rule of 3)
        srcImage._setAutoDelete( false );
        image = srcImage;

        const uint8 numMips = image.getNumMipmaps();
        const uint32 numSlices = image.getDepthOrSlices();

        const size_t numMipSlices = numMips * numSlices;

        mipLevelBitSet.reset( numMipSlices + 1u );
        mipLevelBitSet.setAllUntil( numMipSlices );

        if( srcImage.getTextureType() == TextureTypes::Type3D )
        {
            // For 3D textures, depth is not constant per mip level. If we don't unqueue those
            // now we will later get stuck in an infinite loop (empty() will never return true)
            uint32 currDepth = std::max<uint32>( numSlices >> 1u, 1u );
            for( uint8 mip = 1u; mip < numMips; ++mip )
            {
                for( uint32 slice = currDepth; slice < numSlices; ++slice )
                    unqueueMipSlice( mip, static_cast<uint8>( slice ) );
                currDepth = std::max<uint32>( currDepth >> 1u, 1u );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::QueuedImage::destroy()
    {
        if( autoDeleteImage &&
            dstTexture->getGpuPageOutStrategy() != GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
        {
            // Do not delete the internal pointer if the TextureGpu will be owning it.
            image._setAutoDelete( true );
            image.freeMemory();
        }

        assert( filters.empty() &&
                "Internal Error: Failed to send filters to the main thread for destruction. "
                "These filters will leak" );
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::QueuedImage::empty() const { return mipLevelBitSet.empty(); }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::QueuedImage::isMipSliceQueued( uint8 mipLevel, uint8 slice ) const
    {
        const size_t mipSlice = mipLevel * image.getDepthOrSlices() + slice;
        return mipLevelBitSet.test( mipSlice );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::QueuedImage::unqueueMipSlice( uint8 mipLevel, uint8 slice )
    {
        const size_t mipSlice = mipLevel * image.getDepthOrSlices() + slice;
        return mipLevelBitSet.unset( mipSlice );
    }
    //-----------------------------------------------------------------------------------
    uint8 TextureGpuManager::QueuedImage::getMinMipLevel() const
    {
        return static_cast<uint8>( mipLevelBitSet.findFirstBitSet() / image.getDepthOrSlices() );
    }
    //-----------------------------------------------------------------------------------
    uint8 TextureGpuManager::QueuedImage::getMaxMipLevelPlusOne() const
    {
        return static_cast<uint8>(
            ( mipLevelBitSet.findLastBitSetPlusOne() + image.getDepthOrSlices() - 1u ) /
            image.getDepthOrSlices() );
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    TextureGpuManager::PartialImage::PartialImage() :
        sysRamPtr( 0 ),
        numProcessedDepthOrSlices( 0 ),
        toSysRam( false )
    {
    }
    //-----------------------------------------------------------------------------------
    TextureGpuManager::PartialImage::PartialImage( void *_sysRamPtr, bool _toSysRam ) :
        sysRamPtr( _sysRamPtr ),
        numProcessedDepthOrSlices( 0 ),
        toSysRam( _toSysRam )
    {
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::BudgetEntry::operator()( const BudgetEntry &_l, const BudgetEntry &_r ) const
    {
        // Biggest ones come first
        const size_t lSize = PixelFormatGpuUtils::getSizeBytes( _l.minResolution, _l.minResolution, 1u,
                                                                _l.minNumSlices, _l.formatFamily, 4u );
        const size_t rSize = PixelFormatGpuUtils::getSizeBytes( _r.minResolution, _r.minResolution, 1u,
                                                                _r.minNumSlices, _r.formatFamily, 4u );
        return lSize > rSize;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    TextureGpuManager::ScheduledTasks::ScheduledTasks() { memset( this, 0, sizeof( ScheduledTasks ) ); }
}  // namespace Ogre
