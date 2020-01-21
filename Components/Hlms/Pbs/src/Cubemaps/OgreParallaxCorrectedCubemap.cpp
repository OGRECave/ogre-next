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

#include "Cubemaps/OgreParallaxCorrectedCubemap.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/Pass/PassClear/OgreCompositorPassClearDef.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"

#include "OgreHlmsPbs.h"

#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreRenderQueue.h"
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
#include "Vao/OgreVaoManager.h"

namespace Ogre
{
    const char *cSuffixes[6] =
    {
        "PX", "NX",
        "PY", "NY",
        "PZ", "NZ",
    };

    ParallaxCorrectedCubemap::ParallaxCorrectedCubemap( IdType id, Root *root,
                                                        SceneManager *sceneManager,
                                                        const CompositorWorkspaceDef *probeWorkspcDef,
                                                        uint8 reservedRqId, uint32 proxyVisibilityMask ) :
        ParallaxCorrectedCubemapBase( id, root, sceneManager, probeWorkspcDef, false ),
        mNumCollectedProbes( 0 ),
        mStagingBuffer( 0 ),
        mLastPassNumViewMatrices( 1 ),
        mCachedLastViewMatrix( Matrix4::ZERO ),
        mBlendedProbeNeedsUpdate( true ),
        mTrackedPosition( Vector3::ZERO ),
        mTrackedViewProjMatrix( Matrix4::IDENTITY ),
        mBlankProbe( this ),
        mFinalProbe( this ),
        mBlendProxyCamera( 0 ),
        mBlendWorkspace( 0 ),
        mSamplerblockPoint( 0 ),
        mCurrentMip( 0 ),
        mProxyVisibilityMask( proxyVisibilityMask ),
        mReservedRqId( reservedRqId )
    {
        memset( mProbeNDFs, 0, sizeof(mProbeNDFs) );
        memset( mProbeBlendFactors, 0, sizeof(mProbeBlendFactors) );
        memset( mCollectedProbes, 0, sizeof(mCollectedProbes) );
        memset( mBlendCubemapTUs, 0, sizeof(mBlendCubemapTUs) );
        memset( mCopyCubemapTUs, 0, sizeof(mCopyCubemapTUs) );
        memset( mProxyItems, 0, sizeof(mProxyItems) );
        memset( mProxyNodes, 0, sizeof(mProxyNodes) );
        createProxyGeometry();
        createCubemapBlendWorkspaceDefinition();

        //Save the TextureUnitStates for setting the cubemap probes for blending every frame.
        char tmpBuffer[64];
        LwString materialName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
        {
            materialName = "Cubemap/BlendProjectCubemap";
            const size_t matNameSize = materialName.size();

            for( size_t i=0; i<OGRE_MAX_CUBE_PROBES; ++i )
            {
                materialName.resize( matNameSize );
                materialName.a( (uint32)i );
                MaterialPtr material = MaterialManager::getSingleton().load(
                            materialName.c_str(), ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME ).
                        staticCast<Material>();
                Pass *pass = material->getTechnique(0)->getPass(0);

                mBlendCubemapParamsVs[i] = pass->getVertexProgramParameters();
                mBlendCubemapParams[i] = pass->getFragmentProgramParameters();
                mBlendCubemapTUs[i] = pass->getTextureUnitState( 0 );
            }
        }

        {
            materialName = "Cubemap/CopyCubemap_";
            const size_t matNameSize = materialName.size();

            for( size_t i=0; i<6; ++i )
            {
                materialName.resize( matNameSize );
                materialName.a( cSuffixes[i] );
                MaterialPtr material = MaterialManager::getSingleton().load(
                            materialName.c_str(), ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME ).
                        staticCast<Material>();
                Pass *pass = material->getTechnique(0)->getPass(0);

                mCopyCubemapParams[i] = pass->getFragmentProgramParameters();
                mCopyCubemapTUs[i] = pass->getTextureUnitState( 0 );
            }
        }

        mBlankProbe.setTextureParams( 1, 1 );
        mBlankProbe.set( Vector3::ZERO, Aabb( Vector3::ZERO, Vector3::UNIT_SCALE ),
                         Vector3::ZERO, Matrix3::IDENTITY,
                         Aabb( Vector3::ZERO, Vector3::UNIT_SCALE ) );
        mBlankProbe.mDirty = false;

        HlmsManager *hlmsManager = mRoot->getHlmsManager();
        HlmsSamplerblock samplerblock;
        samplerblock.mMipFilter = FO_NONE;
        mSamplerblockPoint = hlmsManager->getSamplerblock( samplerblock );
    }
    //-----------------------------------------------------------------------------------
    ParallaxCorrectedCubemap::~ParallaxCorrectedCubemap()
    {
        setEnabled( false, 0, 0, PFG_UNKNOWN );

        destroyAllProbes();

        destroyProxyGeometry();

        if( mBindTexture )
        {
            TextureGpuManager *textureGpuManager =
                    mSceneManager->getDestinationRenderSystem()->getTextureGpuManager();
            textureGpuManager->destroyTexture( mBindTexture );
            mBindTexture = 0;
        }

        HlmsManager *hlmsManager = mRoot->getHlmsManager();
        hlmsManager->destroySamplerblock( mSamplerblockPoint );
        mSamplerblockPoint = 0;

        if( mStagingBuffer )
        {
            mStagingBuffer->removeReferenceCount();
            mStagingBuffer = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::destroyAllProbes(void)
    {
        {
            TextureGpuManager *textureGpuManager =
                    mSceneManager->getDestinationRenderSystem()->getTextureGpuManager();
            TempRttVec::const_iterator itor = mTmpRtt.begin();
            TempRttVec::const_iterator end  = mTmpRtt.end();
            while( itor != end )
            {
                textureGpuManager->destroyTexture( itor->texture );
                ++itor;
            }
            mTmpRtt.clear();
        }

        ParallaxCorrectedCubemapBase::destroyAllProbes();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::createProxyItems(void)
    {
        destroyProxyItems();

        char tmpBuffer[64];
        LwString materialName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
        materialName = "Cubemap/BlendProjectCubemap";
        const size_t matNameSize = materialName.size();

        //Create the Items using that geometry
        for( size_t i=0; i<OGRE_MAX_CUBE_PROBES; ++i )
        {
            mProxyItems[i] = mSceneManager->createItem( mProxyMesh, SCENE_DYNAMIC );
            mProxyNodes[i] = mSceneManager->getRootSceneNode()->createChildSceneNode();
            mProxyItems[i]->setRenderQueueGroup( mReservedRqId );
            mProxyItems[i]->setVisibilityFlags( mProxyVisibilityMask );
            mProxyItems[i]->setCastShadows( false );
            mProxyNodes[i]->attachObject( mProxyItems[i] );

            materialName.resize( matNameSize );
            materialName.a( (uint32)i );
            mProxyItems[i]->setMaterialName( materialName.c_str() );
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::destroyProxyItems(void)
    {
        for( size_t i=0; i<OGRE_MAX_CUBE_PROBES; ++i )
        {
            if( mProxyNodes[i] )
            {
                mProxyNodes[i]->getParentSceneNode()->removeAndDestroyChild( mProxyNodes[i] );
                mProxyNodes[i] = 0;
            }
            if( mProxyItems[i] )
            {
                mSceneManager->destroyItem( mProxyItems[i] );
                mProxyItems[i] = 0;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::prepareForClearScene(void)
    {
        destroyProxyItems();
        ParallaxCorrectedCubemapBase::prepareForClearScene();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::restoreFromClearScene(void)
    {
        createProxyItems();
        SceneNode *rootNode = mSceneManager->getRootSceneNode();
        rootNode->attachObject( mBlendProxyCamera );

        ParallaxCorrectedCubemapBase::restoreFromClearScene();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::setUpdatedTrackedDataFromCamera( Camera *trackedCamera )
    {
        mTrackedPosition = trackedCamera->getDerivedPosition();
        mTrackedViewProjMatrix = trackedCamera->getProjectionMatrix() * trackedCamera->getViewMatrix();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::setEnabled( bool bEnabled, uint32 maxWidth,
                                               uint32 maxHeight, PixelFormatGpu pixelFormat )
    {
        if( bEnabled == getEnabled() )
            return;

        if( bEnabled )
        {
            TextureGpuManager *textureGpuManager =
                    mSceneManager->getDestinationRenderSystem()->getTextureGpuManager();
            mBindTexture = textureGpuManager->createTexture(
                               "ParallaxCorrectedCubemap Blend Result " +
                               StringConverter::toString( getId() ),
                               GpuPageOutStrategy::Discard,
                               TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps,
                               TextureTypes::TypeCube );
            mBindTexture->setResolution( maxWidth, maxHeight );
            mBindTexture->setPixelFormat( pixelFormat );
            mBindTexture->setNumMipmaps( PixelFormatGpuUtils::getMaxMipmapCount( maxWidth,
                                                                                 maxHeight ) );
            mBindTexture->_setDepthBufferDefaults( DepthBuffer::POOL_NO_DEPTH, false, PFG_UNKNOWN );
            mBindTexture->_transitionTo( GpuResidency::Resident, (uint8*)0 );

            createCubemapBlendWorkspace();

            mRoot->addFrameListener( this );
            CompositorManager2 *compositorManager = mDefaultWorkspaceDef->getCompositorManager();
            compositorManager->addListener( this );

            HlmsManager *hlmsManager = mRoot->getHlmsManager();
            OGRE_ASSERT_HIGH( dynamic_cast<HlmsPbs *>( hlmsManager->getHlms( HLMS_PBS ) ) );
            HlmsPbs *hlmsPbs = static_cast<HlmsPbs *>( hlmsManager->getHlms( HLMS_PBS ) );
            hlmsPbs->_notifyIblSpecMipmap( mBindTexture->getNumMipmaps() );
        }
        else
        {
            destroyCompositorData();

            CompositorManager2 *compositorManager = mDefaultWorkspaceDef->getCompositorManager();
            compositorManager->removeListener( this );

            mRoot->removeFrameListener( this );
        }
    }
    //-----------------------------------------------------------------------------------
    bool ParallaxCorrectedCubemap::getEnabled(void) const
    {
        return mBlendWorkspace != 0;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::createProxyGeometry(void)
    {
        //Create the mesh geometry
        const Vector3 c_vertices[8] =
        {
            Vector3( -1, -1,  1 ), Vector3(  1, -1,  1 ),
            Vector3(  1,  1,  1 ), Vector3( -1,  1,  1 ),
            Vector3( -1, -1, -1 ), Vector3(  1, -1, -1 ),
            Vector3(  1,  1, -1 ), Vector3( -1,  1, -1 )
        };
        const uint16 c_indexData[3 * 2 * 6] =
        {
            2, 1, 0, 0, 3, 2, //Front face
            4, 5, 6, 6, 7, 4, //Back face

            6, 2, 3, 3, 7, 6, //Top face
            0, 1, 5, 5, 4, 0, //Bottom face

            3, 0, 4, 4, 7, 3, //Left face
            1, 2, 6, 6, 5, 1, //Right face
        };

        VertexElement2Vec vertexElements;
        vertexElements.push_back( VertexElement2( VET_FLOAT3, VES_POSITION ) );

        VaoManager *vaoManager = mSceneManager->getDestinationRenderSystem()->getVaoManager();
        VertexBufferPacked *vertexBuffer = vaoManager->createVertexBuffer( vertexElements, 8,
                                                                           BT_IMMUTABLE,
                                                                           (void*)c_vertices, false );
        IndexBufferPacked *indexBuffer = vaoManager->createIndexBuffer( IndexBufferPacked::IT_16BIT,
                                                                        3 * 2 * 6, BT_IMMUTABLE,
                                                                        (void*)c_indexData, false );

        VertexBufferPackedVec vertexBuffers( 1, vertexBuffer );
        VertexArrayObject *vao = vaoManager->createVertexArrayObject( vertexBuffers, indexBuffer,
                                                                      OT_TRIANGLE_LIST );
        mProxyMesh = MeshManager::getSingleton().createManual(
                    "AutoGen_ParallaxCorrectedCubemap_" + StringConverter::toString( getId() ) +
                    "_Proxy", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );
        mProxyMesh->_setBounds( Aabb( Vector3::ZERO, Vector3::UNIT_SCALE ) );

        SubMesh *subMesh = mProxyMesh->createSubMesh();
        for( int i=0; i<NumVertexPass; ++i )
            subMesh->mVao[i].push_back( vao );
        subMesh->mMaterialName = "Cubemap/BlendProjectCubemap0";

        RenderQueue *renderQueue = mSceneManager->getRenderQueue();
        renderQueue->setRenderQueueMode( mReservedRqId, RenderQueue::FAST );
        renderQueue->setSortRenderQueue( mReservedRqId, RenderQueue::DisableSort );

        createProxyItems();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::destroyProxyGeometry(void)
    {
        destroyProxyItems();

        if( !mProxyMesh.isNull() )
        {
            MeshManager::getSingleton().remove( mProxyMesh->getHandle() );
            mProxyMesh.setNull();
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::createCubemapBlendWorkspaceDefinition(void)
    {
        String workspaceName = "AutoGen_ParallaxCorrectedCubemapBlending_Workspace";
        CompositorManager2 *compositorManager = mDefaultWorkspaceDef->getCompositorManager();
        CompositorWorkspaceDef *workspaceDef =
                compositorManager->getWorkspaceDefinitionNoThrow( workspaceName );
        if( !workspaceDef )
        {
            //Create blending workspace definition (blending multiple probes)
            CompositorNodeDef *nodeDef = compositorManager->addNodeDefinition(
                        "AutoGen_ParallaxCorrectedCubemapBlending_Node" );
            //Input texture
            nodeDef->addTextureSourceName( "BlendedProbeRT", 0, TextureDefinitionBase::TEXTURE_INPUT );
            nodeDef->setNumTargetPass( 6 );

            for( uint32 i=0; i<6; ++i )
            {
                CompositorTargetDef *targetDef = nodeDef->addTargetPass( "BlendedProbeRT", i );
                targetDef->setNumPasses( i == 5 ? 2 : 1 );
                {
                    {
                        CompositorPassSceneDef *passScene = static_cast<CompositorPassSceneDef*>
                                                                ( targetDef->addPass( PASS_SCENE ) );
                        passScene->mIdentifier = 10u + i;
                        passScene->mCameraCubemapReorient = true;
                        passScene->mFirstRQ = mReservedRqId;
                        passScene->mLastRQ  = mReservedRqId + 1u;
                        passScene->mEnableForwardPlus = false;
                        passScene->mIncludeOverlays = false;
                        passScene->mVisibilityMask  = mProxyVisibilityMask;

                        passScene->mLoadActionColour[0] = LoadAction::Clear;
                        passScene->mClearColour[0]      = ColourValue::Black;
                    }
                    if( i == 5 )
                    {
                        targetDef->addPass( PASS_MIPMAP );
                    }
                }
            }

            CompositorWorkspaceDef *workDef = compositorManager->addWorkspaceDefinition( workspaceName );
            workDef->connectExternal( 0, nodeDef->getName(), 0 );

            //Create copy workspace definition (just one probe)
            workspaceName = "AutoGen_ParallaxCorrectedCubemapCopy_Workspace";
            nodeDef = compositorManager->addNodeDefinition(
                        "AutoGen_ParallaxCorrectedCubemapCopy_Node" );
            //Input texture
            nodeDef->addTextureSourceName( "CopyProbeRT", 0, TextureDefinitionBase::TEXTURE_INPUT );
            nodeDef->setNumTargetPass( 6 );

            char tmpBuffer[64];
            LwString materialName( LwString::FromEmptyPointer( tmpBuffer, sizeof(tmpBuffer) ) );
            materialName = "Cubemap/CopyCubemap_";
            const size_t matNameSize = materialName.size();

            for( uint32 i=0; i<6; ++i )
            {
                CompositorTargetDef *targetDef = nodeDef->addTargetPass( "CopyProbeRT", i );
                targetDef->setNumPasses( i == 5 ? 2 : 1 );
                {
                    {
                        CompositorPassQuadDef *passQuad = static_cast<CompositorPassQuadDef*>
                                                                ( targetDef->addPass( PASS_QUAD ) );
                        materialName.resize( matNameSize );
                        materialName.a( cSuffixes[i] );
                        passQuad->mIdentifier = 10u + i;
                        passQuad->mMaterialName = materialName.c_str();

                        passQuad->mLoadActionColour[0]  = LoadAction::Clear;
                        passQuad->mClearColour[0]       = ColourValue::Black;
                    }
                    if( i == 5 )
                    {
                        targetDef->addPass( PASS_MIPMAP );
                    }
                }
            }

            workDef = compositorManager->addWorkspaceDefinition( workspaceName );
            workDef->connectExternal( 0, nodeDef->getName(), 0 );


            //Create clear workspace definition (to clear dirty probes)
            workspaceName = "AutoGen_ParallaxCorrectedCubemapClear_Workspace";
            nodeDef = compositorManager->addNodeDefinition(
                        "AutoGen_ParallaxCorrectedCubemapClear_Node" );
            //Input texture
            nodeDef->addTextureSourceName( "ProbeRT", 0, TextureDefinitionBase::TEXTURE_INPUT );
            nodeDef->setNumTargetPass( 6 );

            for( uint32 i=0; i<6; ++i )
            {
                CompositorTargetDef *targetDef = nodeDef->addTargetPass( "ProbeRT", i );
                targetDef->setNumPasses( 1 );
                {
                    {
                        CompositorPassClearDef *passClear = static_cast<CompositorPassClearDef*>
                                                                ( targetDef->addPass( PASS_CLEAR ) );
                        passClear->mClearColour[0] = ColourValue::Black;
                    }
                }
            }

            workDef = compositorManager->addWorkspaceDefinition( workspaceName );
            workDef->connectExternal( 0, nodeDef->getName(), 0 );
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::createCubemapBlendWorkspace(void)
    {
        mBlendProxyCamera = mSceneManager->createCamera( "ParallaxCorrectedCubemap for blending " +
                                                         StringConverter::toString( getId() ),
                                                         false );
        mBlendProxyCamera->setFOVy( Degree(90) );
        mBlendProxyCamera->setAspectRatio( 1 );
        mBlendProxyCamera->setFixedYawAxis(false);
        mBlendProxyCamera->setNearClipDistance( 0.01 );
        mBlendProxyCamera->setFarClipDistance( 0.0 );

        CompositorChannelVec channels( 1, mBindTexture );

        IdString workspaceName( "AutoGen_ParallaxCorrectedCubemapBlending_Workspace" );

        CompositorManager2 *compositorManager = mDefaultWorkspaceDef->getCompositorManager();
        mBlendWorkspace = compositorManager->addWorkspace( mSceneManager,
                                                           channels,
                                                           mBlendProxyCamera,
                                                           workspaceName,
                                                           false );
        mBlendWorkspace->setListener( this );

        workspaceName = "AutoGen_ParallaxCorrectedCubemapCopy_Workspace";
        mCopyWorkspace = compositorManager->addWorkspace( mSceneManager,
                                                          channels,
                                                          mBlendProxyCamera,
                                                          workspaceName,
                                                          false );
        mCopyWorkspace->setListener( this );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::destroyCompositorData(void)
    {
        mBlendWorkspace->setListener( 0 );
        CompositorManager2 *compositorManager = mDefaultWorkspaceDef->getCompositorManager();
        compositorManager->removeWorkspace( mBlendWorkspace );
        mBlendWorkspace = 0;

        mSceneManager->destroyCamera( mBlendProxyCamera );
        mBlendProxyCamera = 0;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::calculateBlendFactors(void)
    {
        assert( mNumCollectedProbes < OGRE_MAX_CUBE_PROBES );

        for( size_t i=mNumCollectedProbes; i<OGRE_MAX_CUBE_PROBES; ++i )
            mProbeBlendFactors[i] = 0;

        if( mNumCollectedProbes <= 1 )
        {
            mProbeBlendFactors[0] = 1.0f;
            return;
        }

        //See Sebastien Lagarde "Local Image-based Lighting With Parallax-corrected Cubemap"
        //https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/
        //https://seblagarde.wordpress.com/2012/11/28/siggraph-2012-talk/

        //There he explains:
        // Primitive have a normalized distance function which is 0 at center and 1 at boundary
        // When blending multiple primitive, we want the following constraint to be respect:
        // A - 100% (full weight) at center of primitive whatever the number of primitive overlapping
        // B - 0% (zero weight) at boundary of primitive whatever the number of primitive overlapping
        // For this we calc two weight and modulate them.
        // Weight0 is calc with NDF and allow to respect constraint B
        // Weight1 is calc with inverse NDF, which is (1 - NDF) and allow to respect constraint A
        // What enforce the constraint is the special case of 0 which once multiply by another value is 0.
        // For Weight 0, the 0 will enforce that boundary is always at 0%, but center will not always be 100%
        // For Weight 1, the 0 will enforce that center is always at 100%, but boundary will not always be 0%
        // Modulate weight0 and weight1 then renormalizing will allow to respects A and B at the same time.
        // The in between is not linear but give a pleasant result.
        // In practice the algorithm fail to avoid popping when leaving inner range of a primitive
        // which is include in at least 2 other primitives.
        // As this is a rare case, we do with it.

        //This will allow us to blend between the cubemaps. Notes:
        //  * What he calls "inverse NDF" we call "reverse NDF"
        //  * The math has been slightly changed but still has mathematical equivalent results.

        Real sumNdf = 0.0;

        for( size_t i=0; i<mNumCollectedProbes; ++i )
            sumNdf += mProbeNDFs[i];

        const Real invSumNdf = 1.0 / sumNdf;

        const Real reverseSumNdf = mNumCollectedProbes - sumNdf;
        const Real invRevSumNdf = 1.0 / reverseSumNdf;

        Real sumBlendFactor = 0;

        // "Weight0 = normalized NDF, inverted to have 1 at center, 0 at boundary.
        // And as we invert, we need to divide by Num-1 to stay normalized (else sum is > 1).
        // respect constraint B.
        // Weight1 = normalized inverted NDF, so we have 1 at center, 0 at boundary
        // and respect constraint A."
        for( size_t i=0; i<mNumCollectedProbes; ++i )
        {
            mProbeBlendFactors[i] = 1.0f - (mProbeNDFs[i] * invSumNdf);
            mProbeBlendFactors[i] *= (1.0f - mProbeNDFs[i]) * invRevSumNdf;
            sumBlendFactor += mProbeBlendFactors[i];
        }

        if( sumBlendFactor <= 0.0 )
            sumBlendFactor = 1.0f;

        Real invSumBlendFactor = 1.0 / sumBlendFactor;

        for( size_t i=0; i<mNumCollectedProbes; ++i )
            mProbeBlendFactors[i] *= invSumBlendFactor;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::setFinalProbeTo( size_t probeIdx )
    {
        mFinalProbe.mProbeCameraPos = mCollectedProbes[probeIdx]->mProbeCameraPos;
        mFinalProbe.mArea = mCollectedProbes[probeIdx]->mArea;
        mFinalProbe.mAreaInnerRegion = mCollectedProbes[probeIdx]->mAreaInnerRegion;
        mFinalProbe.mOrientation = mCollectedProbes[probeIdx]->mOrientation;
        mFinalProbe.mInvOrientation = mCollectedProbes[probeIdx]->mInvOrientation;
        mFinalProbe.mProbeShape = mCollectedProbes[probeIdx]->mProbeShape;

        const bool requiresTrilinear = mCollectedProbes[probeIdx]->mTexture->getNumMipmaps() !=
                                                             mBindTexture->getNumMipmaps();
        for( size_t i=0; i<6; ++i )
        {
            mCopyCubemapTUs[i]->setTexture( mCollectedProbes[probeIdx]->mTexture );
            mCopyCubemapTUs[i]->_setSamplerblock( requiresTrilinear ? mSamplerblockTrilinear :
                                                                      mSamplerblockPoint );
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::checkStagingBufferIsBigEnough(void)
    {
        //Check the staging buffer is big enough to avoid a stall
        VaoManager *vaoManager = mSceneManager->getDestinationRenderSystem()->getVaoManager();
        const size_t neededBytes = mManuallyActiveProbes.size() * getConstBufferSize() *
                vaoManager->getDynamicBufferMultiplier() * std::max<size_t>( 1u,
                                                                             mLastPassNumViewMatrices );

        if( (!mStagingBuffer && !mManuallyActiveProbes.empty()) ||
            neededBytes > mStagingBuffer->getMaxSize() )
        {
            if( mStagingBuffer )
                mStagingBuffer->removeReferenceCount();

            mStagingBuffer = vaoManager->getStagingBuffer( neededBytes, true );
        }

        mLastPassNumViewMatrices = 0;
        mCachedLastViewMatrix = Matrix4::ZERO;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::findClosestProbe(void)
    {
        //When we're not inside of any probe, select the 'closest' one.
        //We do that by projecting the AABBs into the camera and determining
        //which AABBs occupy more visible volume (approximation)
        CubemapProbeVec::iterator itor = mProbes.begin();
        CubemapProbeVec::iterator end  = mProbes.end();

        const uint32 systemMask = mMask;

        Matrix4 viewProjMatrix = mTrackedViewProjMatrix;

        AxisAlignedBox axisAlignedBox;

        mProbeNDFs[0] = 0;

        while( itor != end )
        {
            CubemapProbe *probe = *itor;

            if( probe->mEnabled && (probe->mMask & systemMask) )
            {
                Vector3 vMin = probe->mArea.getMinimum();
                Vector3 vMax = probe->mArea.getMaximum();
                axisAlignedBox.setExtents( vMin, vMax );

                const Vector3 *corners = axisAlignedBox.getAllCorners();

                Vector3 psMin = Vector3::UNIT_SCALE, psMax = -Vector3::UNIT_SCALE;

                for( int i=0; i<8; ++i )
                {
                    Vector4 transformedVert( probe->mOrientation * corners[i] );
                    transformedVert = viewProjMatrix * transformedVert;
                    transformedVert.w = Ogre::max( transformedVert.w, Real(1e-6f) );
                    transformedVert /= transformedVert.w;

                    Vector3 psVertex( transformedVert.ptr() );
                    psVertex.makeFloor( Vector3::UNIT_SCALE );
                    psVertex.makeCeil( -Vector3::UNIT_SCALE );

                    psMin.makeFloor( psVertex );
                    psMax.makeCeil( psVertex );
                }

                //Check the probe isn't behind us.
                if( psMax.z > -1.0f )
                {
                    //psMin.z is in range [-1; 1]; bring it to range [0; 1] and square it
                    //in an attempt to fight Z's behavior (Z grows a lot when close to,
                    //camera grows little when far from camera)
                    psMin.z = psMin.z * 0.5f + 0.5f;
                    psMax.z = psMax.z * 0.5f + 0.5f;
                    psMin.z *= psMin.z;
                    psMax.z *= psMax.z;
                    Real area = (psMax.x - psMin.x) * (psMax.y - psMin.y) * (psMax.z - psMin.z);

                    if( area > mProbeNDFs[0] )
                    {
                        mProbeNDFs[0]       = area;
                        mCollectedProbes[0] = probe;
                        mNumCollectedProbes = 1;
                    }
                }
            }

            ++itor;
        }

        if( mNumCollectedProbes == 0 )
            mProbeNDFs[0] = std::numeric_limits<Real>::max();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::updateSceneGraph(void)
    {
        const uint32 prevNumCollectedProbes = mNumCollectedProbes;
        const CubemapProbe *prevProbe = mCollectedProbes[0];

        mCurrentMip = 0;

        for( size_t i=0; i<OGRE_MAX_CUBE_PROBES; ++i )
        {
            mCollectedProbes[i] = 0;
            mProbeNDFs[i] = std::numeric_limits<Real>::max();
        }

        mNumCollectedProbes = 0;

        CubemapProbeVec::iterator itor = mProbes.begin();
        CubemapProbeVec::iterator end  = mProbes.end();

        const uint32 systemMask = mMask;

        while( itor != end )
        {
            CubemapProbe *probe = *itor;

            const Vector3 posLS = probe->mInvOrientation * (mTrackedPosition - probe->mArea.mCenter);
            const Aabb areaLS = probe->getAreaLS();
            if( areaLS.contains( posLS ) && probe->mEnabled && (probe->mMask & systemMask) )
            {
                const Real ndf = probe->getNDF( posLS );

                if( ndf > 0 )
                {
                    //Collect this probe, ensuring we collect the ones with the lowest NDF.
                    uint32 probeIdx = mNumCollectedProbes;

                    if( mNumCollectedProbes >= OGRE_MAX_CUBE_PROBES )
                    {
                        Real highestNdf = -1;
                        int highestNdfIdx = OGRE_MAX_CUBE_PROBES;

                        //Drop the probe with the highest NDF (note: we may drop this probe)
                        for( size_t i=0; i<OGRE_MAX_CUBE_PROBES; ++i )
                        {
                            if( ndf < mProbeNDFs[i] && mProbeNDFs[i] >= highestNdf )
                            {
                                highestNdf = mProbeNDFs[i];
                                highestNdfIdx = i;
                            }
                        }

                        probeIdx = highestNdfIdx;
                    }

                    if( probeIdx < OGRE_MAX_CUBE_PROBES )
                    {
                        mProbeNDFs[probeIdx]        = ndf;
                        mCollectedProbes[probeIdx]  = probe;
                        ++mNumCollectedProbes;
                        mNumCollectedProbes = std::min( mNumCollectedProbes, OGRE_MAX_CUBE_PROBES );
                    }
                }
                else
                {
                    //Early out. Use ONLY this probe.
                    mProbeNDFs[0]       = ndf;
                    mCollectedProbes[0] = probe;
                    mNumCollectedProbes = 1;
                    itor = end;
                    break;
                }
            }

            ++itor;
        }

        if( mNumCollectedProbes == 0 )
            findClosestProbe();

        for( size_t i=mNumCollectedProbes; i<OGRE_MAX_CUBE_PROBES; ++i )
            mCollectedProbes[i] = &mBlankProbe;

        calculateBlendFactors();

        {
            size_t highestIdx = 0;
            //Find the probe with the highest weight and make that one the main one.
            for( size_t i=1; i<mNumCollectedProbes; ++i )
            {
                if( mProbeBlendFactors[i] > mProbeBlendFactors[highestIdx] )
                    highestIdx = i;
            }

            if( highestIdx != 0 )
            {
                std::swap( mProbeBlendFactors[0], mProbeBlendFactors[highestIdx] );
                std::swap( mCollectedProbes[0], mCollectedProbes[highestIdx] );
                std::swap( mProbeNDFs[0], mProbeNDFs[highestIdx] );
            }
        }

        //TODO: Update could be done over several frames.
        for( size_t i=0; i<mNumCollectedProbes; ++i )
            mCollectedProbes[i]->_prepareForRendering();

        for( size_t i=0; i<OGRE_MAX_CUBE_PROBES; ++i )
        {
            const Quaternion qRot( mCollectedProbes[i]->mOrientation );
            mProxyNodes[i]->setPosition( mCollectedProbes[i]->mProbeShape.mCenter );
            mProxyNodes[i]->setScale( mCollectedProbes[i]->mProbeShape.mHalfSize );
            mProxyNodes[i]->setOrientation( qRot );
            mProxyItems[i]->setVisible( i < mNumCollectedProbes );

            //Divide by maxComponent to get better precision in the GPU.
            const Real maxComponent = Ogre::max( mCollectedProbes[i]->mProbeShape.mHalfSize.x,
                                                 Ogre::max(
                                                     mCollectedProbes[i]->mProbeShape.mHalfSize.y,
                                                     mCollectedProbes[i]->mProbeShape.mHalfSize.z ) );
            Matrix4 worldScaledMatrix;
            worldScaledMatrix.makeTransform( mCollectedProbes[i]->mProbeShape.mCenter / maxComponent,
                                             mCollectedProbes[i]->mProbeShape.mHalfSize / maxComponent,
                                             mCollectedProbes[i]->mOrientation );
            const Vector3 probeCameraPosScaled = mCollectedProbes[i]->mProbeCameraPos / maxComponent;

            mBlendCubemapParamsVs[i]->setNamedConstant( "worldScaledMatrix", worldScaledMatrix[0], 3 );
            mBlendCubemapParamsVs[i]->setNamedConstant( "probeCameraPosScaled", probeCameraPosScaled );
            mBlendCubemapParams[i]->setNamedConstant( "weight", mProbeBlendFactors[i] );

            const bool requiresTrilinear = mCollectedProbes[i]->mTexture->getNumMipmaps() !=
                                                              mBindTexture->getNumMipmaps();
            mBlendCubemapTUs[i]->setTexture( mCollectedProbes[i]->mTexture );
            mBlendCubemapTUs[i]->_setSamplerblock( requiresTrilinear ? mSamplerblockTrilinear :
                                                                       mSamplerblockPoint );
        }

        //Project p0ToCam onto p0ToP1.
//        Vector3 p0ToCam = mTrackedPosition - mCollectedProbes[0]->mProbeCameraPos;
//        Vector3 p0ToP1 = mCollectedProbes[1]->mProbeCameraPos - mCollectedProbes[0]->mProbeCameraPos;
//        p0ToP1.normalise();
//        Vector3 finalPos = p0ToP1 * p0ToP1.dotProduct( p0ToCam ) + mCollectedProbes[0]->mProbeCameraPos;

        Vector3 finalPos = mCollectedProbes[0]->mProbeCameraPos;

        if( mNumCollectedProbes > 1 )
        {
            //When mNumCollectedProbes = 2
            //mProbeBlendFactors[0] is in range [0.5; 1] so we need to move it to range [0; 1]
            //When mNumCollectedProbes = 3
            //mProbeBlendFactors[0] is in range [0.333; 1] so we need to move it to range [0; 1]
            //When mNumCollectedProbes = 4
            //mProbeBlendFactors[0] is in range [0.25; 1] so we need to move it to range [0; 1]
            finalPos = Math::lerp( mTrackedPosition, finalPos,
                                   mProbeBlendFactors[0] * mNumCollectedProbes - Real(1.0) );
        }

        //TODO: restrict mTrackedPosition to a region between the 4 probes.
        const Quaternion qRot( mCollectedProbes[0]->mOrientation );
        mBlendProxyCamera->setPosition( finalPos );
        //mBlendProxyCamera->setPosition( mTrackedPosition );
        mBlendProxyCamera->setOrientation( qRot );

        mBlendedProbeNeedsUpdate = !(prevNumCollectedProbes <= 1 &&
                                     mNumCollectedProbes <= 1 &&
                                     prevNumCollectedProbes == mNumCollectedProbes &&
                                     mCollectedProbes[0] == prevProbe );

        if( !mManuallyActiveProbes.empty() )
            checkStagingBufferIsBigEnough();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::updateExpensiveCollectedDirtyProbes( uint16 iterationThreshold )
    {
        RenderSystem *renderSystem = mSceneManager->getDestinationRenderSystem();
        HlmsManager *hlmsManager = mRoot->getHlmsManager();

        const uint32 oldVisibilityMask = mSceneManager->getVisibilityMask();
        mSceneManager->setVisibilityMask( 0xffffffff );

        for( size_t i=0; i<mNumCollectedProbes; ++i )
        {
            if( (mCollectedProbes[i]->mDirty || !mCollectedProbes[i]->mStatic) &&
                mCollectedProbes[i]->mNumIterations > iterationThreshold )
            {
                setFinalProbeTo( i );

                for( int j=0; j<mCollectedProbes[i]->mNumIterations; ++j )
                {
                    renderSystem->_beginFrameOnce();
                    mCopyWorkspace->_beginUpdate( true );
                        if( j == 0 )
                            mCollectedProbes[i]->_clearCubemap();
                        mCopyWorkspace->_update();
                        mCollectedProbes[i]->_updateRender();
                    mCopyWorkspace->_endUpdate( true );

                    renderSystem->_update();
                    renderSystem->_endFrameOnce();

                    mCurrentMip = 0;
                }

                mCollectedProbes[i]->mDirty = false;
                mBlendedProbeNeedsUpdate = true;
            }
        }

        mSceneManager->setVisibilityMask( oldVisibilityMask );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::updateRender(void)
    {
        const uint32 oldVisibilityMask = mSceneManager->getVisibilityMask();
        mSceneManager->setVisibilityMask( 0xffffffff );

        for( size_t i=0; i<mNumCollectedProbes; ++i )
        {
            if( mCollectedProbes[i]->mDirty || !mCollectedProbes[i]->mStatic )
            {
                setFinalProbeTo( i );

                mCollectedProbes[i]->_clearCubemap();

                for( int j=0; j<mCollectedProbes[i]->mNumIterations; ++j )
                {
                    mCopyWorkspace->_update();
                    mCollectedProbes[i]->_updateRender();
                    mCurrentMip = 0;
                }

                mCollectedProbes[i]->mDirty = false;
                mBlendedProbeNeedsUpdate = true;
            }
        }

        setFinalProbeTo( 0 );

        for( size_t i=1; i<mNumCollectedProbes; ++i )
        {
            //When mNumCollectedProbes = 2
            //mProbeBlendFactors[1] is in range [0; 0.5] so we need to move it to range [0; 1]
            //When mNumCollectedProbes = 3
            //mProbeBlendFactors[1] is in range [0.333; 0.5] so we need to move it to range [0; 1]
            //mProbeBlendFactors[2] is in range [0; 0.333] so we need to move it to range [0; 1]
            //When mNumCollectedProbes = 4
            //mProbeBlendFactors[1] is in range [0.25; 0.5] so we need to move it to range [0; 1]
            //mProbeBlendFactors[2] is in range [0.25; 0.333] so we need to move it to range [0; 1]
            //mProbeBlendFactors[3] is in range [0; 0.25] so we need to move it to range [0; 1]
            Real weight = mProbeBlendFactors[i] * (i+1);
            if( i != mNumCollectedProbes - 1u )
                weight -= Real(i+1) / (Real)mNumCollectedProbes;

            Aabb mergedProbe = mFinalProbe.mProbeShape;
            mergedProbe.merge( mCollectedProbes[i]->mProbeShape );
            Vector3 newMin = Math::lerp( mFinalProbe.mProbeShape.getMinimum(),
                                         mergedProbe.getMinimum(),
                                         weight * weight * weight );
            Vector3 newMax = Math::lerp( mFinalProbe.mProbeShape.getMaximum(),
                                         mergedProbe.getMaximum(),
                                         weight * weight * weight );
            mFinalProbe.mProbeShape.setExtents( newMin, newMax );
        }

        //Warning to other devs: Don't mess with mFinalProbe.mProbeCameraPos here.
        //Change mBlendProxyCamera->setPosition instead in updateSceneGraph.
        mFinalProbe.mProbeCameraPos = mBlendProxyCamera->getPosition();

        if( mBlendedProbeNeedsUpdate )
        {
            if( mNumCollectedProbes == 1u )
                mCopyWorkspace->_update();
            else if( mNumCollectedProbes > 1u )
                mBlendWorkspace->_update();
        }

        mSceneManager->setVisibilityMask( oldVisibilityMask );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::updateAllDirtyProbes(void)
    {
        mSceneManager->updateSceneGraph();

        const uint32 systemMask = mMask;

        CubemapProbeVec::const_iterator itor = mProbes.begin();
        CubemapProbeVec::const_iterator end  = mProbes.end();

        while( itor != end )
        {
            if( (*itor)->mEnabled && ((*itor)->mMask & systemMask) )
            {
                mTrackedPosition = (*itor)->mProbeCameraPos;
                this->updateSceneGraph();
                for( size_t i=0; i<mNumCollectedProbes; ++i )
                    mProxyNodes[i]->_getFullTransformUpdated();
                this->updateExpensiveCollectedDirtyProbes( 0 );
            }
            ++itor;
        }

        mSceneManager->clearFrameData();

        //Set to 0 so next time mBlendedProbeNeedsUpdate will be set to true correctly;
        mNumCollectedProbes = 0;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::_notifyPreparePassHash( const Matrix4 &viewMatrix )
    {
        if( !mManuallyActiveProbes.empty() && viewMatrix != mCachedLastViewMatrix )
        {
            Matrix3 invViewMat3;
            viewMatrix.extract3x3Matrix( invViewMat3 );
            invViewMat3 = invViewMat3.Inverse();

            const size_t neededBytes = mManuallyActiveProbes.size() * getConstBufferSize();

            StagingBuffer::DestinationVec destinations;

            float * RESTRICT_ALIAS probeData = static_cast<float * RESTRICT_ALIAS>(
                        mStagingBuffer->map( neededBytes ) );
            size_t srcOffset = 0;

            CubemapProbeVec::const_iterator itor = mManuallyActiveProbes.begin();
            CubemapProbeVec::const_iterator end  = mManuallyActiveProbes.end();

            while( itor != end )
            {
                destinations.push_back( StagingBuffer::Destination( (*itor)->mConstBufferForManualProbes,
                                                                    0, srcOffset,
                                                                    getConstBufferSize() ) );

                ParallaxCorrectedCubemapBase::
                        fillConstBufferData( **itor, viewMatrix, invViewMat3, probeData );
                probeData += getConstBufferSize() >> 2u;
                srcOffset += getConstBufferSize();

                ++itor;
            }

            mStagingBuffer->unmap( destinations );

            mCachedLastViewMatrix = viewMatrix;
            ++mLastPassNumViewMatrices;
        }
    }
    //-----------------------------------------------------------------------------------
    size_t ParallaxCorrectedCubemap::getConstBufferSize(void)
    {
        return getConstBufferSizeStatic();
    }
    //-----------------------------------------------------------------------------------
    size_t ParallaxCorrectedCubemap::getConstBufferSizeStatic(void)
    {
        return 6u * 4u * sizeof(float); //CubemapProbe localProbe;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::fillConstBufferData( const Matrix4 &viewMatrix,
                                                        float * RESTRICT_ALIAS passBufferPtr ) const
    {
        Matrix3 invViewMat3;
        viewMatrix.extract3x3Matrix( invViewMat3 );
        invViewMat3 = invViewMat3.Inverse();
        ParallaxCorrectedCubemapBase::
                fillConstBufferData( mFinalProbe, viewMatrix, invViewMat3, passBufferPtr );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *ParallaxCorrectedCubemap::findRtt( const TextureGpu *baseParams, TempRttVec &container,
                                                   uint32 textureFlags, bool fullMipmaps )
    {
        TextureGpu *retVal = 0;

        TempRttVec::iterator itor = container.begin();
        TempRttVec::iterator end = container.end();

        while( itor != end )
        {
            if( itor->texture->getWidth() == baseParams->getWidth() &&
                itor->texture->getHeight() == baseParams->getHeight() &&
                itor->texture->getPixelFormat() == baseParams->getPixelFormat() &&
                itor->texture->getMsaa() == baseParams->getMsaa() )
            {
                retVal = itor->texture;
                ++itor->refCount;
            }

            ++itor;
        }

        if( !retVal )
        {
            uint8 numMipmaps = PixelFormatGpuUtils::getMaxMipmapCount( baseParams->getWidth(),
                                                                       baseParams->getHeight() );
            if( !fullMipmaps )
                numMipmaps = std::max<uint8>( baseParams->getNumMipmaps(), 5u ) - 4u;

            TempRtt tmpRtt;
            TextureGpuManager *textureGpuManager =
                mSceneManager->getDestinationRenderSystem()->getTextureGpuManager();
            retVal = textureGpuManager->createTexture(
                "ParallaxCorrectedCubemap Temp RTT " + StringConverter::toString( getId() ),
                GpuPageOutStrategy::Discard, textureFlags, TextureTypes::TypeCube );
            retVal->setResolution( baseParams->getWidth(), baseParams->getHeight() );
            retVal->setPixelFormat( baseParams->getPixelFormat() );
            retVal->setNumMipmaps( numMipmaps );
            retVal->setMsaa( baseParams->getMsaa() );
            retVal->setMsaaPattern( baseParams->getMsaaPattern() );
            retVal->_transitionTo( GpuResidency::Resident, (uint8 *)0 );

            tmpRtt.texture = retVal;
            tmpRtt.refCount = 1;
            container.push_back( tmpRtt );
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::releaseRtt( const TextureGpu *rtt, TempRttVec &container )
    {
        TempRttVec::iterator itor = container.begin();
        TempRttVec::iterator end  = container.end();

        while( itor != end && itor->texture != rtt )
            ++itor;

        if( itor != end )
        {
            --itor->refCount;
            if( !itor->refCount )
            {
                TextureGpuManager *textureGpuManager =
                        mSceneManager->getDestinationRenderSystem()->getTextureGpuManager();
                textureGpuManager->destroyTexture( itor->texture );
                efficientVectorRemove( container, itor );
            }
        }
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* ParallaxCorrectedCubemap::findTmpRtt( const TextureGpu *baseParams )
    {
        return findRtt( baseParams, mTmpRtt,
                        TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps, true );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::releaseTmpRtt( const TextureGpu *tmpRtt )
    {
        releaseRtt( tmpRtt, mTmpRtt );
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* ParallaxCorrectedCubemap::findIbl( const TextureGpu *baseParams )
    {
        return findRtt( baseParams, mIblRtt, getIblTargetTextureFlags( baseParams->getPixelFormat() ),
                        false );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::releaseIbl( const TextureGpu *tmpRtt )
    {
        releaseRtt( tmpRtt, mIblRtt );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::_addManuallyActiveProbe( CubemapProbe *probe )
    {
        mManuallyActiveProbes.push_back( probe );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::_removeManuallyActiveProbe( CubemapProbe *probe )
    {
        CubemapProbeVec::iterator itor = std::find( mManuallyActiveProbes.begin(),
                                                    mManuallyActiveProbes.end(), probe );

        if( itor != mManuallyActiveProbes.end() )
            efficientVectorRemove( mManuallyActiveProbes, itor );

        if( mManuallyActiveProbes.empty() && mStagingBuffer )
        {
            mStagingBuffer->removeReferenceCount();
            mStagingBuffer = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::passPreExecute( CompositorPass *pass )
    {
        if( pass->getType() == PASS_SCENE && pass->getDefinition()->mIdentifier == 10u )
        {
            for( size_t i=0; i<OGRE_MAX_CUBE_PROBES; ++i )
            {
                const float mipLevel = ( mCurrentMip * mCollectedProbes[i]->mTexture->getNumMipmaps() ) /
                                       mBindTexture->getNumMipmaps();
                mBlendCubemapParams[i]->setNamedConstant( "lodLevel", mipLevel );
            }
            ++mCurrentMip;
        }
        else if( pass->getType() == PASS_QUAD && pass->getDefinition()->mIdentifier == 10u )
        {
            const float mipLevel = ( mCurrentMip * mCollectedProbes[0]->mTexture->getNumMipmaps() ) /
                                    mBindTexture->getNumMipmaps();
            for( size_t i=0; i<6; ++i )
                mCopyCubemapParams[i]->setNamedConstant( "lodLevel", mipLevel );
            ++mCurrentMip;
        }
        else
        {
            ParallaxCorrectedCubemapBase::passPreExecute( pass );
        }
    }
    //-----------------------------------------------------------------------------------
    bool ParallaxCorrectedCubemap::frameStarted( const FrameEvent& evt )
    {
        if( !mPaused )
            this->updateSceneGraph();
        return true;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::allWorkspacesBeforeBeginUpdate(void)
    {
        if( !mPaused )
            this->updateExpensiveCollectedDirtyProbes( 1 );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemap::allWorkspacesBeginUpdate(void)
    {
        if( !mPaused )
            this->updateRender();
    }
}
