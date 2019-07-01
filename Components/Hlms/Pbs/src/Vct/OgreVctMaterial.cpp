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
#include "OgreSceneManager.h"

#include "OgreMaterialManager.h"
#include "OgreTechnique.h"
#include "OgrePass.h"
#include "OgreTextureUnitState.h"
#include "OgreDepthBuffer.h"
#include "Compositor/OgreCompositorManager2.h"
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
    VctMaterial::VctMaterial( IdType id, VaoManager *vaoManager, CompositorManager2 *compositorManager,
                              TextureGpuManager *textureGpuManager ) :
        IdObject( id ),
        mVaoManager( vaoManager ),
        mNumUsedPoolSlices( 0u ),
        mTexturePool( 0 ),
        mTextureGpuManager( textureGpuManager ),
        mCompositorManager( compositorManager ),
        mDownsampleTex( 0 ),
        mDownsampleMatPass2DArray( 0 ),
        mDownsampleMatPass2D( 0 ),
        mDownsampleWorkspace2DArray( 0 ),
        mDownsampleWorkspace2D( 0 )
    {
        MaterialPtr mat;
        mat = MaterialManager::getSingleton().load(
                  "Ogre/Copy/4xFP32_2DArray",
                  ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME ).staticCast<Material>();
        mDownsampleMatPass2DArray = mat->getTechnique( 0 )->getPass( 0 );

        mat = MaterialManager::getSingleton().load(
                  "Ogre/Copy/4xFP32",
                  ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME ).staticCast<Material>();
        mDownsampleMatPass2D = mat->getTechnique( 0 )->getPass( 0 );
    }
    //-------------------------------------------------------------------------
    VctMaterial::~VctMaterial()
    {
        BucketVec::const_iterator itor = mBuckets.begin();
        BucketVec::const_iterator end  = mBuckets.end();

        while( itor != end )
        {
            mVaoManager->destroyConstBuffer( itor->buffer );
            ++itor;
        }

        mBuckets.clear();
        mDatablockConversionResults.clear();

        if( mTexturePool )
        {
            mTextureGpuManager->destroyTexture( mTexturePool );
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
                shaderMaterial.bgDiffuse[i] = 1.0f;
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

        TextureGpu *diffuseTex = datablock->getDiffuseTexture();
        TextureGpu *emissiveTex = datablock->getEmissiveTexture();

        DatablockConversionResult conversionResult;
        conversionResult.slotIdx        = static_cast<uint32>( usedSlots );
        conversionResult.constBuffer    = bucket.buffer;
        if( diffuseTex )
        {
            conversionResult.diffuseTexIdx = getPoolSliceIdxForTexture( diffuseTex );
            shaderMaterial.diffuseTexIdx = conversionResult.diffuseTexIdx;
        }
        if( emissiveTex )
        {
            conversionResult.emissiveTexIdx = getPoolSliceIdxForTexture( emissiveTex );
            shaderMaterial.emissiveTexIdx = conversionResult.emissiveTexIdx;
        }

        bucket.buffer->upload( &shaderMaterial, usedSlots * sizeof( ShaderVctMaterial ),
                               sizeof( ShaderVctMaterial ) );

        bucket.datablocks.insert( datablock );
        mDatablockConversionResults[datablock] = conversionResult;

        return conversionResult;
    }
    //-------------------------------------------------------------------------
    uint16 VctMaterial::getPoolSliceIdxForTexture( TextureGpu *texture )
    {
        TextureToPoolEntryMap::const_iterator itor = mTextureToPoolEntry.find( texture );
        if( itor != mTextureToPoolEntry.end() )
            return itor->second; //We already copied that texture. We're done

        if( !mTexturePool || mNumUsedPoolSlices >= mTexturePool->getNumSlices() )
            resizeTexturePool();

        texture->waitForData();

        if( texture->getInternalTextureType() == TextureTypes::Type2DArray )
        {
            GpuProgramParametersSharedPtr psParams =
                    mDownsampleMatPass2DArray->getFragmentProgramParameters();
            psParams->setNamedConstant( "sliceIdx",
                                        static_cast<float>( texture->getInternalSliceStart() ) );
            mDownsampleMatPass2DArray->getTextureUnitState( 0 )->setTexture( texture );
            mDownsampleWorkspace2DArray->_update();
        }
        else
        {
            mDownsampleMatPass2D->getTextureUnitState( 0 )->setTexture( texture );
            mDownsampleWorkspace2D->_update();
        }

        const uint16 sliceIdx = mNumUsedPoolSlices++;

        TextureBox dstBox = mTexturePool->getEmptyBox( 0u );
        dstBox.sliceStart = sliceIdx;
        dstBox.numSlices = 1u;
        mDownsampleTex->copyTo( mTexturePool, dstBox, 0u, mDownsampleTex->getEmptyBox( 0u ), 0u );

        mTextureToPoolEntry[texture] = sliceIdx;
        return sliceIdx;
    }
    //-------------------------------------------------------------------------
    void VctMaterial::resizeTexturePool(void)
    {
        String texName = "VctMaterial" + StringConverter::toString( getId() ) + "/" +
                         StringConverter::toString( mNumUsedPoolSlices );
        TextureGpu *newPool = mTextureGpuManager->createTexture( texName, texName,
                                                                 GpuPageOutStrategy::Discard,
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
            mTextureGpuManager->destroyTexture( mTexturePool );
        }

        mTexturePool = newPool;
    }
    //-------------------------------------------------------------------------
    VctMaterial::MaterialBucket* VctMaterial::findFreeBucketFor( HlmsDatablock *datablock )
    {
        const bool needsDiffuse = datablock->getDiffuseTexture() != 0;
        const bool needsEmissive = datablock->getEmissiveTexture() != 0;

        BucketVec::iterator itor = mBuckets.begin();
        BucketVec::iterator end  = mBuckets.end();

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
    void VctMaterial::initTempResources( SceneManager *sceneManager )
    {
        mDownsampleTex = mTextureGpuManager->createTexture( "VctMaterialDownsampleTex",
                                                            "VctMaterialDownsampleTex",
                                                            GpuPageOutStrategy::Discard,
															TextureFlags::RenderToTexture,
                                                            TextureTypes::Type2D );
        mDownsampleTex->setResolution( 64u, 64u );
		mDownsampleTex->setPixelFormat( PFG_RGBA8_UNORM_SRGB );
		mDownsampleTex->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_UNKNOWN );
        mDownsampleTex->scheduleTransitionTo( GpuResidency::Resident );

        Camera *dummyCamera = sceneManager->createCamera( "VctMaterialCam" );

        mDownsampleWorkspace2DArray =
                mCompositorManager->addWorkspace(
                    sceneManager, mDownsampleTex, dummyCamera, "VctTexDownsampleWorkspace", false,
                    -1, 0, 0, 0, Vector4::ZERO, 0x00, 0x01 );
        mDownsampleWorkspace2D =
                mCompositorManager->addWorkspace(
                    sceneManager, mDownsampleTex, dummyCamera, "VctTexDownsampleWorkspace", false,
                    -1, 0, 0, 0, Vector4::ZERO, 0x00, 0x02 );
    }
    //-------------------------------------------------------------------------
    void VctMaterial::destroyTempResources(void)
    {
        mTextureGpuManager->destroyTexture( mDownsampleTex );
        mDownsampleTex = 0;

        Camera *dummyCamera = mDownsampleWorkspace2DArray->getDefaultCamera();
        SceneManager *sceneManager = mDownsampleWorkspace2DArray->getSceneManager();
        sceneManager->destroyCamera( dummyCamera );
        dummyCamera = 0;

        mCompositorManager->removeWorkspace( mDownsampleWorkspace2DArray );
        mDownsampleWorkspace2DArray = 0;

        mCompositorManager->removeWorkspace( mDownsampleWorkspace2D );
        mDownsampleWorkspace2D = 0;
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
                newBucket.buffer = mVaoManager->createConstBuffer( c_numDatablocksPerConstBuffer *
                                                                   sizeof(ShaderVctMaterial),
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
