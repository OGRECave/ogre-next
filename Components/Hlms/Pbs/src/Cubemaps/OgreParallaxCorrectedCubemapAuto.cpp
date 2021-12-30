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

#include "Cubemaps/OgreParallaxCorrectedCubemapAuto.h"

#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceDef.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuad.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "OgreBitwise.h"
#include "OgreCamera.h"
#include "OgreDepthBuffer.h"
#include "OgreHlms.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreInternalCubemapProbe.h"
#include "OgreItem.h"
#include "OgreLwString.h"
#include "OgreMaterialManager.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreSubMesh2.h"
#include "OgreTechnique.h"
#include "OgreTextureBox.h"
#include "OgreTextureGpuManager.h"
#include "Vao/OgreConstBufferPacked.h"
#include "Vao/OgreStagingBuffer.h"

namespace Ogre
{
    static uint32 c_cubmapToDpmIdentifier = 1863669;

    ParallaxCorrectedCubemapAutoListener::~ParallaxCorrectedCubemapAutoListener() {}
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAutoListener::preCopyRenderTargetToCubemap( TextureGpu *renderTarget,
                                                                             uint32 cubemapArrayIdx )
    {
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    ParallaxCorrectedCubemapAuto::ParallaxCorrectedCubemapAuto(
        IdType id, Root *root, SceneManager *sceneManager,
        const CompositorWorkspaceDef *probeWorkspcDef ) :
        ParallaxCorrectedCubemapBase( id, root, sceneManager, probeWorkspcDef, true ),
        mTrackedPosition( Vector3::ZERO ),
        mRenderTarget( 0 ),
        mIblTarget( 0 ),
        mListener( 0 )
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
    void ParallaxCorrectedCubemapAuto::destroyProbe( CubemapProbe *probe )
    {
        CubemapProbeVec::iterator itor = std::find( mDirtyProbes.begin(), mDirtyProbes.end(), probe );
        if( itor != mDirtyProbes.end() )
        {
            efficientVectorRemove( mDirtyProbes, itor );
        }

        ParallaxCorrectedCubemapBase::destroyProbe( probe );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::setUpdatedTrackedDataFromCamera( Camera *trackedCamera )
    {
        mTrackedPosition = trackedCamera->getDerivedPosition();
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *ParallaxCorrectedCubemapAuto::_acquireTextureSlot( uint16 &outTexSlot )
    {
        OGRE_ASSERT_LOW( mBindTexture && "Must call ParallaxCorrectedCubemapAuto::setEnabled first!" );

        vector<uint64>::type::iterator itor = mReservedSlotBitset.begin();
        vector<uint64>::type::iterator end = mReservedSlotBitset.end();

        TextureGpu *retVal = 0;
        uint32 firstBitSet = 64u;
        while( itor != end && firstBitSet == 64u )
        {
            firstBitSet = Bitwise::ctz64( *itor );
            if( firstBitSet != 64u )
            {
                uint32 idx = static_cast<uint32>( itor - mReservedSlotBitset.begin() );
                outTexSlot = static_cast<uint16>( firstBitSet + 64u * idx );
                retVal = mBindTexture;
                *itor &= ~( ( (uint64)1ul ) << firstBitSet );
            }
            ++itor;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::_releaseTextureSlot( TextureGpu *texture, uint32 texSlot )
    {
        OGRE_ASSERT_LOW( texture == mBindTexture );

        size_t idx = texSlot / 64u;
        uint64 mask = texSlot % 64u;
        mask = ( (uint64)1ul ) << mask;

        OGRE_ASSERT_LOW( idx < mReservedSlotBitset.size() && "Slot is invalid. Out of bounds!" );
        OGRE_ASSERT_LOW( !( mReservedSlotBitset[idx] & mask ) && "Slot was already released!" );
        mReservedSlotBitset[idx] |= mask;
    }
    //-----------------------------------------------------------------------------------
    TextureGpu *ParallaxCorrectedCubemapAuto::findTmpRtt( const TextureGpu *baseParams )
    {
        OGRE_ASSERT_LOW( mRenderTarget && "Must call ParallaxCorrectedCubemapAuto::setEnabled first!" );
        return mRenderTarget;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::releaseTmpRtt( const TextureGpu *tmpRtt ) {}
    //-----------------------------------------------------------------------------------
    TextureGpu *ParallaxCorrectedCubemapAuto::findIbl( const TextureGpu *baseParams )
    {
        OGRE_ASSERT_LOW( mIblTarget && "Must call ParallaxCorrectedCubemapAuto::setEnabled first!" );
        return mIblTarget;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::releaseIbl( const TextureGpu *ibl ) {}
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::_copyRenderTargetToCubemap( uint32 cubemapArrayIdx )
    {
        if( mListener )
            mListener->preCopyRenderTargetToCubemap( mRenderTarget, cubemapArrayIdx );

        TextureGpu *ibl = mIblTarget;

        RenderSystem *renderSystem = mSceneManager->getDestinationRenderSystem();
        BarrierSolver &solver = renderSystem->getBarrierSolver();

        ResourceTransitionArray &barrier = solver.getNewResourceTransitionsArrayTmp();
        solver.resolveTransition( barrier, mIblTarget, ResourceLayout::CopySrc, ResourceAccess::Read,
                                  0u );
        solver.resolveTransition( barrier, mBindTexture, ResourceLayout::CopyDst, ResourceAccess::Write,
                                  0u );

        renderSystem->executeResourceTransition( barrier );

        if( !mUseDpm2DArray )
            cubemapArrayIdx *= 6u;

        const uint8 numMipmaps = ibl->getNumMipmaps();
        for( uint8 mip = 0; mip < numMipmaps; ++mip )
        {
            const CopyEncTransitionMode::CopyEncTransitionMode transitionMode =
                mip == 0u ? CopyEncTransitionMode::AlreadyInLayoutThenAuto : CopyEncTransitionMode::Auto;

            // srcBox.numSlices = 6, thus we ask the RenderSystem to copy all 6 slices in one call
            TextureBox srcBox = ibl->getEmptyBox( mip );
            TextureBox dstBox = srcBox;
            srcBox.sliceStart = 0;
            dstBox.sliceStart = cubemapArrayIdx;
            ibl->copyTo( mBindTexture, dstBox, mip, srcBox, mip, true, transitionMode, transitionMode );
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::setEnabled( bool bEnabled, uint32 width, uint32 height,
                                                   uint32 maxNumProbes, PixelFormatGpu pixelFormat )
    {
        if( bEnabled == getEnabled() )
            return;

        TextureGpuManager *textureGpuManager =
            mSceneManager->getDestinationRenderSystem()->getTextureGpuManager();

        if( bEnabled )
        {
            uint32 textureFlags = TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps |
                                  TextureFlags::DiscardableContent;

            // Create Cubemap
            mRenderTarget = textureGpuManager->createTexture(
                "ParallaxCorrectedCubemapAuto Target " + StringConverter::toString( getId() ),
                GpuPageOutStrategy::Discard, textureFlags, TextureTypes::TypeCube );
            mRenderTarget->setResolution( width, height );
            mRenderTarget->setPixelFormat( pixelFormat );
            mRenderTarget->setNumMipmaps( PixelFormatGpuUtils::getMaxMipmapCount( width, height ) );
            mRenderTarget->_transitionTo( GpuResidency::Resident, (uint8 *)0 );

            const uint8 numMipmaps = getIblNumMipmaps( width, height );

            // OVERRIDE textureFlags. mIblTarget is only used by Compute Shaders
            textureFlags = getIblTargetTextureFlags( pixelFormat );

            TextureTypes::TextureTypes iblTextureType = TextureTypes::TypeCube;
            String namePostfix = "";

            if( mUseDpm2DArray )
            {
                iblTextureType = TextureTypes::Type2D;
                namePostfix = "(DPM) ";
            }

            mIblTarget = textureGpuManager->createTexture(
                "ParallaxCorrectedCubemapAuto IBL " + namePostfix + StringConverter::toString( getId() ),
                GpuPageOutStrategy::Discard, textureFlags, iblTextureType );
            if( !mUseDpm2DArray )
                mIblTarget->setResolution( width, height );
            else
                mIblTarget->setResolution( width << 1u, height << 1u );
            mIblTarget->setPixelFormat( pixelFormat );
            mIblTarget->setNumMipmaps( numMipmaps );
            mIblTarget->scheduleTransitionTo( GpuResidency::Resident );

            // Create the actual texture to contain the reflections
            // that will be bound during normal render
            const TextureTypes::TextureTypes bindTextureType =
                mUseDpm2DArray ? TextureTypes::Type2DArray : TextureTypes::TypeCubeArray;

            uint32 numSlices = maxNumProbes;
            if( !mUseDpm2DArray )
                numSlices *= 6u;

            mBindTexture = textureGpuManager->createTexture(
                "ParallaxCorrectedCubemapAuto Array " + StringConverter::toString( getId() ),
                GpuPageOutStrategy::Discard, TextureFlags::ManualTexture, bindTextureType );
            mBindTexture->setResolution( mIblTarget->getWidth(), mIblTarget->getHeight(), numSlices );
            mBindTexture->setPixelFormat( pixelFormat );
            mBindTexture->setNumMipmaps( numMipmaps );
            mBindTexture->scheduleTransitionTo( GpuResidency::Resident );

            const uint64 remainder = maxNumProbes % 64u;
            mReservedSlotBitset.resize( alignToNextMultiple( maxNumProbes, 64u ) >> 6u,
                                        0xffffffffffffffff );
            if( remainder != 0u )
                mReservedSlotBitset.back() = ( (uint64_t)1u << remainder ) - 1u;

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
            CompositorManager2 *compositorManager = mDefaultWorkspaceDef->getCompositorManager();
            compositorManager->removeListener( this );

            mRoot->removeFrameListener( this );

            CubemapProbeVec::const_iterator itor = mProbes.begin();
            CubemapProbeVec::const_iterator end = mProbes.end();

            while( itor != end )
            {
                ( *itor )->destroyWorkspace();
                ++itor;
            }

            mReservedSlotBitset.clear();

            if( mIblTarget )
            {
                textureGpuManager->destroyTexture( mIblTarget );
                mIblTarget = 0;
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
    bool ParallaxCorrectedCubemapAuto::getEnabled() const { return mRenderTarget != 0; }
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
    void ParallaxCorrectedCubemapAuto::updateSceneGraph()
    {
        mDirtyProbes.clear();

        const uint32 systemMask = mMask;

        CubemapProbeVec::iterator itor = mProbes.begin();
        CubemapProbeVec::iterator end = mProbes.end();

        while( itor != end )
        {
            CubemapProbe *probe = *itor;

            const Vector3 posLS = probe->mInvOrientation * ( mTrackedPosition - probe->mArea.mCenter );
            const Aabb areaLS = probe->getAreaLS();
            if( ( ( areaLS.contains( posLS ) && !probe->mStatic ) || probe->mDirty ) &&
                probe->mEnabled && probe->mTexture && ( probe->mMask & systemMask ) )
            {
                mDirtyProbes.push_back( probe );
            }

            if( probe->mInternalProbe )
            {
                probe->mInternalProbe->setVisible( probe->mEnabled && probe->mTexture &&
                                                   ( probe->mMask & systemMask ) != 0u );
            }

            ++itor;
        }

        itor = mDirtyProbes.begin();
        end = mDirtyProbes.end();

        while( itor != end )
        {
            ( *itor )->_prepareForRendering();
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::updateExpensiveCollectedDirtyProbes( uint16 iterationThreshold )
    {
        RenderSystem *renderSystem = mSceneManager->getDestinationRenderSystem();

        const uint32 oldVisibilityMask = mSceneManager->getVisibilityMask();
        mSceneManager->setVisibilityMask( 0xffffffff );

        CubemapProbeVec::iterator itor = mDirtyProbes.begin();
        CubemapProbeVec::iterator end = mDirtyProbes.end();

        while( itor != end )
        {
            CubemapProbe *probe = *itor;

            if( probe->mNumIterations > iterationThreshold )
            {
                renderSystem->_beginFrameOnce();
                probe->_updateRender();

                renderSystem->_update();
                renderSystem->_endFrameOnce();

                probe->mDirty = false;

                if( iterationThreshold > 0u )
                {
                    // We have to remove only those that are no longer dirty
                    itor = efficientVectorRemove( mDirtyProbes, itor );
                    end = mDirtyProbes.end();
                }
                else
                    ++itor;
            }
            else
            {
                ++itor;
            }
        }

        // When iterationThreshold == 0; we're updating every probe,
        // thus just clear them all at once
        if( iterationThreshold == 0u )
            mDirtyProbes.clear();

        mSceneManager->setVisibilityMask( oldVisibilityMask );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::updateRender()
    {
        const uint32 oldVisibilityMask = mSceneManager->getVisibilityMask();
        mSceneManager->setVisibilityMask( 0xffffffff );

        CubemapProbeVec::iterator itor = mDirtyProbes.begin();
        CubemapProbeVec::iterator end = mDirtyProbes.end();

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
    void ParallaxCorrectedCubemapAuto::updateAllDirtyProbes()
    {
        mSceneManager->updateSceneGraph();

        mDirtyProbes.clear();

        CubemapProbeVec::iterator itor = mProbes.begin();
        CubemapProbeVec::iterator end = mProbes.end();

        while( itor != end )
        {
            CubemapProbe *probe = *itor;
            if( probe->mDirty && probe->mEnabled && probe->mTexture )
                mDirtyProbes.push_back( probe );
            ++itor;
        }

        itor = mDirtyProbes.begin();
        end = mDirtyProbes.end();

        while( itor != end )
        {
            ( *itor )->_prepareForRendering();
            ++itor;
        }

        this->updateExpensiveCollectedDirtyProbes( 0 );

        mSceneManager->clearFrameData();
    }
    //-----------------------------------------------------------------------------------
    bool ParallaxCorrectedCubemapAuto::frameStarted( const FrameEvent &evt )
    {
        if( !mPaused )
            this->updateSceneGraph();
        return true;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::allWorkspacesBeforeBeginUpdate()
    {
        if( !mPaused )
            this->updateExpensiveCollectedDirtyProbes( 1 );
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::allWorkspacesBeginUpdate()
    {
        if( !mPaused )
            this->updateRender();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::passPreExecute( CompositorPass *pass )
    {
        // This is not really needed because we only copy LOD 0 then auto generate the mipmaps.
        // However we left this code just in case we need to copy multiple LODs later again.
        if( pass->getType() == PASS_QUAD &&
            pass->getDefinition()->mIdentifier == c_cubmapToDpmIdentifier )
        {
            OGRE_ASSERT_HIGH( dynamic_cast<CompositorPassQuad *>( pass ) );
            CompositorPassQuad *passQuad = static_cast<CompositorPassQuad *>( pass );
            Pass *matPass = passQuad->getPass();
            GpuProgramParametersSharedPtr psParams = matPass->getFragmentProgramParameters();
            psParams->setNamedConstant(
                "lodLevel", static_cast<float>( pass->getRenderPassDesc()->mColour[0].mipLevel ) );
        }
        else
        {
            ParallaxCorrectedCubemapBase::passPreExecute( pass );
        }
    }
}  // namespace Ogre
