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
#include "Compositor/Pass/PassClear/OgreCompositorPassClearDef.h"
#include "Compositor/Pass/PassQuad/OgreCompositorPassQuadDef.h"
#include "Compositor/Pass/PassScene/OgreCompositorPassSceneDef.h"

#include "OgreBitwise.h"

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
    ParallaxCorrectedCubemapAuto::ParallaxCorrectedCubemapAuto(
            IdType id, Root *root, SceneManager *sceneManager,
            const CompositorWorkspaceDef *probeWorkspcDef ) :
        ParallaxCorrectedCubemapBase( id, root, sceneManager, probeWorkspcDef, true ),
        mTrackedPosition( Vector3::ZERO ),
        mRenderTarget( 0 ),
        mTextureArray( 0 )
    {
    }
    //-----------------------------------------------------------------------------------
    ParallaxCorrectedCubemapAuto::~ParallaxCorrectedCubemapAuto()
    {
        setEnabled( false, 0, 0, 0, PFG_UNKNOWN );
        destroyAllProbes();
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::setUpdatedTrackedDataFromCamera( Camera *trackedCamera )
    {
        mTrackedPosition = trackedCamera->getDerivedPosition();
    }
    //-----------------------------------------------------------------------------------
    TextureGpu* ParallaxCorrectedCubemapAuto::_acquireTextureSlot( uint32 &outTexSlot )
    {
        OGRE_ASSERT_LOW( mTextureArray && "Must call ParallaxCorrectedCubemapAuto::setEnabled first!" );

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
                retVal = mTextureArray;
                *itor &= ~(((uint64)1ul) << firstBitSet);
            }
            ++itor;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::_releaseTextureSlot( TextureGpu *texture, uint32 texSlot )
    {
        OGRE_ASSERT_LOW( texture == mTextureArray );

        size_t idx  = texSlot / 64u;
        uint64 mask = texSlot % 64u;
        mask = ~(((uint64)1ul) << mask);

        OGRE_ASSERT_LOW( idx < mReservedSlotBitset.size() && "Slot is invalid. Out of bounds!" );
        OGRE_ASSERT_LOW( !(mReservedSlotBitset[idx] & mask) && "Slot was already released!" );
        mReservedSlotBitset[idx] |= texSlot;
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
            mRenderTarget =
                    textureGpuManager->createTexture(
                        "ParallaxCorrectedCubemapAuto Target " +
                        StringConverter::toString( getId() ),
                        GpuPageOutStrategy::Discard,
                        TextureFlags::RenderToTexture | TextureFlags::AllowAutomipmaps,
                        TextureTypes::TypeCube );
            mRenderTarget->setResolution( width, height );
            mRenderTarget->setPixelFormat( pixelFormat );
            mRenderTarget->setNumMipmaps( PixelFormatGpuUtils::getMaxMipmapCount( width, height ) );
            mRenderTarget->_transitionTo( GpuResidency::Resident, (uint8*)0 );

            mTextureArray =
                    textureGpuManager->createTexture(
                        "ParallaxCorrectedCubemapAuto Array " +
                        StringConverter::toString( getId() ),
                        GpuPageOutStrategy::Discard,
                        TextureFlags::ManualTexture,
                        TextureTypes::TypeCubeArray );
            mTextureArray->setResolution( width, height, maxNumProbes );
            mTextureArray->setPixelFormat( pixelFormat );
            mTextureArray->setNumMipmaps( PixelFormatGpuUtils::getMaxMipmapCount( width, height ) );
            mTextureArray->_transitionTo( GpuResidency::Resident, (uint8*)0 );

            mReservedSlotBitset.resize( alignToNextMultiple( maxNumProbes, 64u ) >> 6u,
                                        0xffffffffffffffff );

            mRoot->addFrameListener( this );
            CompositorManager2 *compositorManager = mDefaultWorkspaceDef->getCompositorManager();
            compositorManager->addListener( this );

            CubemapProbeVec::const_iterator itor = mProbes.begin();
            CubemapProbeVec::const_iterator end  = mProbes.end();

            while( itor != end )
            {
                if( (*itor)->isInitialized() )
                    (*itor)->initWorkspace();
                ++itor;
            }
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

            if( mRenderTarget )
            {
                textureGpuManager->destroyTexture( mRenderTarget );
                mRenderTarget = 0;
            }
            if( mTextureArray )
            {
                textureGpuManager->destroyTexture( mTextureArray );
                mTextureArray = 0;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    bool ParallaxCorrectedCubemapAuto::getEnabled(void) const
    {
        return mRenderTarget != 0;
    }
    //-----------------------------------------------------------------------------------
    void ParallaxCorrectedCubemapAuto::updateSceneGraph(void)
    {
        mDirtyProbes.clear();

        CubemapProbeVec::iterator itor = mProbes.begin();
        CubemapProbeVec::iterator end  = mProbes.end();

        while( itor != end )
        {
            CubemapProbe *probe = *itor;

            const Vector3 posLS = probe->mInvOrientation * (mTrackedPosition - probe->mArea.mCenter);
            const Aabb areaLS = probe->getAreaLS();
            if( ((areaLS.contains( posLS ) && !probe->mStatic) || probe->mDirty) && probe->mEnabled )
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
            }

            ++itor;
        }

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
}
