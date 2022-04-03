/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

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

#include "Cubemaps/OgreParallaxCorrectedCubemapBase.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"
#include "Compositor/Pass/PassClear/OgreCompositorPassClearDef.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuad.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"

#include "OgreCamera.h"
#include "OgreDepthBuffer.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreTextureGpuManager.h"

#include "OgreLwString.h"
#include "OgreMaterialManager.h"
#include "OgreTechnique.h"

#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"

#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreStagingBuffer.h"

namespace Ogre
{
    ParallaxCorrectedCubemapBase::ParallaxCorrectedCubemapBase(
        IdType id, Root *root, SceneManager *sceneManager, const CompositorWorkspaceDef *probeWorkspcDef,
        bool automaticMode ) :
        IdObject( id ),
        mBindTexture( 0 ),
        mSamplerblockTrilinear( 0 ),
        mAutomaticMode( automaticMode ),
        mUseDpm2DArray( false ),
        mIsRendering( false ),
        mPaused( false ),
        mMask( 0xffffffff ),
        mRoot( root ),
        mSceneManager( sceneManager ),
        mDefaultWorkspaceDef( probeWorkspcDef ),
        mPccCompressorPass( 0 ),
        mProbeRenderInProgress( 0 )
    {
        HlmsManager *hlmsManager = mRoot->getHlmsManager();
        HlmsSamplerblock samplerblock;
        samplerblock.mMipFilter = FO_LINEAR;
        mSamplerblockTrilinear = hlmsManager->getSamplerblock( samplerblock );

        MaterialPtr depthCompressor = MaterialManager::getSingleton().getByName(
            "PCC/DepthCompressor", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        if( depthCompressor )
        {
            depthCompressor->load();
            mPccCompressorPass = depthCompressor->getTechnique( 0 )->getPass( 0 );
        }

        RenderSystem::addSharedListener( this );
    }
    //-----------------------------------------------------------------------------------
    ParallaxCorrectedCubemapBase::~ParallaxCorrectedCubemapBase()
    {
        RenderSystem::removeSharedListener( this );

        destroyAllProbes();

        HlmsManager *hlmsManager = mRoot->getHlmsManager();
        hlmsManager->destroySamplerblock( mSamplerblockTrilinear );
        mSamplerblockTrilinear = 0;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::_releaseManualHardwareResources()
    {
        for( CubemapProbeVec::iterator it = mProbes.begin(), end = mProbes.end(); it != end; ++it )
            ( *it )->_releaseManualHardwareResources();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::_restoreManualHardwareResources()
    {
        for( CubemapProbeVec::iterator it = mProbes.begin(), end = mProbes.end(); it != end; ++it )
            ( *it )->_restoreManualHardwareResources();

        updateAllDirtyProbes();
    }
    //-----------------------------------------------------------------------------------
    uint32 ParallaxCorrectedCubemapBase::getIblTargetTextureFlags( PixelFormatGpu pixelFormat ) const
    {
        const RenderSystemCapabilities *caps =
            mSceneManager->getDestinationRenderSystem()->getCapabilities();
        uint32 textureFlags;
        if( caps->hasCapability( RSC_UAV ) )
        {
            textureFlags = TextureFlags::Uav;
            if( PixelFormatGpuUtils::isSRgb( pixelFormat ) )
                textureFlags |= TextureFlags::Reinterpretable;
        }
        else
        {
            textureFlags = TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps;
        }
        return textureFlags;
    }
    //-----------------------------------------------------------------------------------
    uint8 ParallaxCorrectedCubemapBase::getIblNumMipmaps( uint32 width, uint32 height )
    {
        uint8 numMipmaps = PixelFormatGpuUtils::getMaxMipmapCount( width, height );
        numMipmaps = std::max<uint8>( numMipmaps, 5u ) - 4u;
        return numMipmaps;
    }
    //-----------------------------------------------------------------------------------
    CubemapProbe *ParallaxCorrectedCubemapBase::createProbe()
    {
        CubemapProbe *probe = OGRE_NEW CubemapProbe( this );
        mProbes.push_back( probe );
        return probe;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::destroyProbe( CubemapProbe *probe )
    {
        CubemapProbeVec::iterator itor = std::find( mProbes.begin(), mProbes.end(), probe );
        if( itor == mProbes.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Probe to delete does not belong to us, or was already freed",
                         "ParallaxCorrectedCubemapBase::destroyProbe" );
        }

        OGRE_DELETE *itor;
        efficientVectorRemove( mProbes, itor );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::destroyAllProbes()
    {
        CubemapProbeVec::iterator itor = mProbes.begin();
        CubemapProbeVec::iterator end = mProbes.end();

        while( itor != end )
        {
            OGRE_DELETE *itor;
            ++itor;
        }

        mProbes.clear();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::prepareForClearScene() {}
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::restoreFromClearScene()
    {
        SceneNode *rootNode = mSceneManager->getRootSceneNode();
        CubemapProbeVec::iterator itor = mProbes.begin();
        CubemapProbeVec::iterator end = mProbes.end();
        while( itor != end )
        {
            ( *itor )->restoreFromClearScene( rootNode );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::_notifyPreparePassHash( const Matrix4 &viewMatrix ) {}
    //-----------------------------------------------------------------------------------
    size_t ParallaxCorrectedCubemapBase::getConstBufferSize() { return 0; }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::fillConstBufferData( const Matrix4 &viewMatrix,
                                                            float *RESTRICT_ALIAS passBufferPtr ) const
    {
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::fillConstBufferData( const CubemapProbe &probe,
                                                            const Matrix4 &viewMatrix,
                                                            const Matrix3 &invViewMat3,
                                                            float *RESTRICT_ALIAS passBufferPtr )
    {
        const Matrix3 viewSpaceToProbeLocal = probe.mInvOrientation * invViewMat3;

        const Aabb &probeShape = probe.getProbeShape();
        Vector3 probeShapeCenterVS = viewMatrix * probeShape.mCenter;  // View-space

        // float4 row0_centerX;
        *passBufferPtr++ = viewSpaceToProbeLocal[0][0];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][1];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][2];
        *passBufferPtr++ = probeShapeCenterVS.x;

        // float4 row1_centerY;
        *passBufferPtr++ = viewSpaceToProbeLocal[0][3];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][4];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][5];
        *passBufferPtr++ = probeShapeCenterVS.y;

        // float4 row2_centerZ;
        *passBufferPtr++ = viewSpaceToProbeLocal[0][6];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][7];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][8];
        *passBufferPtr++ = probeShapeCenterVS.z;

        // float4 halfSize;
        *passBufferPtr++ = probeShape.mHalfSize.x;
        *passBufferPtr++ = probeShape.mHalfSize.y;
        *passBufferPtr++ = probeShape.mHalfSize.z;
        *passBufferPtr++ = 1.0f;

        // float4 cubemapPosLS;
        const Vector3 cubemapPos = probe.mProbeCameraPos - probeShape.mCenter;
        const Vector3 cubemapPosLS = probe.mInvOrientation * cubemapPos;
        *passBufferPtr++ = cubemapPosLS.x;
        *passBufferPtr++ = cubemapPosLS.y;
        *passBufferPtr++ = cubemapPosLS.z;
        *passBufferPtr++ = 1.0f;

        // float4 cubemapPosVS;
        const Vector3 cubemapPosVS = viewMatrix * cubemapPos;
        *passBufferPtr++ = cubemapPosVS.x;
        *passBufferPtr++ = cubemapPosVS.y;
        *passBufferPtr++ = cubemapPosVS.z;
        *passBufferPtr++ = 1.0f;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *ParallaxCorrectedCubemapBase::findTmpRtt( const TextureGpu *baseParams )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "", "" );
        return 0;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::releaseTmpRtt( const TextureGpu *tmpRtt )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "", "" );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *ParallaxCorrectedCubemapBase::findIbl( const TextureGpu *baseParams )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "", "" );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::releaseIbl( const TextureGpu *ibl )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "", "" );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::_copyRenderTargetToCubemap( uint32 cubemapArrayIdx ) {}
    //-----------------------------------------------------------------------------------
    TextureGpu *ParallaxCorrectedCubemapBase::_acquireTextureSlot( uint16 &outTexSlot )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "", "" );
        outTexSlot = 0;
        return 0;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::_releaseTextureSlot( TextureGpu *texture, uint32 texSlot )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "", "" );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::_addManuallyActiveProbe( CubemapProbe *probe )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "", "" );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::_removeManuallyActiveProbe( CubemapProbe *probe )
    {
        OGRE_EXCEPT( Exception::ERR_INVALID_CALL, "", "" );
    }
    //-----------------------------------------------------------------------------------
    SceneManager *ParallaxCorrectedCubemapBase::getSceneManager() const { return mSceneManager; }
    //-----------------------------------------------------------------------------------
    const CompositorWorkspaceDef *ParallaxCorrectedCubemapBase::getDefaultWorkspaceDef() const
    {
        return mDefaultWorkspaceDef;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::passPreExecute( CompositorPass *pass )
    {
        const CompositorPassDef *passDef = pass->getDefinition();
        if( passDef->getType() != PASS_QUAD )
            return;

        OGRE_ASSERT_HIGH( dynamic_cast<CompositorPassQuad *>( pass ) );
        CompositorPassQuad *passQuad = static_cast<CompositorPassQuad *>( pass );
        if( passQuad->getPass() == mPccCompressorPass )
        {
            GpuProgramParametersSharedPtr psParams = mPccCompressorPass->getFragmentProgramParameters();

            Ogre::Camera *camera = passQuad->getCamera();
            Ogre::Vector2 projectionAB = camera->getProjectionParamsAB();
            // The division will keep "linearDepth" in the shader in the [0; 1] range.
            projectionAB.y /= camera->getFarClipDistance();
            psParams->setNamedConstant( "projectionParams", projectionAB );

            CubemapProbe *probe = mProbeRenderInProgress;

            const Aabb &probeShape = probe->getProbeShape();
            Vector3 cameraPosLS = probe->mProbeCameraPos - probeShape.mCenter;
            cameraPosLS = probe->mInvOrientation * cameraPosLS;

            const Matrix4 viewMatrix4 = camera->getViewMatrix();
            Matrix3 viewMatrix3;
            viewMatrix4.extract3x3Matrix( viewMatrix3 );
            const Matrix3 invViewMatrix3 = viewMatrix3.Inverse();

            Matrix3 viewSpaceToProbeLocalSpace = probe->mInvOrientation * invViewMatrix3;
            psParams->setNamedConstant( "probeShapeHalfSize", probe->mProbeShape.mHalfSize );
            psParams->setNamedConstant( "cameraPosLS", cameraPosLS );
            psParams->setNamedConstant( "viewSpaceToProbeLocalSpace", viewSpaceToProbeLocalSpace );
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::eventOccurred( const String &eventName,
                                                      const NameValuePairList *parameters )
    {
        if( eventName == "DeviceLost" )
            _releaseManualHardwareResources();
        else if( eventName == "DeviceRestored" )
            _restoreManualHardwareResources();
    }
}  // namespace Ogre
