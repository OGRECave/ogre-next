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
        struct PoolParameters
        {
            /// Pool shall grow until maxBytesPerPool is reached.
            /// Once that's reached, a new pool will be created.
            /// Otherwise GPU RAM fragmentation may cause out of memory even
            /// though it could otherwise fulfill our request.
            /// Includes mipmaps.
            /// This value may actually be exceeded if a single texture surpasses this limit,
            /// or if minSlicesPerPool is > 1 (it takes higher priority)
            size_t  maxBytesPerPool;
            /// Minimum slices per pool, regardless of maxBytesPerPool.
            /// It's also the starting num of slices. See maxResolutionToApplyMinSlices
            uint16  minSlicesPerPool[4];
            /// If texture resolution is <= maxResolutionToApplyMinSlices[i];
            /// we'll apply minSlicesPerPool[i]. Otherwise, we'll apply minSlicesPerPool[i+1]
            /// If resolution > maxResolutionToApplyMinSlices[N]; then minSlicesPerPool = 1;
            uint32  maxResolutionToApplyMinSlices[4];
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

        /// A rare request is a texture which we could not upload because
        /// no StagingTexture in availableStagingTex was capable of
        /// handling, so we need to tell the main thread about it.
        struct RareRequest
        {
            uint32 width;
            uint32 height;
            PixelFormatGpu pixelFormat;
            size_t accumSizeBytes;
            RareRequest( uint32 _width, uint32 _height, uint32 _depthOrSlices,
                         PixelFormatGpu _pixelFormat );
        };
        typedef vector<RareRequest>::type RareRequestVec;

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
            RareRequestVec  rareRequests;
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

        StagingTexture* getStagingTexture( uint32 width, uint32 height, uint32 depth,
                                           uint32 slices, PixelFormatGpu pixelFormat );
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
