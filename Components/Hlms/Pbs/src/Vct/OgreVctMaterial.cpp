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

#include "OgreStableHeaders.h"

#include "Vct/OgreVctMaterial.h"

#include "Vao/OgreVaoManager.h"
#include "Vao/OgreConstBufferPacked.h"

#include "OgreHlms.h"
#include "OgreHlmsPbsDatablock.h"

#include "OgreTextureGpuManager.h"
#include "OgreTextureBox.h"

#include "OgreTextureUnitState.h"
#include "Compositor/OgreCompositorWorkspace.h"

#include "OgreLogManager.h"

namespace Ogre
{
    static const size_t c_numDatablocksPerConstBuffer = 1024u;

    struct ShaderVctMaterial
    {
        float bgDiffuse[4];
        float diffuse[4];
        float emissive[4];
        uint32 diffuseTexIdx;
        uint32 emissiveTexIdx;
        uint32 padding01[2];
    };
    //-------------------------------------------------------------------------
    VctMaterial::VctMaterial( VaoManager *vaoManager ) :
        mVaoManager( vaoManager ),
        mNumUsedPoolSlices( 0u ),
        mTexturePool( 0 ),
        mDownscaleTex( 0 ),
        mDownscaleMatTextureUnit( 0 ),
        mDownscaleWorkspace( 0 )
    {
    }
    //-------------------------------------------------------------------------
    VctMaterial::~VctMaterial()
    {
        BucketArray::const_iterator itor = mBuckets.begin();
        BucketArray::const_iterator end  = mBuckets.end();

        while( itor != end )
        {
            mVaoManager->destroyConstBuffer( itor->buffer );
            ++itor;
        }

        mBuckets.clear();
        mDatablockConversionResults.clear();

        if( mTexturePool )
        {
            TextureGpuManager *textureManager = 0;
            textureManager->destroyTexture( mTexturePool );
            mTexturePool = 0;
        }
    }
    //-------------------------------------------------------------------------
    VctMaterial::DatablockConversionResult VctMaterial::addDatablockToBucket( HlmsDatablock *datablock,
                                                                              MaterialBucket &bucket )
    {
        const size_t usedSlots = bucket.datablocks.size();

        OGRE_ASSERT_MEDIUM( usedSlots < c_numDatablocksPerConstBuffer );

        ShaderVctMaterial shaderMaterial;
        memset( &shaderMaterial, 0, sizeof(shaderMaterial) );

        {
            ColourValue diffuseCol = datablock->getDiffuseColour();
            ColourValue emissiveCol = datablock->getEmissiveColour();
            for( size_t i=0; i<4; ++i )
            {
                shaderMaterial.diffuse[i] = diffuseCol[i];
                shaderMaterial.emissive[i] = emissiveCol[i];
            }

            shaderMaterial.diffuse[3] = Ogre::max( diffuseCol.a, emissiveCol.a );
        }

        if( datablock->getCreator()->getType() == HLMS_PBS )
        {
            OGRE_ASSERT_HIGH( dynamic_cast<HlmsPbsDatablock*>( datablock ) );
            HlmsPbsDatablock *pbsDatablock = static_cast<HlmsPbsDatablock*>( datablock );

            ColourValue bgDiffuse = pbsDatablock->getBackgroundDiffuse();
            float transparency = pbsDatablock->getTransparency();

            for( size_t i=0; i<4; ++i )
                shaderMaterial.bgDiffuse[i] = bgDiffuse[i];
            shaderMaterial.diffuse[3] = transparency;
            shaderMaterial.emissive[3] = 1.0f;
        }

        bucket.buffer->upload( &shaderMaterial, usedSlots * sizeof( ShaderVctMaterial ),
                               sizeof( ShaderVctMaterial ) );

        bucket.datablocks.insert( datablock );

        TextureGpu *diffuseTex = datablock->getDiffuseTexture();
        TextureGpu *emissiveTex = datablock->getEmissiveTexture();

        DatablockConversionResult conversionResult;
        conversionResult.slotIdx        = static_cast<uint32>( usedSlots );
        conversionResult.constBuffer    = bucket.buffer;
        if( diffuseTex )
            conversionResult.diffuseTexIdx = getPoolSliceIdxForTexture( diffuseTex );
        if( emissiveTex )
            conversionResult.emissiveTexIdx = getPoolSliceIdxForTexture( emissiveTex );
        mDatablockConversionResults[datablock] = conversionResult;

        return conversionResult;
    }
    //-------------------------------------------------------------------------
    uint16 VctMaterial::getPoolSliceIdxForTexture( TextureGpu *texture )
    {
        TextureToPoolEntryMap::const_iterator itor = mTextureToPoolEntry.find( texture );
        if( itor != mTextureToPoolEntry.end() )
            return itor->second; //We already copied that texture. We're done

        mDownscaleMatTextureUnit->setTexture( texture );
        mDownscaleWorkspace->_update();

        const uint16 sliceIdx = mNumUsedPoolSlices++;

        TextureBox dstBox = mTexturePool->getEmptyBox( 0u );
        dstBox.sliceStart = sliceIdx;
        dstBox.numSlices = 1u;
        mDownscaleTex->copyTo( mTexturePool, dstBox, 0u, mDownscaleTex->getEmptyBox( 0u ), 0u );

        mTextureToPoolEntry[texture] = sliceIdx;
        return sliceIdx;
    }
    //-------------------------------------------------------------------------
    void VctMaterial::resizeTexturePool(void)
    {
        TextureGpuManager *textureManager = 0;


        TextureGpu *newPool = textureManager->createTexture( "", "", GpuPageOutStrategy::Discard,
                                                             TextureFlags::ManualTexture,
                                                             TextureTypes::Type2DArray );
        newPool->setResolution( 64u, 64u, 64u );
        newPool->setPixelFormat( PFG_RGBA8_UNORM_SRGB );
        if( mTexturePool )
        {
            //We use quadratic growth because AMD GCN cards already round up to the next power of 2.
            newPool->setResolution( 64u, 64u, mTexturePool->getDepthOrSlices() << 1u );
        }
        newPool->scheduleTransitionTo( GpuResidency::Resident );

        if( mTexturePool )
        {
            TextureBox box( mTexturePool->getEmptyBox( 0u ) );
            mTexturePool->copyTo( newPool, box, 0u, box, 0u );
            textureManager->destroyTexture( mTexturePool );
        }

        mTexturePool = newPool;
    }
    //-------------------------------------------------------------------------
    VctMaterial::MaterialBucket* VctMaterial::findFreeBucketFor( HlmsDatablock *datablock )
    {
        const bool needsDiffuse = datablock->getDiffuseTexture() != 0;
        const bool needsEmissive = datablock->getEmissiveTexture() != 0;

        BucketArray::iterator itor = mBuckets.begin();
        BucketArray::iterator end  = mBuckets.end();

        while( itor != end &&
               (itor->datablocks.size() >= c_numDatablocksPerConstBuffer ||
                itor->hasDiffuse != needsDiffuse ||
                itor->hasEmissive != needsEmissive) )
        {
            ++itor;
        }

        MaterialBucket *retVal = 0;
        if( itor != end )
            retVal = &(*itor);

        return retVal;
    }
    //-------------------------------------------------------------------------
    VctMaterial::DatablockConversionResult VctMaterial::addDatablock( HlmsDatablock *datablock )
    {
        DatablockConversionResult retVal;

        DatablockConversionResultMap::const_iterator itResult =
                mDatablockConversionResults.find( datablock );
        if( itResult != mDatablockConversionResults.end() )
            retVal = itResult->second;
        else
        {
            MaterialBucket *bucket = findFreeBucketFor( datablock );
            if( !bucket )
            {
                //Create a new bucket
                MaterialBucket newBucket;
                newBucket.buffer = mVaoManager->createConstBuffer( 1024u * sizeof(ShaderVctMaterial),
                                                                   BT_DEFAULT, 0, false );
                newBucket.hasDiffuse = datablock->getDiffuseTexture() != 0;
                newBucket.hasEmissive = datablock->getEmissiveTexture() != 0;
                mBuckets.push_back( newBucket );
                bucket = &mBuckets.back();
            }

            retVal = addDatablockToBucket( datablock, *bucket );
        }

        return retVal;
    }
}
