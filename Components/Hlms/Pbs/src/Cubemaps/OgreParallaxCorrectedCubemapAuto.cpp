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

#include "Cubemaps/OgreParallaxCorrectedCubemapAuto.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuad.h"

#include "OgreBitwise.h"

#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreDepthBuffer.h"
#include "OgreTextureBox.h"

#include "OgreMaterialManager.h"
#include "OgreTechnique.h"
#include "OgreLwString.h"

#include "OgreMeshManager2.h"
#include "OgreMesh2.h"
#include "OgreSubMesh2.h"
#include "OgreItem.h"
#include "OgreInternalCubemapProbe.h"

#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreStagingBuffer.h"

namespace Ogre
{
    static uint32 c_cubmapToDpmIdentifier = 1863669;

    ParallaxCorrectedCubemapAuto::ParallaxCorrectedCubemapAuto(
            IdType id, Root *root, SceneManager *sceneManager,
            const CompositorWorkspaceDef *probeWorkspcDef ) :
        ParallaxCorrectedCubemapBase( id, root, sceneManager, probeWorkspcDef, true ),
        mTrackedPosition( Vector3::ZERO ),
        mRenderTarget( 0 ),
        mDpmRenderTarget( 0 ),
        mDpmCamera( 0 ),
        mCubeToDpmWorkspace( 0 )
    {
        const RenderSystemCapabilities *caps =
                mSceneManager->getDestinationRenderSystem()->getCapabilities();
        mUseDpm2DArray = !caps->hasCapability( RSC_TEXTURE_CUBE_MAP_ARRAY );
    }
    //-----------------------------------------------------------------------------------
    ParallaxCorrectedCubemapAuto::~ParallaxCorrectedCubemapAuto()
    {
        setEnabled( false, 0, 0, 0, PFG_UNKNOWN );
        destroyAllProbes();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::createCubemapToDpmWorkspaceDef(
            CompositorManager2 *compositorManager, TextureGpu *cubeTexture )
    {
        CompositorNodeDef *nodeDef =
                compositorManager->addNodeDefinition( "AutoGen_PccCubeToDpm_Node/" +
                                                      cubeTexture->getNameStr() );
        nodeDef->addTextureSourceName( "DpmDst", 0, TextureDefinitionBase::TEXTURE_INPUT );
        nodeDef->addTextureSourceName( "CubemapSrc", 1, TextureDefinitionBase::TEXTURE_INPUT );
        nodeDef->setNumTargetPass( 1 );

        CompositorTargetDef *targetDef = nodeDef->addTargetPass( "DpmDst" );
        targetDef->setNumPasses( 2u );

        CompositorPassQuadDef *passQuad = static_cast<CompositorPassQuadDef*>(
                                              targetDef->addPass( PASS_QUAD ) );
        passQuad->addQuadTextureSource( 0, "CubemapSrc" );
        passQuad->mMaterialName = "Ogre/DPM/CubeToDpm";
        passQuad->mIdentifier = c_cubmapToDpmIdentifier;
        passQuad->mLoadActionColour[0]  = LoadAction::DontCare;
        passQuad->mStoreActionColour[0] = StoreAction::Store;

        targetDef->addPass( PASS_MIPMAP );

        String workspaceName = "AutoGen_PccAutoCubeToDpm_Workspace/" + cubeTexture->getNameStr();
        CompositorWorkspaceDef *workDef = compositorManager->addWorkspaceDefinition( workspaceName );
        workDef->connectExternal( 0, nodeDef->getName(), 0 );
        workDef->connectExternal( 1, nodeDef->getName(), 1 );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::destroyCubemapToDpmWorkspaceDef(
            CompositorManager2 *compositorManager, TextureGpu *cubeTexture )
    {
        String workspaceName = "AutoGen_PccAutoCubeToDpm_Workspace/" + cubeTexture->getNameStr();
        compositorManager->removeWorkspaceDefinition( workspaceName );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::setUpdatedTrackedDataFromCamera( Camera *trackedCamera )
    {
        mTrackedPosition = trackedCamera->getDerivedPosition();
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* ParallaxCorrectedCubemapAuto::_acquireTextureSlot( uint32 &outTexSlot )
    {
        OGRE_ASSERT_LOW( mBindTexture && "Must call ParallaxCorrectedCubemapAuto::setEnabled first!" );

        vector<uint64>::type::iterator itor = mReservedSlotBitset.begin();
        vector<uint64>::type::iterator end  = mReservedSlotBitset.end();

        TextureGpu *retVal = 0;
        uint32 firstBitSet = 64u;
        while( itor != end && firstBitSet == 64u )
        {
            firstBitSet = Bitwise::ctz64( *itor );
            if( firstBitSet != 64u )
            {
                uint32 idx = static_cast<uint32>( itor - mReservedSlotBitset.begin() );
                outTexSlot = firstBitSet + 64u * idx;
                retVal = mBindTexture;
                *itor &= ~(((uint64)1ul) << firstBitSet);
            }
            ++itor;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::_releaseTextureSlot( TextureGpu *texture, uint32 texSlot )
    {
        OGRE_ASSERT_LOW( texture == mBindTexture );

        size_t idx  = texSlot / 64u;
        uint64 mask = texSlot % 64u;
        mask = ((uint64)1ul) << mask;

        OGRE_ASSERT_LOW( idx < mReservedSlotBitset.size() && "Slot is invalid. Out of bounds!" );
        OGRE_ASSERT_LOW( !(mReservedSlotBitset[idx] & mask) && "Slot was already released!" );
        mReservedSlotBitset[idx] |= texSlot;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* ParallaxCorrectedCubemapAuto::findTmpRtt( const TextureGpu *baseParams )
    {
        OGRE_ASSERT_LOW( mRenderTarget && "Must call ParallaxCorrectedCubemapAuto::setEnabled first!" );
        return mRenderTarget;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::releaseTmpRtt( const TextureGpu *tmpRtt )
    {
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::_copyRenderTargetToCubemap( uint32 cubemapArrayIdx )
    {
        TextureGpu *renderTarget = mRenderTarget;

        if( mUseDpm2DArray )
        {
            mCubeToDpmWorkspace->_update();
            renderTarget = mDpmRenderTarget;
        }
        else
            cubemapArrayIdx *= 6u;

        const uint8 numMipmaps = renderTarget->getNumMipmaps();
        for( uint8 mip=0; mip<numMipmaps; ++mip )
        {
            //srcBox.numSlices = 6, thus we ask the RenderSystem to copy all 6 slices in one call
            TextureBox srcBox = renderTarget->getEmptyBox( mip );
            TextureBox dstBox = srcBox;
            srcBox.sliceStart = 0;
            dstBox.sliceStart = cubemapArrayIdx;
            renderTarget->copyTo( mBindTexture, dstBox, mip, srcBox, mip );
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::setEnabled( bool bEnabled, uint32 width,
                                                   uint32 height, uint32 maxNumProbes,
                                                   PixelFormatGpu pixelFormat )
    {
        if( bEnabled == getEnabled() )
            return;

        TextureGpuManager *textureGpuManager =
                mSceneManager->getDestinationRenderSystem()->getTextureGpuManager();

        if( bEnabled )
        {
            //Cubemap target has no mipmaps when using DPM
            uint32 textureFlags = TextureFlags::RenderToTexture;

            if( !mUseDpm2DArray )
                textureFlags |= TextureFlags::AllowAutomipmaps;

            //Create Cubemap
            mRenderTarget =
                    textureGpuManager->createTexture(
                        "ParallaxCorrectedCubemapAuto Target " +
                        StringConverter::toString( getId() ),
                        GpuPageOutStrategy::Discard,
                        textureFlags,
                        TextureTypes::TypeCube );
            mRenderTarget->setResolution( width, height );
            mRenderTarget->setPixelFormat( pixelFormat );
            if( !mUseDpm2DArray )
                mRenderTarget->setNumMipmaps( PixelFormatGpuUtils::getMaxMipmapCount( width, height ) );
            else
                mRenderTarget->setNumMipmaps( 1u );
            mRenderTarget->_transitionTo( GpuResidency::Resident, (uint8*)0 );

            //Create temporary 2D texture to convert the cubemap -> Dual Paraboloid
            if( mUseDpm2DArray )
            {
                mDpmRenderTarget = textureGpuManager->createTexture(
                                       "ParallaxCorrectedCubemapAuto 2D DPM Target " +
                                       StringConverter::toString( getId() ),
                                       GpuPageOutStrategy::Discard,
                                       TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps,
                                       TextureTypes::Type2D );
                mDpmRenderTarget->setResolution( width << 1u, height << 1u );
                mDpmRenderTarget->setPixelFormat( pixelFormat );
                mDpmRenderTarget->setNumMipmaps(
                            PixelFormatGpuUtils::getMaxMipmapCount(
                                mDpmRenderTarget->getWidth(), mDpmRenderTarget->getHeight() ) );
                mDpmRenderTarget->_transitionTo( GpuResidency::Resident, (uint8*)0 );
            }

            //Create the actual texture to contain the reflections
            //that will be bound during normal render
            TextureTypes::TextureTypes bindTextureType = mUseDpm2DArray ? TextureTypes::Type2DArray :
                                                                          TextureTypes::TypeCubeArray;
            mBindTexture =
                    textureGpuManager->createTexture(
                        "ParallaxCorrectedCubemapAuto Array " +
                        StringConverter::toString( getId() ),
                        GpuPageOutStrategy::Discard,
                        TextureFlags::ManualTexture,
                        bindTextureType );
            if( mUseDpm2DArray )
                mBindTexture->setResolution( width << 1u, height << 1u, maxNumProbes );
            else
                mBindTexture->setResolution( width, height, maxNumProbes );
            mBindTexture->setPixelFormat( pixelFormat );
            mBindTexture->setNumMipmaps( PixelFormatGpuUtils::getMaxMipmapCount(
                                             mBindTexture->getWidth(), mBindTexture->getHeight() ) );
            mBindTexture->_transitionTo( GpuResidency::Resident, (uint8*)0 );

            const uint64 remainder = maxNumProbes % 64u;
            mReservedSlotBitset.resize( alignToNextMultiple( maxNumProbes, 64u ) >> 6u,
                                        0xffffffffffffffff );
            if( remainder != 0u )
                mReservedSlotBitset.back() = (1u << remainder) - 1u;

            mRoot->addFrameListener( this );
            CompositorManager2 *compositorManager = mDefaultWorkspaceDef->getCompositorManager();
            compositorManager->addListener( this );

            if( mUseDpm2DArray )
            {
                createCubemapToDpmWorkspaceDef( compositorManager, mRenderTarget );
                mDpmCamera = mSceneManager->createCamera( "Camera/" + mRenderTarget->getNameStr(),
                                                          false, false );
                mDpmCamera->setFOVy( Degree(90) );
                mDpmCamera->setAspectRatio( 1 );
                mDpmCamera->setFixedYawAxis(false);
                mDpmCamera->setNearClipDistance( 0.1f );
                mDpmCamera->setFarClipDistance( 1.0f );

                IdString workspaceName( "AutoGen_PccAutoCubeToDpm_Workspace/" +
                                        mRenderTarget->getNameStr() );
                CompositorChannelVec channels;
                channels.reserve( 2u );
                channels.push_back( mDpmRenderTarget );
                channels.push_back( mRenderTarget );
                mCubeToDpmWorkspace = compositorManager->addWorkspace( mSceneManager, channels,
                                                                       mDpmCamera, workspaceName,
                                                                       false );
                mCubeToDpmWorkspace->setListener( this );
            }

//            CubemapProbeVec::const_iterator itor = mProbes.begin();
//            CubemapProbeVec::const_iterator end  = mProbes.end();

//            while( itor != end )
//            {
//                if( (*itor)->isInitialized() )
//                    (*itor)->initWorkspace();
//                ++itor;
//            }
        }
        else
        {
            CompositorManager2 *compositorManager = mDefaultWorkspaceDef->getCompositorManager();
            compositorManager->removeListener( this );

            mRoot->removeFrameListener( this );

            CubemapProbeVec::const_iterator itor = mProbes.begin();
            CubemapProbeVec::const_iterator end  = mProbes.end();

            while( itor != end )
            {
                (*itor)->destroyWorkspace();
                ++itor;
            }

            mReservedSlotBitset.clear();

            if( mCubeToDpmWorkspace )
            {
                compositorManager->removeWorkspace( mCubeToDpmWorkspace );
                mCubeToDpmWorkspace = 0;
                destroyCubemapToDpmWorkspaceDef( compositorManager, mRenderTarget );
            }

            if( mDpmCamera )
            {
                mSceneManager->destroyCamera( mDpmCamera );
                mDpmCamera = 0;
            }

            if( mRenderTarget )
            {
                textureGpuManager->destroyTexture( mRenderTarget );
                mRenderTarget = 0;
            }
            if( mBindTexture )
            {
                textureGpuManager->destroyTexture( mBindTexture );
                mBindTexture = 0;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    bool ParallaxCorrectedCubemapAuto::getEnabled(void) const
    {
        return mRenderTarget != 0;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::setUseDpm2DArray( bool useDpm2DArray )
    {
        OGRE_ASSERT_LOW( !getEnabled() );
        mUseDpm2DArray = useDpm2DArray;
        const RenderSystemCapabilities *caps =
                mSceneManager->getDestinationRenderSystem()->getCapabilities();
        if( !caps->hasCapability( RSC_TEXTURE_CUBE_MAP_ARRAY ) )
            mUseDpm2DArray = true;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::updateSceneGraph(void)
    {
        mDirtyProbes.clear();

        const uint32 systemMask = mMask;

        CubemapProbeVec::iterator itor = mProbes.begin();
        CubemapProbeVec::iterator end  = mProbes.end();

        while( itor != end )
        {
            CubemapProbe *probe = *itor;

            const Vector3 posLS = probe->mInvOrientation * (mTrackedPosition - probe->mArea.mCenter);
            const Aabb areaLS = probe->getAreaLS();
            if( ((areaLS.contains( posLS ) && !probe->mStatic) || probe->mDirty) &&
                probe->mEnabled &&
                (probe->mMask & systemMask) )
            {
                mDirtyProbes.push_back( probe );
            }

            if( probe->mInternalProbe )
            {
                probe->mInternalProbe->setVisible( probe->mEnabled &&
                                                   (probe->mMask & systemMask) != 0u );
            }

            ++itor;
        }

        itor = mDirtyProbes.begin();
        end  = mDirtyProbes.end();

        while( itor != end )
        {
            (*itor)->_prepareForRendering();
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::updateExpensiveCollectedDirtyProbes( uint16 iterationThreshold )
    {
        RenderSystem *renderSystem = mSceneManager->getDestinationRenderSystem();
        HlmsManager *hlmsManager = mRoot->getHlmsManager();

        const uint32 oldVisibilityMask = mSceneManager->getVisibilityMask();
        mSceneManager->setVisibilityMask( 0xffffffff );

        CubemapProbeVec::iterator itor = mDirtyProbes.begin();
        CubemapProbeVec::iterator end  = mDirtyProbes.end();

        while( itor != end )
        {
            CubemapProbe *probe = *itor;

            if( probe->mNumIterations > iterationThreshold )
            {
                renderSystem->_beginFrameOnce();
                probe->_updateRender();
                mSceneManager->_frameEnded();
                for( size_t k=0; k<HLMS_MAX; ++k )
                {
                    Hlms *hlms = hlmsManager->getHlms( static_cast<HlmsTypes>( k ) );
                    if( hlms )
                        hlms->frameEnded();
                }

                renderSystem->_update();
                renderSystem->_endFrameOnce();

                probe->mDirty = false;

                if( iterationThreshold > 0u )
                {
                    //We have to remove only those that are no longer dirty
                    itor = efficientVectorRemove( mDirtyProbes, itor );
                    end  = mDirtyProbes.end();
                }
                else
                    ++itor;
            }
            else
            {
                ++itor;
            }
        }

        //When iterationThreshold == 0; we're updating every probe,
        //thus just clear them all at once
        if( iterationThreshold == 0u )
            mDirtyProbes.clear();

        mSceneManager->setVisibilityMask( oldVisibilityMask );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::updateRender(void)
    {
        const uint32 oldVisibilityMask = mSceneManager->getVisibilityMask();
        mSceneManager->setVisibilityMask( 0xffffffff );

        CubemapProbeVec::iterator itor = mDirtyProbes.begin();
        CubemapProbeVec::iterator end  = mDirtyProbes.end();

        while( itor != end )
        {
            CubemapProbe *probe = *itor;
            probe->_updateRender();
            probe->mDirty = false;
            ++itor;
        }

        mDirtyProbes.clear();

        mSceneManager->setVisibilityMask( oldVisibilityMask );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::updateAllDirtyProbes(void)
    {
        mSceneManager->updateSceneGraph();

        mDirtyProbes.clear();

        CubemapProbeVec::iterator itor = mProbes.begin();
        CubemapProbeVec::iterator end  = mProbes.end();

        while( itor != end )
        {
            CubemapProbe *probe = *itor;
            if( probe->mDirty && probe->mEnabled )
                mDirtyProbes.push_back( probe );
            ++itor;
        }

        itor = mDirtyProbes.begin();
        end  = mDirtyProbes.end();

        while( itor != end )
        {
            (*itor)->_prepareForRendering();
            ++itor;
        }

        this->updateExpensiveCollectedDirtyProbes( 0 );

        mSceneManager->clearFrameData();
    }
    //-----------------------------------------------------------------------------------
    bool ParallaxCorrectedCubemapAuto::frameStarted( const FrameEvent& evt )
    {
        if( !mPaused )
            this->updateSceneGraph();
        return true;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::allWorkspacesBeforeBeginUpdate(void)
    {
        if( !mPaused )
            this->updateExpensiveCollectedDirtyProbes( 1 );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::allWorkspacesBeginUpdate(void)
    {
        if( !mPaused )
            this->updateRender();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::passPreExecute( CompositorPass *pass )
    {
        //This is not really needed because we only copy LOD 0 then auto generate the mipmaps.
        //However we left this code just in case we need to copy multiple LODs later again.
        if( pass->getType() == PASS_QUAD &&
            pass->getDefinition()->mIdentifier == c_cubmapToDpmIdentifier )
        {
            OGRE_ASSERT_HIGH( dynamic_cast<CompositorPassQuad*>( pass ) );
            CompositorPassQuad *passQuad = static_cast<CompositorPassQuad*>( pass );
            Pass *matPass = passQuad->getPass();
            GpuProgramParametersSharedPtr psParams = matPass->getFragmentProgramParameters();
            psParams->setNamedConstant( "lodLevel", static_cast<float>(
                                            pass->getRenderPassDesc()->mColour[0].mipLevel ) );
        }
    }
}
