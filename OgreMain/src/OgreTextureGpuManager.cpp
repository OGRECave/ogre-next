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

#include "OgreTextureGpuManager.h"
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

    TextureGpuManager::TextureGpuManager( VaoManager *vaoManager ) :
        mShuttingDown( false ),
        mEntriesToProcessPerIteration( 3u ),
        mVaoManager( vaoManager )
    {
        //64MB default
        mDefaultPoolParameters.maxBytesPerPool = 64 * 1024 * 1024;
        mDefaultPoolParameters.minSlicesPerPool[0] = 16;
        mDefaultPoolParameters.minSlicesPerPool[1] = 8;
        mDefaultPoolParameters.minSlicesPerPool[2] = 4;
        mDefaultPoolParameters.minSlicesPerPool[3] = 2;
        mDefaultPoolParameters.maxResolutionToApplyMinSlices[0] = 256;
        mDefaultPoolParameters.maxResolutionToApplyMinSlices[1] = 512;
        mDefaultPoolParameters.maxResolutionToApplyMinSlices[2] = 1024;
        mDefaultPoolParameters.maxResolutionToApplyMinSlices[3] = 4096;

        PixelFormatGpu format;
        //64MB for RGBA8, that's one 4096x4096 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_RGBA8_UNORM );
        mDefaultPoolParameters.budget.push_back( BudgetEntry( format, 4096u, 1u ) );
        //16MB for BC1, that's two 4096x4096 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_BC1_UNORM );
        mDefaultPoolParameters.budget.push_back( BudgetEntry( format, 4096u, 2u ) );
        //16MB for BC3, that's one 4096x4096 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_BC3_UNORM );
        mDefaultPoolParameters.budget.push_back( BudgetEntry( format, 4096u, 1u ) );
        //16MB for BC5, that's one 4096x4096 texture.
        format = PixelFormatGpuUtils::getFamily( PFG_BC5_UNORM );
        mDefaultPoolParameters.budget.push_back( BudgetEntry( format, 4096u, 1u ) );

        //Sort in descending order.
        std::sort( mDefaultPoolParameters.budget.begin(), mDefaultPoolParameters.budget.end(),
                   BudgetEntry() );

        for( int i=0; i<2; ++i )
            mThreadData[i].objCmdBuffer = new ObjCmdBuffer();

#if OGRE_PLATFORM != OGRE_PLATFORM_EMSCRIPTEN
        mWorkerThread = Threads::CreateThread( THREAD_GET( updateStreamingWorkerThread ), 0, this );
#endif
    }
    //-----------------------------------------------------------------------------------
    TextureGpuManager::~TextureGpuManager()
    {
        mShuttingDown = true;
#if OGRE_PLATFORM != OGRE_PLATFORM_EMSCRIPTEN
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
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAll(void)
    {
        waitForStreamingCompletion();

        destroyAllStagingBuffers();
        destroyAllAsyncTextureTicket();
        destroyAllTextures();
        destroyAllPools();
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::destroyAllStagingBuffers(void)
    {
        StagingTextureVec::iterator itor = mAvailableStagingTextures.begin();
        StagingTextureVec::iterator end  = mAvailableStagingTextures.end();

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
    uint16 TextureGpuManager::getNumSlicesFor( TextureGpu *texture ) const
    {
        const PoolParameters &poolParams = mDefaultPoolParameters;

        uint32 maxResolution = std::max( texture->getWidth(), texture->getHeight() );
        uint16 minSlicesPerPool = 1u;

        for( int i=0; i<4; ++i )
        {
            if( maxResolution <= poolParams.maxResolutionToApplyMinSlices[i] )
            {
                minSlicesPerPool = poolParams.minSlicesPerPool[i];
                break;
            }
        }

        return minSlicesPerPool;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* TextureGpuManager::createTexture( const String &name,
                                                  GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                                  uint32 textureFlags,
                                                  TextureTypes::TextureTypes initialType,
                                                  const String &resourceGroup )
    {
        IdString idName( name );

        if( mEntries.find( idName ) != mEntries.end() )
        {
            OGRE_EXCEPT( Exception::ERR_DUPLICATE_ITEM,
                         "A texture with name '" + name + "' already exists.",
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

        mEntries[idName] = ResourceEntry( name, resourceGroup, retVal );

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* TextureGpuManager::createOrRetrieveTexture(
            const String &name, GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
            uint32 textureFlags, TextureTypes::TextureTypes initialType, const String &resourceGroup )
    {
        TextureGpu *retVal = 0;

        IdString idName( name );
        ResourceEntryMap::const_iterator itor = mEntries.find( idName );
        if( itor != mEntries.end() )
            retVal = itor->second.texture;
        else
            retVal = createTexture( name, pageOutStrategy, textureFlags, initialType, resourceGroup );

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
                {
                    bestCandidate = itor;
                    mUsedStagingTextures.push_back( stagingTexture );
                    const size_t idx = itor - mAvailableStagingTextures.begin();
                    mAvailableStagingTextures.erase( itor );
                    itor = mAvailableStagingTextures.begin() + idx;
                    end  = mAvailableStagingTextures.end();
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
            //Couldn't find an existing StagingTexture that could handle our request. Create one.
            retVal = createStagingTextureImpl( width, height, depth, slices, pixelFormat );
            mUsedStagingTextures.push_back( retVal );
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
    const String* TextureGpuManager::findNameStr( IdString idName ) const
    {
        const String *retVal = 0;

        ResourceEntryMap::const_iterator itor = mEntries.find( idName );

        if( itor != mEntries.end() )
            retVal = &itor->second.name;

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::_scheduleTransitionTo( TextureGpu *texture,
                                                   GpuResidency::GpuResidency nextResidency )
    {
        String name, resourceGroup;
        ResourceEntryMap::const_iterator itor = mEntries.find( texture->getName() );
        if( itor != mEntries.end() )
        {
            name = itor->second.name;
            resourceGroup = itor->second.resourceGroup;
        }

        Archive *archive = 0;
        if( resourceGroup != BLANKSTRING )
        {
            ResourceGroupManager &resourceGroupManager = ResourceGroupManager::getSingleton();
            archive = resourceGroupManager._getArchiveToResource( name, resourceGroup );
        }

        ThreadData &mainData = mThreadData[c_mainThread];
        mLoadRequestsMutex.lock();
            mainData.loadRequests.push_back( LoadRequest( name, archive, texture, nextResidency ) );
        mLoadRequestsMutex.unlock();
        mWorkerWaitableEvent.wake();
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
                    pool.masterTexture->getDepthOrSlices() == texture->getDepthOrSlices() &&
                    pool.masterTexture->getPixelFormat() == texture->getPixelFormat() &&
                    pool.masterTexture->getNumMipmaps() == texture->getNumMipmaps();

            TODO_grow_pool;

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
            const uint16 numSlices = getNumSlicesFor( texture );

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

        texture->_notifyTextureSlotChanged( 0, 0 );
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::fullfillUsageStats( ThreadData &workerData )
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
    void TextureGpuManager::fullfillMinimumBudget( ThreadData &workerData,
                                                   const PoolParameters &poolParams )
    {
        BudgetEntryVec::const_iterator itBudget = poolParams.budget.begin();
        BudgetEntryVec::const_iterator enBudget = poolParams.budget.end();

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

            //We now have to look in mTmpAvailableStagingTex in case fullfillUsageStats
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
    void TextureGpuManager::fullfillBudget( ThreadData &workerData )
    {
        //Ensure availableStagingTex is sorted in ascending order
        std::sort( mStreamingData.availableStagingTex.begin(),
                   mStreamingData.availableStagingTex.end(),
                   OrderByStagingTexture );

        fullfillUsageStats( workerData );
        fullfillMinimumBudget( workerData, mDefaultPoolParameters );

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

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::processQueuedImage( QueuedImage &queuedImage, ThreadData &workerData,
                                                StreamingData &streamingData )
    {
        Image2 &img = queuedImage.image;
        TextureGpu *texture = queuedImage.dstTexture;
        ObjCmdBuffer *commandBuffer = workerData.objCmdBuffer;

        const uint8 firstMip = queuedImage.getMinMipLevel();
        const uint8 numMips = queuedImage.getMaxMipLevelPlusOne();

        for( uint8 i=firstMip; i<numMips; ++i )
        {
            if( queuedImage.isMipQueued( i ) )
            {
                TextureBox srcBox = img.getData( i );
                StagingTexture *stagingTexture = 0;
                TextureBox dstBox = getStreaming( workerData, streamingData, srcBox,
                                                  img.getPixelFormat(), &stagingTexture );
                if( dstBox.data )
                {
                    //Upload to staging area. CPU -> GPU
                    dstBox.copyFrom( srcBox );

                    //Schedule a command to copy from staging to final texture, GPU -> GPU
                    ObjCmdBuffer::UploadFromStagingTex *uploadCmd = commandBuffer->addCommand<
                            ObjCmdBuffer::UploadFromStagingTex>();
                    new (uploadCmd) ObjCmdBuffer::UploadFromStagingTex( stagingTexture, dstBox,
                                                                        texture, i );
                    //This mip has been processed, flag it as done.
                    queuedImage.unqueueMip( i );
                }
            }
        }

        if( queuedImage.empty() )
        {
            queuedImage.destroy();
            ObjCmdBuffer::NotifyDataIsReady *cmd =
                    commandBuffer->addCommand<ObjCmdBuffer::NotifyDataIsReady>();
            new (cmd) ObjCmdBuffer::NotifyDataIsReady( texture );
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

        while( itQueue != enQueue )
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

        while( itor != end && entriesProcessed < entriesToProcessPerIteration )
        {
            const LoadRequest &loadRequest = *itor;

            if( !loadRequest.archive )
            {
                LogManager::getSingleton().logMessage(
                            "ERROR: Did you call createTexture with a valid resourceGroup? "
                            "Texture: " + loadRequest.name, LML_CRITICAL );
            }

            DataStreamPtr data = loadRequest.archive->open( loadRequest.name );
            {
                //Load the image from file into system RAM
                Image2 img;
                img.load( data );

                loadRequest.texture->setResolution( img.getWidth(), img.getHeight(),
                                                    img.getDepthOrSlices() );
                loadRequest.texture->setTextureType( img.getTextureType() );
                loadRequest.texture->setPixelFormat( img.getPixelFormat() );
                loadRequest.texture->setNumMipmaps( img.getNumMipmaps() );

                void *sysRamCopy = 0;
                if( loadRequest.texture->getGpuPageOutStrategy() ==
                        GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
                {
                    sysRamCopy = img.getData(0).data;
                }

                //We have enough to transition the texture to Resident.
                ObjCmdBuffer::TransitionToResident *transitionCmd = commandBuffer->addCommand<
                        ObjCmdBuffer::TransitionToResident>();
                new (transitionCmd) ObjCmdBuffer::TransitionToResident( loadRequest.texture,
                                                                        sysRamCopy );

                //Queue the image for upload to GPU.
                mStreamingData.queuedImages.push_back( QueuedImage( img, img.getNumMipmaps(),
                                                                    loadRequest.texture ) );
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
    bool TextureGpuManager::_update( bool syncWithWorkerThread )
    {
#if OGRE_PLATFORM == OGRE_PLATFORM_EMSCRIPTEN
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
                fullfillBudget( workerData );

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
                   (*itor)->getLastFrameUsed() - mVaoManager->getFrameCount() > numFramesThreshold )
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
                mRequestToMainThreadEvent.wait();
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
                    mRequestToMainThreadEvent.wait();
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
    TextureGpuManager::QueuedImage::QueuedImage( Image2 &srcImage, uint8 numMips,
                                                 TextureGpu *_dstTexture ) :
        dstTexture( _dstTexture )
    {
        srcImage._setAutoDelete( false );
        image = srcImage;

        for( int i=0; i<4; ++i )
        {
            if( numMips >= 64u )
            {
                mipLevelBitSet[i] = 0xffffffffffffffff;
                numMips -= 64u;
            }
            else
            {
                mipLevelBitSet[i] = (1ul << numMips) - 1ul;
                numMips = 0;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::QueuedImage::destroy(void)
    {
        if( dstTexture->getGpuPageOutStrategy() != GpuPageOutStrategy::AlwaysKeepSystemRamCopy )
        {
            image._setAutoDelete( true );
            image.freeMemory();
        }
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::QueuedImage::empty(void) const
    {
        return  mipLevelBitSet[0] == 0ul && mipLevelBitSet[1] == 0ul &&
                mipLevelBitSet[2] == 0ul && mipLevelBitSet[3] == 0ul;
    }
    //-----------------------------------------------------------------------------------
    bool TextureGpuManager::QueuedImage::isMipQueued( uint8 mipLevel ) const
    {
        size_t idx  = mipLevel / 64u;
        uint64 mask = mipLevel % 64u;
        mask = ((uint64)1ul) << mask;
        return (mipLevelBitSet[idx] & mask) != 0;
    }
    //-----------------------------------------------------------------------------------
    void TextureGpuManager::QueuedImage::unqueueMip( uint8 mipLevel )
    {
        size_t idx  = mipLevel / 64u;
        uint64 mask = mipLevel % 64u;
        mask = ((uint64)1ul) << mask;
        mipLevelBitSet[idx] = mipLevelBitSet[idx] & ~mask;
    }
    //-----------------------------------------------------------------------------------
    uint8 TextureGpuManager::QueuedImage::getMinMipLevel(void) const
    {
        for( size_t i=0; i<4u; ++i )
        {
            if( mipLevelBitSet[i] != 0u )
                return static_cast<uint8>( ctz64( mipLevelBitSet[i] ) );
        }

        return 255u;
    }
    //-----------------------------------------------------------------------------------
    uint8 TextureGpuManager::QueuedImage::getMaxMipLevelPlusOne(void) const
    {
        for( size_t i=4u; i--; )
        {
            if( mipLevelBitSet[i] != 0u )
                return static_cast<uint8>( 64u - clz64( mipLevelBitSet[i] ) + 64u * i );
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
