/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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
#ifndef _OgreVctMaterial_H_
#define _OgreVctMaterial_H_

#include "OgreHlmsPbsPrerequisites.h"
#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreHlmsPbsExport VctMaterial
    {
    public:
        struct DatablockConversionResult
        {
            uint32              slotIdx;
            ConstBufferPacked   *constBuffer;
            uint16              diffuseTexIdx;
            uint16              emissiveTexIdx;
            DatablockConversionResult() :
                slotIdx( (uint32)-1 ), constBuffer( 0 ),
                diffuseTexIdx( std::numeric_limits<uint16>::max() ),
                emissiveTexIdx( std::numeric_limits<uint16>::max() ) {}

            bool hasDiffuseTex(void) const
            { return diffuseTexIdx != std::numeric_limits<uint16>::max(); }
            bool hasEmissiveTex(void) const
            { return emissiveTexIdx != std::numeric_limits<uint16>::max(); }
        };

    protected:
        typedef set<HlmsDatablock*>::type HlmsDatablockSet;
        struct MaterialBucket
        {
            ConstBufferPacked   *buffer;
            bool                hasDiffuse;
            bool                hasEmissive;
            HlmsDatablockSet    datablocks;
        };
        typedef FastArray<MaterialBucket> BucketArray;

        typedef map<HlmsDatablock*, DatablockConversionResult>::type DatablockConversionResultMap;
        DatablockConversionResultMap mDatablockConversionResults;

        BucketArray mBuckets;

        VaoManager  *mVaoManager;

        typedef map<TextureGpu*, uint16>::type TextureToPoolEntryMap;
        uint16                  mNumUsedPoolSlices;
        TextureGpu              *mTexturePool;
        TextureGpu              *mDownscaleTex;
        TextureUnitState        *mDownscaleMatTextureUnit;
        CompositorWorkspace     *mDownscaleWorkspace;
        TextureToPoolEntryMap   mTextureToPoolEntry;

        DatablockConversionResult addDatablockToBucket( HlmsDatablock *datablock,
                                                        MaterialBucket &bucket );
        uint16 getPoolSliceIdxForTexture( TextureGpu *texture );

        void resizeTexturePool(void);

        MaterialBucket* findFreeBucketFor( HlmsDatablock *datablock );

    public:
        VctMaterial( VaoManager *vaoManager );
        ~VctMaterial();

        DatablockConversionResult addDatablock( HlmsDatablock *datablock );

        TextureGpu* getTexturePool(void) const      { return mTexturePool; }
    };
}

#include "OgreHeaderSuffix.h"

#endif
