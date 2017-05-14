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

#ifndef _OgreTextureGpuManager_H_
#define _OgreTextureGpuManager_H_

#include "OgrePrerequisites.h"
#include "OgreTextureGpu.h"
#include "OgreImage2.h"
#include "Threading/OgreLightweightMutex.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

    typedef vector<TextureGpu*>::type TextureGpuVec;
    class ObjCmdBuffer;

    struct _OgreExport TexturePool
    {
        TextureGpu  *masterTexture;
        uint16                  usedMemory;
        vector<uint16>::type    availableSlots;
        TextureGpuVec           usedSlots;

        bool hasFreeSlot(void) const;
    };

    typedef list<TexturePool>::type TexturePoolList;
    typedef vector<TexturePool*>::type TexturePoolVec;

    typedef vector<StagingTexture*>::type StagingTextureVec;

    class _OgreExport TextureGpuManager : public ResourceAlloc
    {
    public:
        /// Specifies the minimum squared resolution & number of slices to keep around
        /// all the for time StagingTextures. So:
        /// PixelFormatGpu format = PixelFormatGpuUtils::getFamily(PFG_RGBA8_UNORM);
        /// BudgetEntry( format, 4096u, 2u );
        /// Will keep around 128MB of staging texture, with a resolution of 4096x4096 x 2 slices.
        struct BudgetEntry
        {
            PixelFormatGpu formatFamily;
            uint32 minResolution;
            uint32 minNumSlices;
            BudgetEntry() : formatFamily( PFG_UNKNOWN ), minResolution( 0 ), minNumSlices( 0 ) {}
            BudgetEntry( PixelFormatGpu _formatFamily, uint32 _minResolution, uint32 _minNumSlices ) :
                formatFamily( _formatFamily ), minResolution( _minResolution ),
                minNumSlices( _minNumSlices ) {}

            bool operator () ( const BudgetEntry &_l, const BudgetEntry &_r ) const;
        };

        typedef vector<BudgetEntry>::type BudgetEntryVec;

        struct PoolParameters
        {
            /// Pool shall grow until maxBytesPerPool is reached.
            /// Once that's reached, a new pool will be created.
            /// Otherwise GPU RAM fragmentation may cause out of memory even
            /// though it could otherwise fulfill our request.
            /// Includes mipmaps.
            /// This value may actually be exceeded if a single texture surpasses this limit,
            /// or if minSlicesPerPool is > 1 (it takes higher priority)
            size_t maxBytesPerPool;
            /// Minimum slices per pool, regardless of maxBytesPerPool.
            /// It's also the starting num of slices. See maxResolutionToApplyMinSlices
            uint16 minSlicesPerPool[4];
            /// If texture resolution is <= maxResolutionToApplyMinSlices[i];
            /// we'll apply minSlicesPerPool[i]. Otherwise, we'll apply minSlicesPerPool[i+1]
            /// If resolution > maxResolutionToApplyMinSlices[N]; then minSlicesPerPool = 1;
            uint32 maxResolutionToApplyMinSlices[4];

            /// See BudgetEntry. Must be sorted by size in bytes (biggest entries first).
            BudgetEntryVec budget;
        };

    protected:
        struct ResourceEntry
        {
            String      name;
            TextureGpu  *texture;

            ResourceEntry() : texture( 0 ) {}
            ResourceEntry( const String &_name, TextureGpu *_texture ) :
                name( _name ), texture( _texture ) {}
        };
        typedef map<IdString, ResourceEntry>::type ResourceEntryMap;

        struct LoadRequest
        {
            String      name;
            TextureGpu  *texture;
            Archive     *archive;

            LoadRequest( const String &_name, TextureGpu *_texture, Archive *_archive ) :
                name( _name ), texture( _texture ), archive( _archive ) {}
        };

        typedef vector<LoadRequest>::type LoadRequestVec;

        struct UsageStats
        {
            uint32 width;
            uint32 height;
            PixelFormatGpu formatFamily;
            size_t accumSizeBytes;
            size_t prevSizeBytes;
            uint32 frameCount;
            UsageStats( uint32 _width, uint32 _height, uint32 _depthOrSlices,
                        PixelFormatGpu _formatFamily );
        };
        typedef vector<UsageStats>::type UsageStatsVec;

        struct QueuedImage
        {
            Image2      image;
            uint64      mipLevelBitSet[4];
            TextureGpu  *dstTexture;

            QueuedImage( Image2 &srcImage, uint8 numMips, TextureGpu *_dstTexture );
            void destroy(void);
            bool empty(void) const;
            bool isMipQueued( uint8 mipLevel ) const;
            void unqueueMip( uint8 mipLevel );
            uint8 getMinMipLevel(void) const;
            uint8 getMaxMipLevelPlusOne(void) const;
        };

        typedef vector<QueuedImage>::type QueuedImageVec;

        struct ThreadData
        {
            TexturePoolVec  poolsPending;
            LoadRequestVec  loadRequests;
            ObjCmdBuffer    *objCmdBuffer;
            UsageStatsVec   stats;
            StagingTextureVec availableStagingTex;
            StagingTextureVec usedStagingTex;
        };

        LightweightMutex    mPoolsPendingMutex;
        LightweightMutex    mLoadRequestsMutex;
        LightweightMutex    mMutex;
        ThreadData          mThreadData[2];
        QueuedImageVec      mQueuedImages;

        TexturePoolList     mTexturePool;
        ResourceEntryMap    mEntries;

        PoolParameters      mDefaultPoolParameters;

        StagingTextureVec   mUsedStagingTextures;
        StagingTextureVec   mAvailableStagingTextures;

        StagingTextureVec   mTmpAvailableStagingTex;

        VaoManager          *mVaoManager;

        void destroyAll(void);
        void destroyAllStagingBuffers(void);
        void destroyAllTextures(void);
        void destroyAllPools(void);

        virtual TextureGpu* createTextureImpl( GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                               IdString name, uint32 textureFlags ) = 0;
        virtual StagingTexture* createStagingTextureImpl( uint32 width, uint32 height, uint32 depth,
                                                          uint32 slices, PixelFormatGpu pixelFormat )=0;
        virtual void destroyStagingTextureImpl( StagingTexture *stagingTexture ) = 0;

        uint16 getNumSlicesFor( TextureGpu *texture ) const;

        /// Fills mTmpAvailableStagingTex with new StagingTextures that support formats &
        /// resolutions the worker thread couldn't upload because it lacked a compatible one.
        /// Assumes we're protected by mMutex! Called from main thread.
        void fullfillUsageStats( ThreadData &workerData );
        /// Fills mTmpAvailableStagingTex with new StagingTextures if there's not enough
        /// in there to meet our minimum budget in poolParams.
        void fullfillMinimumBudget( ThreadData &workerData, const PoolParameters &poolParams );

        void fullfillBudget( ThreadData &workerData );

        /** Finds a StagingTexture that can map the given region defined by the box & pixelFormat.
            Searches in both used & available textures.
            If no staging texture supports this request, it will fill a RareRequest entry.
        @remarks
            Assumes workerData is protected by a mutex.
        @param workerData
            Worker thread data.
        @param box
        @param pixelFormat
        @param outStagingTexture
            StagingTexture that mapped the return value. Unmodified if we couldn't map.
        @return
            The mapped region. If TextureBox::data is null, it couldn't be mapped.
        */
        static TextureBox getStreaming( ThreadData &workerData, const TextureBox &box,
                                        PixelFormatGpu pixelFormat, StagingTexture **outStagingTexture );
        static void processQueuedImage( QueuedImage &queuedImage, ThreadData &workerData );

    public:
        TextureGpuManager( VaoManager *vaoManager );
        virtual ~TextureGpuManager();

        void _reserveSlotForTexture( TextureGpu *texture );
        void _releaseSlotFromTexture( TextureGpu *texture );

        void _updateStreaming(void);

        void _update(void);


        /**
        @param name
        @param pageOutStrategy
        @param textureFlags
            See TextureFlags::TextureFlags
        @return
        */
        TextureGpu* createTexture( const String &name,
                                   GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                   uint32 textureFlags );
        void destroyTexture( TextureGpu *texture );

        /**
        @brief getStagingTexture
        @remarks
            We try to find the smallest available texture (won't stall) that can fit the request.
        @param minConsumptionRatioThreshold
            Value in range [0; 100].
            The smallest available texture we find may still be too big (e.g. you need to upload
            64x64 texture RGBA8 and we return a 8192x8192x4 staging texture which is overkill).
            For these cases, here you can specify how much "is too big". For example by
            specifying a consumptionRatio of 50; it means that the data you asked for
            must occupy at least 50% of the space; otherwise we'll create a new StagingTexture.

            A value of 100 means the StagingTexture must fit exactly (fully used).
            A value of 0 means any StagingTexture will do, no matter how large.

            StagingTextures that haven't been using in a while will be destroyed. However
            if for some reason we end up returning a huge texture every frame for small
            workloads, we'll be keeping that waste potentially forever.
        @return
            StagingTexture that meets the criteria. When you're done, remove it by calling
            removeStagingTexture.
        */
        StagingTexture* getStagingTexture( uint32 width, uint32 height, uint32 depth,
                                           uint32 slices, PixelFormatGpu pixelFormat,
                                           size_t minConsumptionRatioThreshold=25u );
        void removeStagingTexture( StagingTexture *stagingTexture );

        const String* findNameStr( IdString idName ) const;

        TextureGpu* loadFromFile( const String &name, const String &resourceGroup,
                                  GpuPageOutStrategy::GpuPageOutStrategy pageOutStrategy,
                                  uint32 textureFlags );
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
