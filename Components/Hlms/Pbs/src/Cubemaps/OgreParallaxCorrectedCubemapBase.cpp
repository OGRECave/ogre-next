/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#include "Compositor/OgreCompositorWorkspaceDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/Pass/PassClear/OgreCompositorPassClearDef.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"

#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreDepthBuffer.h"

#include "OgreMaterialManager.h"
#include "OgreTechnique.h"
#include "OgreLwString.h"

#include "OgreMeshManager2.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "OgreItem.h"

#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreStagingBuffer.h"

namespace Ogre
{
    ParallaxCorrectedCubemapBase::ParallaxCorrectedCubemapBase(
            IdType id, Root *root, SceneManager *sceneManager,
            const CompositorWorkspaceDef *probeWorkspcDef, bool automaticMode ) :
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
        mDefaultWorkspaceDef( probeWorkspcDef )
    {
        HlmsManager *hlmsManager = mRoot->getHlmsManager();
        HlmsSamplerblock samplerblock;
        samplerblock.mMipFilter = FO_LINEAR;
        mSamplerblockTrilinear = hlmsManager->getSamplerblock( samplerblock );
    }
    //-----------------------------------------------------------------------------------
    ParallaxCorrectedCubemapBase::~ParallaxCorrectedCubemapBase()
    {
        destroyAllProbes();

        HlmsManager *hlmsManager = mRoot->getHlmsManager();
        hlmsManager->destroySamplerblock( mSamplerblockTrilinear );
        mSamplerblockTrilinear = 0;
    }
    //-----------------------------------------------------------------------------------
    CubemapProbe* ParallaxCorrectedCubemapBase::createProbe(void)
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
    void ParallaxCorrectedCubemapBase::destroyAllProbes(void)
    {
        CubemapProbeVec::iterator itor = mProbes.begin();
        CubemapProbeVec::iterator end  = mProbes.end();

        while( itor != end )
        {
            OGRE_DELETE *itor;
            ++itor;
        }

        mProbes.clear();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::prepareForClearScene(void)
    {
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::restoreFromClearScene(void)
    {
        SceneNode *rootNode = mSceneManager->getRootSceneNode();
        CubemapProbeVec::iterator itor = mProbes.begin();
        CubemapProbeVec::iterator end  = mProbes.end();
        while( itor != end )
        {
            (*itor)->restoreFromClearScene( rootNode );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::_notifyPreparePassHash( const Matrix4 &viewMatrix )
    {
    }
    //-----------------------------------------------------------------------------------
    size_t ParallaxCorrectedCubemapBase::getConstBufferSize(void)
    {
        return 0;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::fillConstBufferData( const Matrix4 &viewMatrix,
                                                            float * RESTRICT_ALIAS passBufferPtr ) const
    {
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapBase::fillConstBufferData( const CubemapProbe &probe,
                                                            const Matrix4 &viewMatrix,
                                                            const Matrix3 &invViewMat3,
                                                            float * RESTRICT_ALIAS passBufferPtr )
    {
        const Matrix3 viewSpaceToProbeLocal = probe.mInvOrientation * invViewMat3;

        const Aabb &probeShape = probe.getProbeShape();
        Vector3 probeShapeCenterVS = viewMatrix * probeShape.mCenter; //View-space

        //float4 row0_centerX;
        *passBufferPtr++ = viewSpaceToProbeLocal[0][0];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][1];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][2];
        *passBufferPtr++ = probeShapeCenterVS.x;

        //float4 row1_centerY;
        *passBufferPtr++ = viewSpaceToProbeLocal[0][3];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][4];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][5];
        *passBufferPtr++ = probeShapeCenterVS.y;

        //float4 row2_centerZ;
        *passBufferPtr++ = viewSpaceToProbeLocal[0][6];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][7];
        *passBufferPtr++ = viewSpaceToProbeLocal[0][8];
        *passBufferPtr++ = probeShapeCenterVS.z;

        //float4 halfSize;
        *passBufferPtr++ = probeShape.mHalfSize.x;
        *passBufferPtr++ = probeShape.mHalfSize.y;
        *passBufferPtr++ = probeShape.mHalfSize.z;
        *passBufferPtr++ = 1.0f;

        //float4 cubemapPosLS;
        Vector3 cubemapPosLS = probe.mProbeCameraPos - probeShape.mCenter;
        cubemapPosLS = probe.mInvOrientation * cubemapPosLS;
        *passBufferPtr++ = cubemapPosLS.x;
        *passBufferPtr++ = cubemapPosLS.y;
        *passBufferPtr++ = cubemapPosLS.z;
        *passBufferPtr++ = 1.0f;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* ParallaxCorrectedCubemapBase::findTmpRtt( const TextureGpu *baseParams )
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
    void ParallaxCorrectedCubemapBase::_copyRenderTargetToCubemap( uint32 cubemapArrayIdx )
    {
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* ParallaxCorrectedCubemapBase::_acquireTextureSlot( uint32 &outTexSlot )
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
    SceneManager* ParallaxCorrectedCubemapBase::getSceneManager(void) const
    {
        return mSceneManager;
    }
    //-----------------------------------------------------------------------------------
    const CompositorWorkspaceDef* ParallaxCorrectedCubemapBase::getDefaultWorkspaceDef(void) const
    {
        return mDefaultWorkspaceDef;
    }
}
