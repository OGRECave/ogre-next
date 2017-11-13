/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#include "OgreTextureGpuManagerListener.h"
#include "OgreObjCmdBuffer.h"
#include "OgreTextureGpu.h"
#include "OgreAsyncTextureTicket.h"
#include "OgreStagingTexture.h"
#include "OgrePixelFormatGpuUtils.h"

#include "OgreId.h"
#include "OgreLwString.h"
#include "OgreCommon.h"

#include "Vao/OgreVaoManager.h"
#include "OgreResourceGroupManager.h"
#include "OgreImage2.h"
#include "OgreTextureFilters.h"

#include "Threading/OgreThreads.h"

#include "OgreException.h"
#include "OgreLogManager.h"

#if OGRE_COMPILER == OGRE_COMPILER_MSVC
    #include <intrin.h>
    #if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_32
        #pragma intrinsic(_BitScanForward)
        #pragma intrinsic(_BitScanReverse)
    #else
        #pragma intrinsic(_BitScanForward64)
        #pragma intrinsic(_BitScanReverse64)
    #endif
#endif

//#define OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD 1
//#define OGRE_DEBUG_MEMORY_CONSUMPTION 1

#define TODO_grow_pool 1

namespace Ogre
{
    inline uint32 ctz64( uint64 value )
    {
        if( value == 0 )
            return 64u;

    #if OGRE_COMPILER == OGRE_COMPILER_MSVC
        unsigned long trailingZero = 0;
        #if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_32
            //Scan the high 32 bits.
            if( _BitScanForward( &trailingZero, static_cast<uint32>(value >> 32u) ) )
                return (trailingZero + 32u);

            //Scan the low 32 bits.
            _BitScanForward( &trailingZero, static_cast<uint32>(value) );
        #else
            _BitScanForward64( &trailingZero, value );
        #endif
        return trailingZero;
    #else
        return __builtin_ctzl( value );
    #endif
    }
    inline uint32 clz64( uint64 value )
    {
        if( value == 0 )
            return 64u;

    #if OGRE_COMPILER == OGRE_COMPILER_MSVC
        unsigned long lastBitSet = 0;
        #if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_32
            //Scan the high 32 bits.
            if( _BitScanReverse( &lastBitSet, static_cast<uint32>(value >> 32u) ) )
                return 63u - (lastBitSet + 32u);

            //Scan the low 32 bits.
            _BitScanReverse( &lastBitSet, static_cast<uint32>(value) );
        #else
            _BitScanReverse64( &lastBitSet, value );
        #endif
        return 63u - lastBitSet;
    #else
        return __builtin_clzl( value );
    #endif
    }

    static const int c_mainThread = 0;
    static const int c_workerThread = 1;

    unsigned long updateStreamingWorkerThread( ThreadHandle *threadHandle );
    THREAD_DECLARE( updateStreamingWorkerThread );

    TextureGpuManager::TextureGpuManager( VaoManager *vaoManager, RenderSystem *renderSystem ) :
        mDefaultMipmapGen( DefaultMipmapGen::HwMode ),
        mDefaultMipmapGenCubemaps( DefaultMipmapGen::SwMode ),
        mShuttingDown( false ),
        mEntriesToProcessPerIteration( 3u ),
        mMaxPreloadBytes( 256u * 1024u * 1024u ), //A value of 512MB begins to shake driver bugs.
        mTextureGpuManagerListener( 0 ),
        mStagingTextureMaxBudgetBytes( 512u * 1024u * 1024u ),
        mVaoManager( vaoManager ),
        mRenderSystem( renderSystem )
    {
        mTextureGpuManagerListener = OGRE_NEW DefaultTextureGpuManagerListener();

        PixelFormatGpu format;
#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS && OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
    #if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_32
        // 32-bit have tighter limited addresse memory. They pay the price
        // in slower streaming (more round trips between main and worker threads)
        const uint32 maxResolution = 2048u;
    #else
        // 64-bit have plenty of virtual addresss to spare. We can reserve much more.
        const uint32 maxResolution = 4096u;
    #endif
        //4MB / 64MB for RGBA8, that's two 4096x4096 / 2048x2048 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_RGBA8_UNORM );
        mBudget.push_back( BudgetEntry( format, maxResolution, 2u ) );
        //4MB / 16MB for BC1, that's two 4096x4096 / 2048x2048 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_BC1_UNORM );
        mBudget.push_back( BudgetEntry( format, maxResolution, 2u ) );
        //4MB / 16MB for BC3, that's one 4096x4096 / 2048x2048 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_BC3_UNORM );
        mBudget.push_back( BudgetEntry( format, maxResolution, 1u ) );
        //4MB / 16MB for BC5, that's one 4096x4096 / 2048x2048 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_BC5_UNORM );
        mBudget.push_back( BudgetEntry( format, maxResolution, 1u ) );
#else
        //Mobile platforms don't support compressed formats, and have tight memory constraints
        //8MB for RGBA8, that's two 2048x2048 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_RGBA8_UNORM );
        mBudget.push_back( BudgetEntry( format, 2048u, 2u ) );
#endif

        //Sort in descending order.
        std::sort( mBudget.begin(), mBudget.end(), BudgetEntry() );

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS || OGRE_PLATFORM == OGRE_PLATFORM_ANDROID
        //Mobile platforms are tight on memory. Keep the limits low.
        mMaxPreloadBytes = 32u * 1024u * 1024u;
        mMaxBytesPerStreamingStagingTexture = 32u * 1024u * 1024u;
#else
        #if OGRE_ARCH_TYPE == OGRE_ARCHITECTURE_32
            //32-bit architectures are more limited.
            //The default 256MB can cause Out of Memory conditions due to memory fragmentation.
            mMaxPreloadBytes = 128u * 1024u * 1024u;
            mMaxBytesPerStreamingStagingTexture = 32u * 1024u * 1024u;
        #endif
#endif

        mStreamingData.bytesPreloaded = 0;

        for( int i=0; i<2; ++i )
            mThreadData[i].objCmdBuffer = new ObjCmdBuffer();

#if OGRE_PLATFORM != OGRE_PLATFORM_EMSCRIPTEN && !OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD
        mWorkerThread = Threads::CreateThread( THREAD_GET( updateStreamingWorkerThread ), 0, this );
#endif
    }
    //-----------------------------------------------------------------------------------
    TextureGpuManager::~TextureGpuManager()
    {
        mShuttingDown = true;
#if OGRE_PLATFORM != OGRE_PLATFORM_EMSCRIPTEN && !OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD
        mWorkerWaitableEvent.wake();
        Threads::WaitForThreads( 1u, &mWorkerThread );
#endif

        assert( mAvailableStagingTextures.empty() && "Derived class didn't call destroyAll!" );
        assert( mUsedStagingTextures.empty() && "Derived class didn't call destroyAll!" );
        assert( mEntries.empty() && "Derived class didn't call destroyAll!" );
        assert( mTexturePool.empty() && "Derived class didn't call destroyAll!" );

        for( int i=0; i<2; ++i )
        {
            delete mThreadData[i].objCmdBuffer;
            mThreadData[i].objCmdBuffer = 0;
        }

        OGRE_DELETE mTextureGpuManagerListener;
        mTextureGpuManagerListener = 0;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAll(void)
    {
        waitForStreamingCompletion();

        mMutex.lock();
        destroyAllStagingBuffers();
        destroyAllAsyncTextureTicket();
        destroyAllTextures();
        destroyAllPools();
        mMutex.unlock();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllStagingBuffers(void)
    {
        StagingTextureVec::iterator itor = mStreamingData.availableStagingTex.begin();
        StagingTextureVec::iterator end  = mStreamingData.availableStagingTex.end();

        while( itor != end )
        {
            (*itor)->stopMapRegion();
            ++itor;
        }

        mStreamingData.availableStagingTex.clear();

        itor = mAvailableStagingTextures.begin();
        end  = mAvailableStagingTextures.end();

        while( itor != end )
        {
            destroyStagingTextureImpl( *itor );
            delete *itor;
            ++itor;
        }

        mAvailableStagingTextures.clear();

        itor = mUsedStagingTextures.begin();
        end  = mUsedStagingTextures.end();

        while( itor != end )
        {
            destroyStagingTextureImpl( *itor );
            delete *itor;
            ++itor;
        }

        mUsedStagingTextures.clear();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllTextures(void)
    {
        ResourceEntryMap::const_iterator itor = mEntries.begin();
        ResourceEntryMap::const_iterator end  = mEntries.end();

        while( itor != end )
        {
            const ResourceEntry &entry = itor->second;
            delete entry.texture;
            ++itor;
        }

        mEntries.clear();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllPools(void)
    {
        TexturePoolList::const_iterator itor = mTexturePool.begin();
        TexturePoolList::const_iterator end  = mTexturePool.end();

        while( itor != end )
        {
            delete itor->masterTexture;
            ++itor;
        }

        mTexturePool.clear();
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* TextureGpuManager::createTexture( const String &name, const String &aliasName,
                                                  GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                                  uint32 textureFlags,
                                                  TextureTypes::TextureTypes initialType,
                                                  const String &resourceGroup,
                                                  uint32 filters )
    {
        IdString idName( aliasName );

        if( mEntries.find( aliasName ) != mEntries.end() )
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "A texture with name '" + aliasName +
                         "' already exists. (Real tex name: '" + name + "')",
                         "TextureGpuManager::createTexture" );
        }

        if( initialType != TextureTypes::Unknown && initialType != TextureTypes::Type2D &&
            textureFlags & TextureFlags::AutomaticBatching )
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "Only Type2D textures can use TextureFlags::AutomaticBatching.",
                         "TextureGpuManager::createTexture" );
        }

        TextureGpu *retVal = createTextureImpl( pageOutStrategy, idName, textureFlags, initialType );

        mEntries[idName] = ResourceEntry( name, aliasName, resourceGroup, retVal, filters );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* TextureGpuManager::createTexture( const String &name,
                                                  GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                                  uint32 textureFlags,
                                                  TextureTypes::TextureTypes initialType,
                                                  const String &resourceGroup,
                                                  uint32 filters )
    {
        return createTexture( name, name, pageOutStrategy, textureFlags,
                              initialType, resourceGroup, filters );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* TextureGpuManager::createOrRetrieveTexture(
            const String &name, const String &aliasName,
            GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
            uint32 textureFlags, TextureTypes::TextureTypes initialType, const String &resourceGroup,
            uint32 filters )
    {
        TextureGpu *retVal = 0;

        IdString idName( aliasName );
        ResourceEntryMap::const_iterator itor = mEntries.find( idName );
        if( itor != mEntries.end() )
            retVal = itor->second.texture;
        else
        {
            retVal = createTexture( name, aliasName, pageOutStrategy, textureFlags,
                                    initialType, resourceGroup, filters );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* TextureGpuManager::createOrRetrieveTexture(
            const String &name, GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
            uint32 textureFlags, TextureTypes::TextureTypes initialType, const String &resourceGroup,
            uint32 filters )
    {
        return createOrRetrieveTexture( name, name, pageOutStrategy, textureFlags,
                                        initialType, resourceGroup, filters );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* TextureGpuManager::findTextureNoThrow( IdString name ) const
    {
        TextureGpu *retVal = 0;
        ResourceEntryMap::const_iterator itor = mEntries.find( name );

        if( itor != mEntries.end() )
            retVal = itor->second.texture;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyTexture( TextureGpu *texture )
    {
        ResourceEntryMap::iterator itor = mEntries.find( texture->getName() );

        if( itor == mEntries.end() )
        {
            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                         "Texture with name '" + texture->getName().getFriendlyText() +
                         "' not found. Perhaps already destroyed?",
                         "TextureGpuManager::destroyTexture" );
        }

        texture->notifyAllListenersTextureChanged( TextureGpuListener::Deleted );

        delete texture;
        mEntries.erase( itor );
    }
    //-----------------------------------------------------------------------------------
    StagingTexture* TextureGpuManager::getStagingTexture( uint32 width, uint32 height,
                                                          uint32 depth, uint32 slices,
                                                          PixelFormatGpu pixelFormat,
                                                          size_t minConsumptionRatioThreshold )
    {
        assert( minConsumptionRatioThreshold <= 100u && "Invalid consumptionRatioThreshold value!" );

#if OGRE_DEBUG_MEMORY_CONSUMPTION
        {
            char tmpBuffer[512];
            LwString text( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
            text.a( "TextureGpuManager::getStagingTexture: ", width, "x", height, "x" );
            text.a( depth, "x", slices, " ", PixelFormatGpuUtils::toString( pixelFormat ) );
            text.a( " (", (uint32)PixelFormatGpuUtils::getSizeBytes( width, height, depth,
                                                                     slices, pixelFormat, 4u ) /
                    1024u / 1024u,
                    " MB)");
            LogManager::getSingleton().logMessage( text.c_str() );
        }
#endif

        StagingTextureVec::iterator bestCandidate = mAvailableStagingTextures.end();
        StagingTextureVec::iterator itor = mAvailableStagingTextures.begin();
        StagingTextureVec::iterator end  = mAvailableStagingTextures.end();

        while( itor != end )
        {
            StagingTexture *stagingTexture = *itor;

            if( stagingTexture->supportsFormat( width, height, depth, slices, pixelFormat ) &&
                (bestCandidate == end || stagingTexture->isSmallerThan( *bestCandidate )) )
            {
                if( !stagingTexture->uploadWillStall() )
                    bestCandidate = itor;
            }

            ++itor;
        }

        StagingTexture *retVal = 0;

        if( bestCandidate != end && minConsumptionRatioThreshold != 0u )
        {
            const size_t requiredSize = PixelFormatGpuUtils::getSizeBytes( width, height, depth,
                                                                           slices, pixelFormat, 4u );
            const size_t ratio = (requiredSize * 100u) / (*bestCandidate)->_getSizeBytes();
            if( ratio < minConsumptionRatioThreshold )
                bestCandidate = end;
        }

        if( bestCandidate != end )
        {
            retVal = *bestCandidate;
            mUsedStagingTextures.push_back( *bestCandidate );
            mAvailableStagingTextures.erase( bestCandidate );
        }
        else
        {
            //Couldn't find an existing StagingTexture that could handle our request.
            //Check that our memory budget isn't exceeded.
            retVal = checkStagingTextureLimits( width, height, depth, slices, pixelFormat,
                                                minConsumptionRatioThreshold );
            if( !retVal )
            {
                //We haven't yet exceeded our budget, or we did exceed it and
                //checkStagingTextureLimits freed some memory. Either way, create a new one.
                retVal = createStagingTextureImpl( width, height, depth, slices, pixelFormat );
                mUsedStagingTextures.push_back( retVal );
            }
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::removeStagingTexture( StagingTexture *stagingTexture )
    {
        //Reverse search to speed up since most removals are
        //likely to remove what has just been requested.
        StagingTextureVec::reverse_iterator ritor = std::find( mUsedStagingTextures.rbegin(),
                                                               mUsedStagingTextures.rend(),
                                                               stagingTexture );
        assert( ritor != mUsedStagingTextures.rend() &&
                "StagingTexture does not belong to this TextureGpuManager or already removed" );

        StagingTextureVec::iterator itor = ritor.base() - 1u;
        efficientVectorRemove( mUsedStagingTextures, itor );

        mAvailableStagingTextures.push_back( stagingTexture );
    }
    //-----------------------------------------------------------------------------------
    AsyncTextureTicket* TextureGpuManager::createAsyncTextureTicket( uint32 width, uint32 height,
                                                                     uint32 depthOrSlices,
                                                                     TextureTypes::TextureTypes texType,
                                                                     PixelFormatGpu pixelFormatFamily )
    {
        pixelFormatFamily = PixelFormatGpuUtils::getFamily( pixelFormatFamily );
        AsyncTextureTicket *retVal = createAsyncTextureTicketImpl( width, height, depthOrSlices,
                                                                   texType, pixelFormatFamily );

        mAsyncTextureTickets.push_back( retVal );
        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAsyncTextureTicket( AsyncTextureTicket *ticket )
    {
        //Reverse search to speed up since most removals are
        //likely to remove what has just been requested.
        AsyncTextureTicketVec::reverse_iterator ritor = std::find( mAsyncTextureTickets.rbegin(),
                                                                   mAsyncTextureTickets.rend(), ticket );

        assert( ritor != mAsyncTextureTickets.rend() &&
                "AsyncTextureTicket does not belong to this TextureGpuManager or already removed" );

        OGRE_DELETE ticket;

        AsyncTextureTicketVec::iterator itor = ritor.base() - 1u;
        efficientVectorRemove( mAsyncTextureTickets, itor );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllAsyncTextureTicket(void)
    {
        AsyncTextureTicketVec::const_iterator itor = mAsyncTextureTickets.begin();
        AsyncTextureTicketVec::const_iterator end  = mAsyncTextureTickets.end();

        while( itor != end )
        {
            OGRE_DELETE *itor;
            ++itor;
        }

        mAsyncTextureTickets.clear();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::dumpStats(void) const
    {
        char tmpBuffer[512];
        LwString text( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );

        LogManager &logMgr = LogManager::getSingleton();

        const size_t availSizeBytes = getConsumedMemoryByStagingTextures( mAvailableStagingTextures );
        const size_t usedSizeBytes = getConsumedMemoryByStagingTextures( mUsedStagingTextures );

        text.clear();
        text.a( "Available Staging Textures\t|", (uint32)mAvailableStagingTextures.size(), "|",
                (uint32)availSizeBytes  / (1024u * 1024u), " MB\t\t |In use:\t|",
                (uint32)usedSizeBytes  / (1024u * 1024u), " MB");
        logMgr.logMessage( text.c_str() );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::dumpMemoryUsage( Log* log ) const
    {
        Log* logActual = log == NULL ? LogManager::getSingleton().getDefaultLog() : log;

        logActual->logMessage(
                    "==============================="
                    "Start dump of TextureGpuManager"
                    "===============================",
                    LML_CRITICAL );

        logActual->logMessage( "== Dumping Pools ==" );

        logActual->logMessage(
                    "||Width|Height|Format|Mipmaps|Size in bytes|"
                    "Num. active textures|Total texture capacity|Texture Names",
                    LML_CRITICAL );

        size_t bytesInPoolInclWaste = 0;
        size_t bytesInPoolExclWaste = 0;

        vector<char>::type tmpBuffer;
        tmpBuffer.resize( 512 * 1024 ); //512kb per line should be way more than enough
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
            text.a( pool.usedMemory, "|", pool.masterTexture->getDepthOrSlices() );

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
                    "RTT|UAV|Manual|MSAA Explicit|Reinterpretable|AutomaticBatched",
                    LML_CRITICAL );

        ResourceEntryMap::const_iterator itEntry = mEntries.begin();
        ResourceEntryMap::const_iterator enEntry = mEntries.end();

        while( itEntry != enEntry )
        {
            const ResourceEntry &entry = itEntry->second;
            text.clear();

            const size_t bytesTexture = entry.texture->getSizeBytes();

            text.a( "|", entry.texture->getNameStr().c_str() );
            text.a( "|", entry.texture->getRealResourceNameStr().c_str() );
            text.a( "|", entry.texture->getWidth(), "|", entry.texture->getHeight(), "|" );
            text.a( entry.texture->getDepth(), "|", entry.texture->getNumSlices(), "|" );
            text.a( PixelFormatGpuUtils::toString( entry.texture->getPixelFormat() ), "|",
                    entry.texture->getNumMipmaps(), "|" );
            text.a( entry.texture->getMsaa(), "|",
                    (uint32)bytesTexture, "|" );
            text.a( entry.texture->isRenderToTexture(), "|",
                    entry.texture->isUav(), "|",
                    entry.texture->hasMsaaExplicitResolves(), "|" );
            text.a( entry.texture->_isManualTextureFlagPresent(), "|",
                    entry.texture->isReinterpretable(), "|",
                    entry.texture->hasAutomaticBatching() );

            if( !entry.texture->hasAutomaticBatching() )
                bytesOutsidePool += bytesTexture;

            logActual->logMessage( text.c_str(), LML_CRITICAL );

            ++itEntry;
        }

        float fMBytesInPoolInclWaste = bytesInPoolInclWaste / (1024.0f * 1024.0f);
        float fMBytesInPoolExclWaste = bytesInPoolExclWaste / (1024.0f * 1024.0f);
        float fMBytesOutsidePool = bytesOutsidePool  / (1024.0f * 1024.0f);

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
        assert( listener );
        OGRE_DELETE mTextureGpuManagerListener;
        mTextureGpuManagerListener = listener;
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
        mMaxPreloadBytes = std::max( 1u, maxPreloadBytes );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::setWorkerThreadMinimumBudget( const BudgetEntryVec &budget )
    {
        mBudget = budget;
        //Sort in descending order.
        std::sort( mBudget.begin(), mBudget.end(), BudgetEntry() );
    }
    //-----------------------------------------------------------------------------------
    const String* TextureGpuManager::findAliasNameStr( IdString idName ) const
    {
        const String *retVal = 0;

        ResourceEntryMap::const_iterator itor = mEntries.find( idName );

        if( itor != mEntries.end() )
            retVal = &itor->second.alias;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    const String* TextureGpuManager::findResourceNameStr( IdString idName ) const
    {
        const String *retVal = 0;

        ResourceEntryMap::const_iterator itor = mEntries.find( idName );

        if( itor != mEntries.end() )
            retVal = &itor->second.name;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    RenderSystem* TextureGpuManager::getRenderSystem(void) const
    {
        return mRenderSystem;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::scheduleLoadRequest( TextureGpu *texture,
                                                 GpuResidency::GpuResidency nextResidency,
                                                 const String &name,
                                                 const String &resourceGroup,
                                                 uint32 filters,
                                                 uint32 sliceOrDepth )
    {
        Archive *archive = 0;
        ResourceLoadingListener *loadingListener = 0;
        if( resourceGroup != BLANKSTRING )
        {
            bool providedByListener = false;
            ResourceGroupManager &resourceGroupManager = ResourceGroupManager::getSingleton();
            loadingListener = resourceGroupManager.getLoadingListener();
            if( loadingListener )
            {
                if( !loadingListener->grouplessResourceExists( name ) )
                    providedByListener = true;
            }

            if( !providedByListener )
                archive = resourceGroupManager._getArchiveToResource( name, resourceGroup );
        }

        ThreadData &mainData = mThreadData[c_mainThread];
        mLoadRequestsMutex.lock();
            mainData.loadRequests.push_back( LoadRequest( name, archive, loadingListener, texture,
                                                          nextResidency, sliceOrDepth, filters ) );
        mLoadRequestsMutex.unlock();
        mWorkerWaitableEvent.wake();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_scheduleTransitionTo( TextureGpu *texture,
                                                   GpuResidency::GpuResidency nextResidency )
    {
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
            scheduleLoadRequest( texture, nextResidency, name, resourceGroup, filters );
        else
        {
            String baseName;
            String ext;
            String::size_type pos = name.find_last_of( '.' );
            if( pos != String::npos )
            {
                baseName    = name.substr( 0, pos );
                ext         = name.substr( pos + 1u );
            }
            else
            {
                baseName = name;
            }

            String lowercaseExt = ext;
            StringUtil::toLowerCase( lowercaseExt );

            if( lowercaseExt == "dds" )
            {
                // XX HACK there should be a better way to specify whether
                // all faces are in the same file or not
                scheduleLoadRequest( texture, nextResidency, name, resourceGroup, filters );
            }
            else
            {
                static const String suffixes[6] = { "_rt.", "_lf.", "_up.", "_dn.", "_fr.", "_bk." };

                for( int i=0; i<6; ++i )
                {
                    scheduleLoadRequest( texture, nextResidency, baseName + suffixes[i] + ext,
                                         resourceGroup, filters, i );
                }
            }
        }
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
    DefaultMipmapGen::DefaultMipmapGen TextureGpuManager::getDefaultMipmapGeneration(void) const
    {
        return mDefaultMipmapGen;
    }
    //-----------------------------------------------------------------------------------
    DefaultMipmapGen::DefaultMipmapGen TextureGpuManager::getDefaultMipmapGenerationCubemaps(void) const
    {
        return mDefaultMipmapGenCubemaps;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_reserveSlotForTexture( TextureGpu *texture )
    {
        bool matchFound = false;

        TexturePoolList::iterator itor = mTexturePool.begin();
        TexturePoolList::iterator end  = mTexturePool.end();

        while( itor != end && !matchFound )
        {
            const TexturePool &pool = *itor;

            matchFound =
                    pool.hasFreeSlot() &&
                    pool.masterTexture->getWidth() == texture->getWidth() &&
                    pool.masterTexture->getHeight() == texture->getHeight() &&
                    pool.masterTexture->getPixelFormat() == texture->getPixelFormat() &&
                    pool.masterTexture->getNumMipmaps() == texture->getNumMipmaps();

            TODO_grow_pool;

            if( !matchFound )
                ++itor;
        }

        if( itor == end )
        {
            IdType newId = Id::generateNewId<TextureGpuManager>();
            char tmpBuffer[64];
            LwString texName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
            texName.a( "_InternalTex", newId );

            TexturePool newPool;
            newPool.masterTexture = createTextureImpl( GpuPageOutStrategy::Discard,
                                                       texName.c_str(), 0,
                                                       TextureTypes::Type2DArray );
            const uint16 numSlices = mTextureGpuManagerListener->getNumSlicesFor( texture, this );

            newPool.usedMemory = 0;
            newPool.usedSlots.reserve( numSlices );

            newPool.masterTexture->setResolution( texture->getWidth(), texture->getHeight(), numSlices );
            newPool.masterTexture->setPixelFormat( texture->getPixelFormat() );
            newPool.masterTexture->setNumMipmaps( texture->getNumMipmaps() );

            mTexturePool.push_back( newPool );
            itor = --mTexturePool.end();

            itor->masterTexture->_transitionTo( GpuResidency::Resident, 0 );
        }

        uint16 sliceIdx = 0;
        //See if we can reuse a slot that was previously acquired and released
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
        texture->_notifyTextureSlotChanged( &(*itor), sliceIdx );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_releaseSlotFromTexture( TextureGpu *texture )
    {
        //const_cast? Yes. We own it. We could do a linear search to mTexturePool;
        //but it's O(N) vs O(1); and O(N) can quickly turn into O(N!).
        TexturePool *texturePool = const_cast<TexturePool*>( texture->getTexturePool() );
        TextureGpuVec::iterator itor = std::find( texturePool->usedSlots.begin(),
                                                  texturePool->usedSlots.end(), texture );
        assert( itor != texturePool->usedSlots.end() );
        efficientVectorRemove( texturePool->usedSlots, itor );

        const uint16 internalSliceStart = texture->getInternalSliceStart();
        if( texturePool->usedMemory == internalSliceStart + 1u )
            --texturePool->usedMemory;
        else
            texturePool->availableSlots.push_back( internalSliceStart );

        if( texturePool->empty() )
        {
            //Destroy the pool if it's no longer needed
            delete texturePool->masterTexture;
            TexturePoolList::iterator itPool = mTexturePool.begin();
            TexturePoolList::iterator enPool = mTexturePool.end();
            while( itPool != enPool && &(*itPool) != texturePool )
                ++itPool;
            mTexturePool.erase( itPool );
        }

        texture->_notifyTextureSlotChanged( 0, 0 );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::fulfillUsageStats(void)
    {
        UsageStatsVec::iterator itStats = mStreamingData.prevStats.begin();
        UsageStatsVec::iterator enStats = mStreamingData.prevStats.end();

        while( itStats != enStats )
        {
            --itStats->loopCount;
            if( itStats->loopCount == 0 )
            {
                //This record has been here too long without the worker thread touching it.
                //Remove it.
                itStats = efficientVectorRemove( mStreamingData.prevStats, itStats );
                enStats = mStreamingData.prevStats.end();
            }
            else
            {
                const uint32 rowAlignment = 4u;
                size_t oneSliceBytes = PixelFormatGpuUtils::getSizeBytes( itStats->width,
                                                                          itStats->height,
                                                                          1u, 1u, itStats->formatFamily,
                                                                          rowAlignment );

                //Round up.
                const size_t numSlices = (itStats->accumSizeBytes + oneSliceBytes - 1u) / oneSliceBytes;

                bool isSupported = false;

                StagingTextureVec::iterator itor = mStreamingData.availableStagingTex.begin();
                StagingTextureVec::iterator end  = mStreamingData.availableStagingTex.end();

                while( itor != end && !isSupported )
                {
                    //Check if the free StagingTextures can take the current usage load.
                    isSupported = (*itor)->supportsFormat( itStats->width, itStats->height, 1u,
                                                           numSlices, itStats->formatFamily );

                    if( isSupported )
                    {
                        mTmpAvailableStagingTex.push_back( *itor );
                        itor = mStreamingData.availableStagingTex.erase( itor );
                        end  = mStreamingData.availableStagingTex.end();
                    }
                    else
                    {
                        ++itor;
                    }
                }

                if( !isSupported )
                {
                    //It cannot. We need a bigger StagingTexture (or one that supports a specific format)
                    StagingTexture *newStagingTexture = getStagingTexture( itStats->width,
                                                                           itStats->height,
                                                                           1u, numSlices,
                                                                           itStats->formatFamily, 50u );
                    newStagingTexture->startMapRegion();
                    mTmpAvailableStagingTex.push_back( newStagingTexture );
                }

                ++itStats;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::fulfillMinimumBudget(void)
    {
        BudgetEntryVec::const_iterator itBudget = mBudget.begin();
        BudgetEntryVec::const_iterator enBudget = mBudget.end();

        while( itBudget != enBudget )
        {
            bool isSupported = false;

            StagingTextureVec::iterator itor = mStreamingData.availableStagingTex.begin();
            StagingTextureVec::iterator end  = mStreamingData.availableStagingTex.end();

            while( itor != end && !isSupported )
            {
                if( (*itor)->getFormatFamily() == itBudget->formatFamily )
                {
                    isSupported = (*itor)->supportsFormat( itBudget->minResolution,
                                                           itBudget->minResolution, 1u,
                                                           itBudget->minNumSlices,
                                                           itBudget->formatFamily );
                }

                if( isSupported )
                {
                    mTmpAvailableStagingTex.push_back( *itor );
                    itor = mStreamingData.availableStagingTex.erase( itor );
                    end  = mStreamingData.availableStagingTex.end();
                }
                else
                {
                    ++itor;
                }
            }

            //We now have to look in mTmpAvailableStagingTex in case fulfillUsageStats
            //already created a staging texture that fulfills the minimum budget.
            itor = mTmpAvailableStagingTex.begin();
            end  = mTmpAvailableStagingTex.end();

            while( itor != end && !isSupported )
            {
                if( (*itor)->getFormatFamily() == itBudget->formatFamily )
                {
                    isSupported = (*itor)->supportsFormat( itBudget->minResolution,
                                                           itBudget->minResolution, 1u,
                                                           itBudget->minNumSlices,
                                                           itBudget->formatFamily );
                }

                ++itor;
            }

            if( !isSupported )
            {
                StagingTexture *newStagingTexture = getStagingTexture( itBudget->minResolution,
                                                                       itBudget->minResolution, 1u,
                                                                       itBudget->minNumSlices,
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
    void TextureGpuManager::fullfillBudget(void)
    {
        //Ensure availableStagingTex is sorted in ascending order
        std::sort( mStreamingData.availableStagingTex.begin(),
                   mStreamingData.availableStagingTex.end(),
                   OrderByStagingTexture );

        fulfillUsageStats();
        fulfillMinimumBudget();

        {
            //The textures that are left are wasting memory, thus can be removed.
            StagingTextureVec::const_iterator itor = mStreamingData.availableStagingTex.begin();
            StagingTextureVec::const_iterator end  = mStreamingData.availableStagingTex.end();

            while( itor != end )
            {
                (*itor)->stopMapRegion();
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
    void TextureGpuManager::mergeUsageStatsIntoPrevStats(void)
    {
        UsageStatsVec::const_iterator itor = mStreamingData.usageStats.begin();
        UsageStatsVec::const_iterator end  = mStreamingData.usageStats.end();

        while( itor != end )
        {
            UsageStatsVec::iterator itPrev = mStreamingData.prevStats.begin();
            UsageStatsVec::iterator enPrev = mStreamingData.prevStats.end();

            while( itPrev != enPrev && itPrev->formatFamily != itor->formatFamily )
                ++itPrev;

            if( itPrev != enPrev )
            {
                //Average current stats with the previous one.
                //But if current one's was bigger, keep current.
                itPrev->width  = std::max( itor->width, (itPrev->width + itor->width) >> 1u );
                itPrev->height = std::max( itor->height, (itPrev->height + itor->height) >> 1u );
                itPrev->accumSizeBytes = std::max( itor->accumSizeBytes, (itPrev->accumSizeBytes +
                                                                          itor->accumSizeBytes) >> 1u );
                itPrev->loopCount = 15u;
            }
            else
            {
                mStreamingData.prevStats.push_back( *itor );
                mStreamingData.prevStats.back().loopCount = 15u;
            }

            ++itor;
        }

        mStreamingData.usageStats.clear();
    }
    //-----------------------------------------------------------------------------------
    TextureBox TextureGpuManager::getStreaming( ThreadData &workerData,
                                                StreamingData &streamingData,
                                                const TextureBox &box,
                                                PixelFormatGpu pixelFormat,
                                                StagingTexture **outStagingTexture )
    {
        //No need to check if streamingData.bytesPreloaded >= mMaxPreloadBytes because
        //our caller's caller already does that. Besides this is a static function.
        //This gives us slightly broader granularity control over memory consumption
        //(we may to try to preload all the mipmaps even if mMaxPreloadBytes is exceeded)
        TextureBox retVal;

        StagingTextureVec::iterator itor = workerData.usedStagingTex.begin();
        StagingTextureVec::iterator end  = workerData.usedStagingTex.end();

        while( itor != end && !retVal.data )
        {
            //supportsFormat will return false if it could never fit, or the format is not compatible.
            if( (*itor)->supportsFormat( box.width, box.height, box.depth, box.numSlices, pixelFormat ) )
            {
                retVal = (*itor)->mapRegion( box.width, box.height, box.depth,
                                             box.numSlices, pixelFormat );
                //retVal.data may be null if there's not enough free space (e.g. it's half empty).
                if( retVal.data )
                    *outStagingTexture = *itor;
            }

            ++itor;
        }

        itor = streamingData.availableStagingTex.begin();
        end  = streamingData.availableStagingTex.end();

        while( itor != end && !retVal.data )
        {
            if( (*itor)->supportsFormat( box.width, box.height, box.depth, box.numSlices, pixelFormat ) )
            {
                retVal = (*itor)->mapRegion( box.width, box.height, box.depth,
                                             box.numSlices, pixelFormat );
                if( retVal.data )
                {
                    *outStagingTexture = *itor;

                    //We need to move this to the 'used' textures
                    workerData.usedStagingTex.push_back( *itor );
                    itor = efficientVectorRemove( streamingData.availableStagingTex, itor );
                    end  = streamingData.availableStagingTex.end();
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

        //Keep track of requests so main thread knows our current workload.
        const PixelFormatGpu formatFamily = PixelFormatGpuUtils::getFamily( pixelFormat );
        UsageStatsVec::iterator itStats = streamingData.usageStats.begin();
        UsageStatsVec::iterator enStats = streamingData.usageStats.end();

        while( itStats != enStats && itStats->formatFamily != formatFamily )
            ++itStats;

        const uint32 rowAlignment = 4u;
        const size_t requiredBytes = PixelFormatGpuUtils::getSizeBytes( box.width, box.height,
                                                                        box.depth, box.numSlices,
                                                                        formatFamily,
                                                                        rowAlignment );

        if( itStats == enStats )
        {
            streamingData.usageStats.push_back( UsageStats( box.width, box.height,
                                                            box.getDepthOrSlices(),
                                                            formatFamily ) );
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
        Image2 &img = queuedImage.image;
        TextureGpu *texture = queuedImage.dstTexture;
        ObjCmdBuffer *commandBuffer = workerData.objCmdBuffer;

        const bool is3DVolume = img.getDepth() > 1u;

        const uint8 firstMip = queuedImage.getMinMipLevel();
        const uint8 numMips = queuedImage.getMaxMipLevelPlusOne();

        for( uint8 i=firstMip; i<numMips; ++i )
        {
            for( uint32 z=0; z<img.getDepthOrSlices(); ++z )
            {
                TextureBox srcBox = img.getData( i );
                if( queuedImage.isMipSliceQueued( i, z ) )
                {
                    srcBox.z            = is3DVolume ? z : 0;
                    srcBox.sliceStart   = is3DVolume ? 0 : z;
                    srcBox.depth        = 1u;
                    srcBox.numSlices    = 1u;

                    StagingTexture *stagingTexture = 0;
                    TextureBox dstBox = getStreaming( workerData, streamingData, srcBox,
                                                      img.getPixelFormat(), &stagingTexture );
                    if( dstBox.data )
                    {
                        //Upload to staging area. CPU -> GPU
                        dstBox.copyFrom( srcBox );
                        if( queuedImage.dstSliceOrDepth != std::numeric_limits<uint32>::max() )
                        {
                            if( !is3DVolume )
                                srcBox.sliceStart += queuedImage.dstSliceOrDepth;
                            else
                                srcBox.z += queuedImage.dstSliceOrDepth;
                        }

                        //Schedule a command to copy from staging to final texture, GPU -> GPU
                        ObjCmdBuffer::UploadFromStagingTex *uploadCmd = commandBuffer->addCommand<
                                                                        ObjCmdBuffer::UploadFromStagingTex>();
                        new (uploadCmd) ObjCmdBuffer::UploadFromStagingTex( stagingTexture, dstBox,
                                                                            texture, srcBox, i );
                        //This mip has been processed, flag it as done.
                        queuedImage.unqueueMipSlice( i, z );
                    }
                }
            }
        }

        if( queuedImage.empty() )
        {
            ObjCmdBuffer::NotifyDataIsReady *cmd =
                    commandBuffer->addCommand<ObjCmdBuffer::NotifyDataIsReady>();
            new (cmd) ObjCmdBuffer::NotifyDataIsReady( texture, queuedImage.filters );
            //We don't restore bytesPreloaded because it gets reset to 0 by worker thread.
            //Doing so could increase throughput of data we can preload. However it can
            //cause a positive feedback effect where limits don't get respected at all
            //(it keeps preloading more and more)
            //if( streamingData.bytesPreloaded >= queuedImage.image.getSizeBytes() )
            //    streamingData.bytesPreloaded -= queuedImage.image.getSizeBytes();
            queuedImage.destroy();
        }
    }
    //-----------------------------------------------------------------------------------
    unsigned long updateStreamingWorkerThread( ThreadHandle *threadHandle )
    {
        TextureGpuManager *textureManager =
                reinterpret_cast<TextureGpuManager*>( threadHandle->getUserParam() );
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
    void TextureGpuManager::_updateStreaming(void)
    {
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

        ThreadData &workerData  = mThreadData[c_workerThread];
        ThreadData &mainData    = mThreadData[c_mainThread];

        mLoadRequestsMutex.lock();
            //Lock while inside mMutex because _update has access to our
            //workerData.loadRequests. We still need mLoadRequestsMutex
            //to keep our access to mainData.loadRequests as short as possible
            //(we don't want to block the main thread for long).
            if( workerData.loadRequests.empty() )
            {
                workerData.loadRequests.swap( mainData.loadRequests );
            }
            else
            {
                workerData.loadRequests.insert( workerData.loadRequests.end(),
                                                mainData.loadRequests.begin(),
                                                mainData.loadRequests.end() );
                mainData.loadRequests.clear();
            }
        mLoadRequestsMutex.unlock();

        ObjCmdBuffer *commandBuffer = workerData.objCmdBuffer;

        const bool processedAnyImage = !workerData.loadRequests.empty() ||
                                       !mStreamingData.queuedImages.empty();

        //First, try to upload the queued images that failed in the previous iteration.
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
        //Now process new requests from main thread
        LoadRequestVec::const_iterator itor = workerData.loadRequests.begin();
        LoadRequestVec::const_iterator end  = workerData.loadRequests.end();

        while( itor != end &&
               entriesProcessed < entriesToProcessPerIteration &&
               mStreamingData.bytesPreloaded < mMaxPreloadBytes )
        {
            const LoadRequest &loadRequest = *itor;

            if( !loadRequest.archive && !loadRequest.loadingListener )
            {
                LogManager::getSingleton().logMessage(
                            "ERROR: Did you call createTexture with a valid resourceGroup? "
                            "Texture: " + loadRequest.name, LML_CRITICAL );
            }

            DataStreamPtr data;
            if( !loadRequest.archive )
                data = loadRequest.loadingListener->grouplessResourceLoading( loadRequest.name );
            else
            {
                data = loadRequest.archive->open( loadRequest.name );
                if( loadRequest.loadingListener )
                {
                    loadRequest.loadingListener->grouplessResourceOpened( loadRequest.name,
                                                                          loadRequest.archive, data );
                }
            }

            {
                //Load the image from file into system RAM
                Image2 img;
                img.load( data );

                FilterBaseArray filters;
                TextureFilter::FilterBase::createFilters( loadRequest.filters, filters,
                                                          loadRequest.texture );

                if( loadRequest.sliceOrDepth == std::numeric_limits<uint32>::max() ||
                    loadRequest.sliceOrDepth == 0 )
                {
                    loadRequest.texture->setResolution( img.getWidth(), img.getHeight(),
                                                        img.getDepthOrSlices() );
                    if( loadRequest.sliceOrDepth == std::numeric_limits<uint32>::max() )
                    {
                        //If the texture had already been set it to 1D
                        //and it is viable, then keep the 1D setting.
                        if( img.getHeight() != 1u ||
                            loadRequest.texture->getTextureType() != TextureTypes::Type1D )
                        {
                            loadRequest.texture->setTextureType( img.getTextureType() );
                        }
                    }
                    loadRequest.texture->setPixelFormat( img.getPixelFormat() );
                    loadRequest.texture->setNumMipmaps( img.getNumMipmaps() );

                    FilterBaseArray::const_iterator itFilters = filters.begin();
                    FilterBaseArray::const_iterator enFilters = filters.end();
                    while( itFilters != enFilters )
                    {
                        (*itFilters)->_executeStreaming( img, loadRequest.texture );
                        ++itFilters;
                    }

                    void *sysRamCopy = 0;
                    if( loadRequest.texture->getGpuPageOutStrategy() ==
                            GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
                    {
                        assert( img.getTextureType() == loadRequest.texture->getTextureType() &&
                                "Cannot load cubemaps partially file by file into each face"
                                "when using GpuPageOutStrategy::AlwaysKeepSystemRamCopy" );
                        sysRamCopy = img.getData(0).data;
                    }

                    //We have enough to transition the texture to Resident.
                    ObjCmdBuffer::TransitionToResident *transitionCmd = commandBuffer->addCommand<
                            ObjCmdBuffer::TransitionToResident>();
                    new (transitionCmd) ObjCmdBuffer::TransitionToResident( loadRequest.texture,
                                                                            sysRamCopy );
                }
                else
                {
                    FilterBaseArray::const_iterator itFilters = filters.begin();
                    FilterBaseArray::const_iterator enFilters = filters.end();
                    while( itFilters != enFilters )
                    {
                        (*itFilters)->_executeStreaming( img, loadRequest.texture );
                        ++itFilters;
                    }
                }

                //Queue the image for upload to GPU.
                mStreamingData.queuedImages.push_back( QueuedImage( img, img.getNumMipmaps(),
                                                                    img.getDepthOrSlices(),
                                                                    loadRequest.texture,
                                                                    loadRequest.sliceOrDepth,
                                                                    filters ) );
            }

            //Try to upload the queued image right now (all of its mipmaps).
            processQueuedImage( mStreamingData.queuedImages.back(), workerData, mStreamingData );

            if( mStreamingData.queuedImages.back().empty() )
                mStreamingData.queuedImages.pop_back();

            ++entriesProcessed;
            ++itor;
        }

        //Two cases:
        //  1. We did something this iteration, and finished 100%.
        //     Main thread could be waiting for us. Let them know.
        //  2. We couldn't do everything in this iteration, which means
        //     we need something from main thread. Wake it up.
        //Note that normally main thread isn't sleeping, but it could be if
        //waitForStreamingCompletion was called.
        bool wakeUpMainThread = false;
        if( (processedAnyImage && mStreamingData.queuedImages.empty()) ||
            !mStreamingData.queuedImages.empty() )
        {
            wakeUpMainThread = true;
        }

        workerData.loadRequests.erase( workerData.loadRequests.begin(),
                                       workerData.loadRequests.begin() + entriesProcessed );
        mergeUsageStatsIntoPrevStats();
        mMutex.unlock();

        //Wake up outside mMutex to avoid unnecessary contention.
        if( wakeUpMainThread )
            mRequestToMainThreadEvent.wake();
    }
    //-----------------------------------------------------------------------------------
    size_t TextureGpuManager::getConsumedMemoryByStagingTextures(
            const StagingTextureVec &stagingTextures ) const
    {
        size_t totalSizeBytes = 0;
        StagingTextureVec::const_iterator itor = stagingTextures.begin();
        StagingTextureVec::const_iterator end  = stagingTextures.end();
        while( itor != end )
        {
            totalSizeBytes += (*itor)->_getSizeBytes();
            ++itor;
        }

        return totalSizeBytes;
    }
    //-----------------------------------------------------------------------------------
    StagingTexture* TextureGpuManager::checkStagingTextureLimits( uint32 width, uint32 height,
                                                                  uint32 depth, uint32 slices,
                                                                  PixelFormatGpu pixelFormat,
                                                                  size_t minConsumptionRatioThreshold )
    {
        const size_t requiredSize = PixelFormatGpuUtils::getSizeBytes( width, height, depth, slices,
                                                                       pixelFormat, 4u );

        size_t consumedBytes = getConsumedMemoryByStagingTextures( mAvailableStagingTextures );

        if( consumedBytes + requiredSize < mStagingTextureMaxBudgetBytes )
            return 0; //We are OK, below limits

        LogManager::getSingleton().logMessage( "Texture memory budget exceeded. Stalling GPU." );

        set<uint32>::type waitedFrames;

        //Before freeing memory, check if we can make some of
        //the existing staging textures available for use.
        StagingTextureVec::iterator bestCandidate = mAvailableStagingTextures.end();
        StagingTextureVec::iterator itor = mAvailableStagingTextures.begin();
        StagingTextureVec::iterator end  = mAvailableStagingTextures.end();
        while( itor != end && bestCandidate == end )
        {
            StagingTexture *stagingTexture = *itor;
            const uint32 frameUsed = stagingTexture->getLastFrameUsed();

            if( waitedFrames.find( frameUsed ) == waitedFrames.end() )
            {
                mVaoManager->waitForSpecificFrameToFinish( frameUsed );
                waitedFrames.insert( frameUsed );
            }

            if( stagingTexture->supportsFormat( width, height, depth, slices, pixelFormat ) &&
                (bestCandidate == end || stagingTexture->isSmallerThan( *bestCandidate )) )
            {
                const size_t ratio = (requiredSize * 100u) / (*itor)->_getSizeBytes();
                if( ratio >= minConsumptionRatioThreshold )
                    bestCandidate = itor;
            }

            ++itor;
        }

        StagingTexture *retVal = 0;

        if( bestCandidate == end )
        {
            LogManager::getSingleton().logMessage( "Stalling was not enough. Freeing memory." );

            //Could not find any best candidate even after stalling.
            //Start deleting staging textures until we've freed enough space.
            itor = mAvailableStagingTextures.begin();
            while( itor != end && (consumedBytes + requiredSize > mStagingTextureMaxBudgetBytes) )
            {
                consumedBytes -= (*itor)->_getSizeBytes();
                destroyStagingTextureImpl( *itor );
                delete *itor;
                ++itor;
            }

            mAvailableStagingTextures.erase( mAvailableStagingTextures.begin(), itor );
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
    bool TextureGpuManager::_update( bool syncWithWorkerThread )
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN || OGRE_FORCE_TEXTURE_STREAMING_ON_MAIN_THREAD
        _updateStreaming();
#endif
        bool isDone = false;

        ThreadData &mainData = mThreadData[c_mainThread];
        {
            ThreadData &workerData = mThreadData[c_workerThread];
            bool lockSucceeded = false;

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
                std::swap( mainData.objCmdBuffer, workerData.objCmdBuffer );
                mainData.usedStagingTex.swap( workerData.usedStagingTex );
                fullfillBudget();

                isDone = mainData.loadRequests.empty() &&
                         workerData.loadRequests.empty() &&
                         mStreamingData.queuedImages.empty();
                mMutex.unlock();
            }
        }

        {
            StagingTextureVec::const_iterator itor = mainData.usedStagingTex.begin();
            StagingTextureVec::const_iterator end  = mainData.usedStagingTex.end();

            while( itor != end )
            {
                (*itor)->stopMapRegion();
                ++itor;
            }
        }

        {
            StagingTextureVec::iterator itor = mAvailableStagingTextures.begin();
            StagingTextureVec::iterator end  = mAvailableStagingTextures.end();

            const uint32 numFramesThreshold = mVaoManager->getDynamicBufferMultiplier() + 2u;

            //They're kept in order.
            while( itor != end &&
                   mVaoManager->getFrameCount() - (*itor)->getLastFrameUsed() > numFramesThreshold )
            {
                destroyStagingTextureImpl( *itor );
                delete *itor;
                ++itor;
            }

            mAvailableStagingTextures.erase( mAvailableStagingTextures.begin(), itor );
        }

        mainData.objCmdBuffer->execute();
        mainData.objCmdBuffer->clear();

        {
            StagingTextureVec::const_iterator itor = mainData.usedStagingTex.begin();
            StagingTextureVec::const_iterator end  = mainData.usedStagingTex.end();

            while( itor != end )
            {
                removeStagingTexture( *itor );
                ++itor;
            }

            mainData.usedStagingTex.clear();
        }

        mWorkerWaitableEvent.wake();

#if OGRE_DEBUG_MODE && 0
        dumpStats();
#endif

        return isDone;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::waitForStreamingCompletion(void)
    {
        bool bDone = false;
        while( !bDone )
        {
            bDone = _update( true );
            if( !bDone )
            {
                mVaoManager->_update();
                mRequestToMainThreadEvent.wait();
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
            bDone = _update( true );
            if( !bDone )
            {
                if( texture->isDataReady() )
                    bDone = true;
                else if( metadataOnly && texture->isMetadataReady() )
                    bDone = true;

                if( !bDone )
                {
                    mVaoManager->_update();
                    mRequestToMainThreadEvent.wait();
                }
            }
        }
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    bool TexturePool::hasFreeSlot(void) const
    {
        return !availableSlots.empty() || usedMemory < masterTexture->getNumSlices();
    }
    //-----------------------------------------------------------------------------------
    bool TexturePool::empty(void) const
    {
        const size_t numSlices = masterTexture->getNumSlices();
        return (availableSlots.size() + (numSlices - usedMemory)) == numSlices;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    TextureGpuManager::UsageStats::UsageStats( uint32 _width, uint32 _height, uint32 _depthOrSlices,
                                                   PixelFormatGpu _formatFamily ) :
        width( _width ),
        height( _height ),
        formatFamily( _formatFamily ),
        accumSizeBytes( PixelFormatGpuUtils::getSizeBytes( _width, _height, _depthOrSlices,
                                                           1u, _formatFamily, 4u ) ),
        loopCount( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    TextureGpuManager::QueuedImage::QueuedImage( Image2 &srcImage, uint8 numMips, uint8 _numSlices,
                                                 TextureGpu *_dstTexture, uint32 _dstSliceOrDepth,
                                                 FilterBaseArray &inOutFilters ) :
        dstTexture( _dstTexture ),
        numSlices( _numSlices ),
        dstSliceOrDepth( _dstSliceOrDepth )
    {
        assert( numSlices >= 1u );

        filters.swap( inOutFilters );

        //Prevent destroying the internal data in srcImage if QueuedImageVec
        //holding us gets resized. (we do not follow the rule of 3)
        srcImage._setAutoDelete( false );
        image = srcImage;

        uint32 numMipSlices = numMips * numSlices;

        assert( numMipSlices < 256u );

        for( int i=0; i<4; ++i )
        {
            if( numMipSlices >= 64u )
            {
                mipLevelBitSet[i] = 0xffffffffffffffff;
                numMipSlices -= 64u;
            }
            else
            {
                mipLevelBitSet[i] = (1ul << numMipSlices) - 1ul;
                numMipSlices = 0;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::QueuedImage::destroy(void)
    {
        if( dstTexture->getGpuPageOutStrategy() != GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
        {
            //Do not delete the internal pointer if the TextureGpu will be owning it.
            image._setAutoDelete( true );
            image.freeMemory();
        }

        assert( filters.empty() &&
                "Internal Error: Failed to send filters to the main thread for destruction. "
                "These filters will leak" );
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::QueuedImage::empty(void) const
    {
        return  mipLevelBitSet[0] == 0ul && mipLevelBitSet[1] == 0ul &&
                mipLevelBitSet[2] == 0ul && mipLevelBitSet[3] == 0ul;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::QueuedImage::isMipSliceQueued( uint8 mipLevel, uint8 slice ) const
    {
        uint32 mipSlice = mipLevel * numSlices + slice;
        size_t idx  = mipSlice / 64u;
        uint64 mask = mipSlice % 64u;
        mask = ((uint64)1ul) << mask;
        return (mipLevelBitSet[idx] & mask) != 0;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::QueuedImage::unqueueMipSlice( uint8 mipLevel, uint8 slice )
    {
        uint32 mipSlice = mipLevel * numSlices + slice;
        size_t idx  = mipSlice / 64u;
        uint64 mask = mipSlice % 64u;
        mask = ((uint64)1ul) << mask;
        mipLevelBitSet[idx] = mipLevelBitSet[idx] & ~mask;
    }
    //-----------------------------------------------------------------------------------
    uint8 TextureGpuManager::QueuedImage::getMinMipLevel(void) const
    {
        for( size_t i=0; i<4u; ++i )
        {
            if( mipLevelBitSet[i] != 0u )
            {
                uint8 firstBitSet = static_cast<uint8>( ctz64( mipLevelBitSet[i] ) );
                return (firstBitSet + 64u * i) / numSlices;
            }
        }

        return 255u;
    }
    //-----------------------------------------------------------------------------------
    uint8 TextureGpuManager::QueuedImage::getMaxMipLevelPlusOne(void) const
    {
        for( size_t i=4u; i--; )
        {
            if( mipLevelBitSet[i] != 0u )
            {
                uint8 lastBitSet = static_cast<uint8>( 64u - clz64( mipLevelBitSet[i] ) + 64u * i );
                return (lastBitSet + numSlices - 1u) / numSlices;
            }
        }

        return 0u;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::BudgetEntry::operator () ( const BudgetEntry &_l,
                                                       const BudgetEntry &_r ) const
    {
        //Biggest ones come first
        const size_t lSize = PixelFormatGpuUtils::getSizeBytes( _l.minResolution, _l.minResolution, 1u,
                                                                _l.minNumSlices, _l.formatFamily, 4u );
        const size_t rSize = PixelFormatGpuUtils::getSizeBytes( _r.minResolution, _r.minResolution, 1u,
                                                                _r.minNumSlices, _r.formatFamily, 4u );
        return lSize > rSize;
    }
}
