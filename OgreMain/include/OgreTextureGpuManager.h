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
#include "OgreLightweightMutex.h"

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

	class _OgreExport TextureGpuManager
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

        LightweightMutex    mMutex;

        TexturePoolList     mTexturePool;
        ResourceEntryMap    mEntries;
        TexturePoolVec      mPoolsPending[2];

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

    public:
        TextureGpuManager( VaoManager *vaoManager );
        virtual ~TextureGpuManager();

        void _reserveSlotForTexture( TextureGpu *texture );
        void _releaseSlotFromTexture( TextureGpu *texture );

        void update(void);


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
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
