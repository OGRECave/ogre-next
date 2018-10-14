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

#include "Compositor/Pass/OgreCompositorPass.h"
#include "Compositor/OgreCompositorChannel.h"
#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"

#include "OgrePixelFormatGpuUtils.h"
#include "OgreViewport.h"

#include "OgreRenderSystem.h"
#include "OgreProfiler.h"

#include "OgreStringConverter.h"

namespace Ogre
{
//    const Quaternion CompositorPass::CubemapRotations[6] =
//    {
//        Quaternion( Degree(-90 ), Vector3::UNIT_Y ),          //+X
//        Quaternion( Degree( 90 ), Vector3::UNIT_Y ),          //-X
//        Quaternion( Degree( 90 ), Vector3::UNIT_X ),          //+Y
//        Quaternion( Degree(-90 ), Vector3::UNIT_X ),          //-Y
//        Quaternion::IDENTITY,                                 //+Z
//        Quaternion( Degree(180 ), Vector3::UNIT_Y )           //-Z
//    };
    //Can NOT use UNIT_X/UNIT_Y and Degree here because on iOS C++ initialization
    //order screws us and thus key global variables are still 0. Hardcode the values.
    const Quaternion CompositorPass::CubemapRotations[6] =
    {
        Quaternion( Radian(-1.570796f ), Vector3( 0, 1, 0 ) ),  //+X
        Quaternion( Radian( 1.570796f ), Vector3( 0, 1, 0 ) ),  //-X
        Quaternion( Radian( 1.570796f ), Vector3( 1, 0, 0 ) ),  //+Y
        Quaternion( Radian(-1.570796f ), Vector3( 1, 0, 0 ) ),  //-Y
        Quaternion( 1, 0, 0, 0 ),                               //+Z
        Quaternion( Radian( 3.1415927f), Vector3( 0, 1, 0 ) )   //-Z
    };

    CompositorPass::CompositorPass( const CompositorPassDef *definition,
                                    CompositorNode *parentNode ) :
            mDefinition( definition ),
            mRenderPassDesc( 0 ),
            mAnyTargetTexture( 0 ),
            mAnyMipLevel( 0u ),
            mNumPassesLeft( definition->mNumInitialPasses ),
            mParentNode( parentNode ),
            mNumValidResourceTransitions( 0 )
    {
        assert( definition->mNumInitialPasses && "Definition is broken, pass will never execute!" );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::initialize( const RenderTargetViewDef *rtv, bool supportsNoRtv )
    {
        if( !supportsNoRtv && !rtv )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "No RenderTargetViewDef provided to this compositor pass "
                         "(e.g. you wrote target {} instead of target myTexture {}.\n"
                         "Only a few compositor passes support this (e.g. Compute passes)\n"
                         "Node with error: " +
                         mDefinition->getParentTargetDef()->getParentNodeDef()->getNameStr() + "\n",
                         "CompositorPass::CompositorPass" );
        }

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        if( rtv )
        {
            mRenderPassDesc = renderSystem->createRenderPassDescriptor();
            setupRenderPassDesc( rtv );

            for( int i=0; i<mRenderPassDesc->getNumColourEntries() && !mAnyTargetTexture; ++i )
            {
                mAnyTargetTexture = mRenderPassDesc->mColour[i].texture;
                mAnyMipLevel = mRenderPassDesc->mColour[i].mipLevel;
            }
            if( !mAnyTargetTexture )
            {
                mAnyTargetTexture = mRenderPassDesc->mDepth.texture;
                mAnyMipLevel = mRenderPassDesc->mDepth.mipLevel;
            }
            if( !mAnyTargetTexture )
            {
                mAnyTargetTexture = mRenderPassDesc->mStencil.texture;
                mAnyMipLevel = mRenderPassDesc->mStencil.mipLevel;
            }
        }

        populateTextureDependenciesFromExposedTextures();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::setRenderPassDescToCurrent(void)
    {
        CompositorWorkspace *workspace = mParentNode->getWorkspace();
        uint8 workspaceVpMask = workspace->getViewportModifierMask();

        bool applyModifier = (workspaceVpMask & mDefinition->mViewportModifierMask) != 0;
        Vector4 vpModifier = applyModifier ? workspace->getViewportModifier() : Vector4( 0, 0, 1, 1 );

        Real left   = mDefinition->mVpLeft      + vpModifier.x;
        Real top    = mDefinition->mVpTop       + vpModifier.y;
        Real width  = mDefinition->mVpWidth     * vpModifier.z;
        Real height = mDefinition->mVpHeight    * vpModifier.w;

        Real scLeft   = mDefinition->mVpScissorLeft     + vpModifier.x;
        Real scTop    = mDefinition->mVpScissorTop      + vpModifier.y;
        Real scWidth  = mDefinition->mVpScissorWidth    * vpModifier.z;
        Real scHeight = mDefinition->mVpScissorHeight   * vpModifier.w;

        Vector4 vpSize( left, top, width, height );
        Vector4 scissors( scLeft, scTop, scWidth, scHeight );

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        renderSystem->beginRenderPassDescriptor( mRenderPassDesc, mAnyTargetTexture, mAnyMipLevel,
                                                 vpSize, scissors, mDefinition->mIncludeOverlays,
                                                 mDefinition->mWarnIfRtvWasFlushed );
    }
    //-----------------------------------------------------------------------------------
    CompositorPass::~CompositorPass()
    {
        if( mRenderPassDesc )
        {
            RenderSystem *renderSystem = mParentNode->getRenderSystem();
            renderSystem->destroyRenderPassDescriptor( mRenderPassDesc );
            mRenderPassDesc = 0;
        }

        _removeAllBarriers();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::setupRenderPassDesc( const RenderTargetViewDef *rtv )
    {
        if( rtv->isRuntimeAnalyzed() )
        {
            TextureGpu *texture =
                    mParentNode->getDefinedTexture( rtv->colourAttachments[0].textureName );
            RenderTargetViewDef tmpRtv;
            if( PixelFormatGpuUtils::isDepth( texture->getPixelFormat() ) )
            {
                tmpRtv.depthAttachment.textureName = rtv->colourAttachments[0].textureName;

                if( PixelFormatGpuUtils::isStencil( texture->getPixelFormat() ) )
                    tmpRtv.stencilAttachment.textureName = rtv->colourAttachments[0].textureName;
            }
            else
            {
                tmpRtv.colourAttachments.push_back( rtv->colourAttachments[0] );
                tmpRtv.depthBufferId        = texture->getDepthBufferPoolId();
                tmpRtv.preferDepthTexture   = texture->getPreferDepthTexture();
                tmpRtv.depthBufferFormat    = texture->getDesiredDepthBufferFormat();
            }

            setupRenderPassDesc( &tmpRtv );
            return;
        }

        RenderPassDescriptor *renderPassDesc = mRenderPassDesc;

        const size_t numColourAttachments = rtv->colourAttachments.size();

        if( numColourAttachments > OGRE_MAX_MULTIPLE_RENDER_TARGETS )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot have more than " +
                         StringConverter::toString( OGRE_MAX_MULTIPLE_RENDER_TARGETS ) +
                         " colour attachments for RTV in " + mParentNode->getName().getFriendlyText(),
                         "CompositorPass::createRenderPassDesc" );
        }

        for( size_t i=0; i<numColourAttachments; ++i )
        {
            renderPassDesc->mColour[i].loadAction   = mDefinition->mLoadActionColour[i];
            renderPassDesc->mColour[i].storeAction  = mDefinition->mStoreActionColour[i];
            renderPassDesc->mColour[i].clearColour  = mDefinition->mClearColour[i];
            renderPassDesc->mColour[i].allLayers    = rtv->colourAttachments[i].colourAllLayers;
            setupRenderPassTarget( &renderPassDesc->mColour[i], rtv->colourAttachments[i], true );
        }

        renderPassDesc->mDepth.loadAction       = mDefinition->mLoadActionDepth;
        renderPassDesc->mDepth.storeAction      = mDefinition->mStoreActionDepth;
        renderPassDesc->mDepth.clearDepth       = mDefinition->mClearDepth;
        renderPassDesc->mDepth.readOnly         = rtv->depthReadOnly ||
                                                  mDefinition->mReadOnlyDepth;
        setupRenderPassTarget( &renderPassDesc->mDepth, rtv->depthAttachment, false,
                               renderPassDesc->mColour[0].texture, rtv->depthBufferId,
                               rtv->preferDepthTexture, rtv->depthBufferFormat );

        //Check if stencil is required.
        PixelFormatGpu stencilFormat = PFG_UNKNOWN;
        if( rtv->stencilAttachment.textureName != IdString() )
        {
            TextureGpu *stencilTexture =
                    mParentNode->getDefinedTexture( rtv->stencilAttachment.textureName );
            if( !stencilTexture )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Couldn't find texture for RTV with name " +
                             rtv->stencilAttachment.textureName.getFriendlyText(),
                             "CompositorPass::setupRenderPassTarget" );
            }
            PixelFormatGpu preferredFormat = stencilTexture->getPixelFormat();
            if( PixelFormatGpuUtils::isStencil( preferredFormat ) )
                stencilFormat = preferredFormat;
        }
        else if( renderPassDesc->mColour[0].texture || renderPassDesc->mDepth.texture )
        {
            if( renderPassDesc->mDepth.texture )
            {
                PixelFormatGpu depthFormat = renderPassDesc->mDepth.texture->getPixelFormat();
                if( PixelFormatGpuUtils::isStencil( depthFormat ) )
                    stencilFormat = depthFormat;
            }
            else if( renderPassDesc->mColour[0].texture )
            {
                PixelFormatGpu desiredFormat =
                        renderPassDesc->mColour[0].texture->getDesiredDepthBufferFormat();
                if( PixelFormatGpuUtils::isStencil( desiredFormat ) )
                    stencilFormat = desiredFormat;
            }
        }

        if( stencilFormat != PFG_UNKNOWN )
        {
            renderPassDesc->mStencil.loadAction     = mDefinition->mLoadActionStencil;
            renderPassDesc->mStencil.storeAction    = mDefinition->mStoreActionStencil;
            renderPassDesc->mStencil.clearStencil   = mDefinition->mClearStencil;
            renderPassDesc->mStencil.readOnly       = rtv->stencilReadOnly ||
                                                      mDefinition->mReadOnlyStencil;
            setupRenderPassTarget( &renderPassDesc->mStencil, rtv->stencilAttachment, false,
                                   renderPassDesc->mColour[0].texture, rtv->depthBufferId,
                                   rtv->preferDepthTexture, rtv->depthBufferFormat );
        }

        postRenderPassDescriptorSetup( renderPassDesc );

        renderPassDesc->entriesModified( RenderPassDescriptor::All );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::setupRenderPassTarget( RenderPassTargetBase *renderPassTargetAttachment,
                                                const RenderTargetViewEntry &rtvEntry,
                                                bool isColourAttachment,
                                                TextureGpu *linkedColourAttachment, uint16 depthBufferId,
                                                bool preferDepthTexture,
                                                PixelFormatGpu depthBufferFormat )
    {
        if( !isColourAttachment )
        {
            //This is depth or stencil
            if( rtvEntry.textureName != IdString() )
            {
                renderPassTargetAttachment->texture =
                        mParentNode->getDefinedTexture( rtvEntry.textureName );

                if( !renderPassTargetAttachment->texture )
                {
                    OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                 "Couldn't find texture for RTV with name " +
                                 rtvEntry.textureName.getFriendlyText(),
                                 "CompositorPass::setupRenderPassTarget" );
                }
            }
            else if( linkedColourAttachment )
            {
                RenderSystem *renderSystem = mParentNode->getRenderSystem();
                renderPassTargetAttachment->texture =
                        renderSystem->getDepthBufferFor( linkedColourAttachment, depthBufferId,
                                                         preferDepthTexture, depthBufferFormat );
            }
        }
        else if( rtvEntry.textureName != IdString() )
        {
            //This is colour
            renderPassTargetAttachment->texture =
                    mParentNode->getDefinedTexture( rtvEntry.textureName );

            if( !renderPassTargetAttachment->texture )
            {
                OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                             "Couldn't find texture for RTV with name " +
                             rtvEntry.textureName.getFriendlyText(),
                             "CompositorPass::setupRenderPassTarget" );
            }

            //Deal with MSAA resolve textures.
            if( renderPassTargetAttachment->texture->getMsaa() > 1u )
            {
                if( renderPassTargetAttachment->storeAction == StoreAction::MultisampleResolve ||
                    renderPassTargetAttachment->storeAction == StoreAction::StoreAndMultisampleResolve ||
                    (renderPassTargetAttachment->storeAction == StoreAction::StoreOrResolve &&
                     (!renderPassTargetAttachment->texture->hasMsaaExplicitResolves() ||
                      rtvEntry.resolveTextureName != IdString())) )
                {
                    //If we're here, the texture is MSAA _AND_ we'll resolve it.
                    if( rtvEntry.resolveTextureName == IdString() )
                    {
                        if( !allowResolveStoreActionsWithoutResolveTexture() )
                        {
                            if( renderPassTargetAttachment->texture->hasMsaaExplicitResolves() )
                            {
                                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                             "Must specify resolveTextureName for RTV when using explicit "
                                             "resolves and store action is either "
                                             "StoreAction::MultisampleResolve, "
                                             "StoreAction::StoreAndMultisampleResolve. "
                                             "Texture: " + renderPassTargetAttachment->texture->getNameStr(),
                                             "CompositorPass::setupRenderPassTarget" );
                            }

                            renderPassTargetAttachment->resolveTexture = renderPassTargetAttachment->texture;
                        }
                        else
                        {
                            renderPassTargetAttachment->resolveTexture = 0;
                        }
                    }
                    else
                    {
                        //Resolve texture is explicitly defined.
                        //Allow this even if the texture has implicit resolves.
                        renderPassTargetAttachment->resolveTexture =
                                mParentNode->getDefinedTexture( rtvEntry.resolveTextureName );

                        if( !renderPassTargetAttachment->resolveTexture )
                        {
                            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                         "Couldn't find resolve texture for RTV with name " +
                                         rtvEntry.resolveTextureName.getFriendlyText(),
                                         "CompositorPass::setupRenderPassTarget" );
                        }

                        if( renderPassTargetAttachment->resolveTexture->getMsaa() > 1u )
                        {
                            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                         "Cannot specify a non-MSAA texture for resolving an "
                                         "MSAA texture."
                                         "\nMSAA Texture: " +
                                         renderPassTargetAttachment->texture->getNameStr() +
                                         "\nBroken Resolve Texture: " +
                                         renderPassTargetAttachment->resolveTexture->getNameStr(),
                                         "CompositorPass::setupRenderPassTarget" );
                        }
                    }
                }
            }
        }

        renderPassTargetAttachment->mipLevel        = rtvEntry.mipLevel;
        renderPassTargetAttachment->resolveMipLevel = rtvEntry.resolveMipLevel;
        if( !isColourAttachment || mDefinition->getRtIndex() == 0 )
        {
            renderPassTargetAttachment->slice       = rtvEntry.slice;
            renderPassTargetAttachment->resolveSlice= rtvEntry.resolveSlice;
        }
        else
        {
            //This is colour target asking to override the RTV.
            renderPassTargetAttachment->slice       = mDefinition->getRtIndex();
            renderPassTargetAttachment->resolveSlice= mDefinition->getRtIndex();
        }

        if( renderPassTargetAttachment->storeAction == StoreAction::StoreOrResolve )
        {
            if( !renderPassTargetAttachment->texture )
                renderPassTargetAttachment->storeAction = StoreAction::DontCare;
            else
            {
                if( renderPassTargetAttachment->texture->getMsaa() > 1u &&
                    renderPassTargetAttachment->resolveTexture )
                {
                    renderPassTargetAttachment->storeAction = StoreAction::MultisampleResolve;
                }
                else
                    renderPassTargetAttachment->storeAction = StoreAction::Store;
            }
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::profilingBegin(void)
    {
#if OGRE_PROFILING
        if( !mParentNode->getWorkspace()->getAmalgamatedProfiling() )
        {
            OgreProfileBeginDynamic( mDefinition->mProfilingId.c_str() );
            OgreProfileGpuBeginDynamic( mDefinition->mProfilingId );
        }
#endif
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::profilingEnd(void)
    {
#if OGRE_PROFILING
        if( !mParentNode->getWorkspace()->getAmalgamatedProfiling() )
        {
            OgreProfileEnd( mDefinition->mProfilingId );
            OgreProfileGpuEnd( mDefinition->mProfilingId );
        }
#endif
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::populateTextureDependenciesFromExposedTextures(void)
    {
        IdStringVec::const_iterator itor = mDefinition->mExposedTextures.begin();
        IdStringVec::const_iterator end  = mDefinition->mExposedTextures.end();

        while( itor != end )
        {
            TextureGpu *channel = mParentNode->getDefinedTexture( *itor );
            mTextureDependencies.push_back( CompositorTexture( *itor, channel ) );

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::executeResourceTransitions(void)
    {
        RenderSystem *renderSystem = mParentNode->getRenderSystem();

        assert( mNumValidResourceTransitions <= mResourceTransitions.size() );
        ResourceTransitionVec::iterator itor = mResourceTransitions.begin();

        for( size_t i=0; i<mNumValidResourceTransitions; ++i )
        {
            renderSystem->_executeResourceTransition( &(*itor) );
            ++itor;
        }
    }
    inline uint32 transitionWriteBarrierBits( ResourceLayout::Layout oldLayout )
    {
        uint32 retVal = 0;
        switch( oldLayout )
        {
        case ResourceLayout::RenderTarget:
            retVal = WriteBarrier::RenderTarget;
            break;
        case ResourceLayout::RenderDepth:
            retVal = WriteBarrier::DepthStencil;
            break;
        case ResourceLayout::Uav:
            retVal = WriteBarrier::Uav;
            break;
        }

        return retVal;
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::addResourceTransition( ResourceLayoutMap::iterator currentLayout,
                                                ResourceLayout::Layout newLayout,
                                                uint32 readBarrierBits )
    {
        ResourceTransition transition;
        //transition.resource = ; TODO
        transition.oldLayout = currentLayout->second;
        transition.newLayout = newLayout;
        transition.writeBarrierBits = transitionWriteBarrierBits( transition.oldLayout );
        transition.readBarrierBits  = readBarrierBits;

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        const RenderSystemCapabilities *caps = renderSystem->getCapabilities();
        if( !caps->hasCapability( RSC_EXPLICIT_API ) )
        {
            //OpenGL. Merge the bits and use only one global barrier.
            //Keep the extra barriers uninitialized for debugging purposes,
            //but we won't be really using them.
            if( mResourceTransitions.empty() )
            {
                ResourceTransition globalBarrier;
                globalBarrier.oldLayout = ResourceLayout::Undefined;
                globalBarrier.newLayout = ResourceLayout::Undefined;
                globalBarrier.writeBarrierBits  = transition.writeBarrierBits;
                globalBarrier.readBarrierBits   = transition.readBarrierBits;
                globalBarrier.mRsData = 0;
                renderSystem->_resourceTransitionCreated( &globalBarrier );
                mResourceTransitions.push_back( globalBarrier );
            }
            else
            {
                ResourceTransition &globalBarrier = mResourceTransitions.front();

                renderSystem->_resourceTransitionDestroyed( &globalBarrier );

                globalBarrier.writeBarrierBits  |= transition.writeBarrierBits;
                globalBarrier.readBarrierBits   |= transition.readBarrierBits;

                renderSystem->_resourceTransitionCreated( &globalBarrier );
            }

            mNumValidResourceTransitions = 1;
        }
        else
        {
            //D3D12, Vulkan, Mantle. Takes advantage of per-resource barriers
            renderSystem->_resourceTransitionCreated( &transition );
            ++mNumValidResourceTransitions;
        }

        mResourceTransitions.push_back( transition );

        currentLayout->second = transition.newLayout;
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::_placeBarriersAndEmulateUavExecution(
                                            BoundUav boundUavs[64], ResourceAccessMap &uavsAccess,
                                            ResourceLayoutMap &resourcesLayout )
    {
        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        const RenderSystemCapabilities *caps = renderSystem->getCapabilities();
        const bool explicitApi = caps->hasCapability( RSC_EXPLICIT_API );

        {
            //mResourceTransitions will be non-empty if we call _placeBarriersAndEmulateUavExecution
            //for a second time (i.e. 2nd pass to check frame-to-frame dependencies). We need
            //to tell what is shielded. On the first time, it should be empty.
            ResourceTransitionVec::const_iterator itor = mResourceTransitions.begin();
            ResourceTransitionVec::const_iterator end  = mResourceTransitions.end();

            while( itor != end )
            {
                if( itor->newLayout == ResourceLayout::Uav &&
                    itor->writeBarrierBits & WriteBarrier::Uav &&
                    itor->readBarrierBits & ReadBarrier::Uav )
                {
                    TextureGpu *renderTarget = 0;
                    resourcesLayout[renderTarget] = ResourceLayout::Uav;
                    //Set to undefined so that following passes
                    //can see it's safe / shielded by a barrier
                    uavsAccess[renderTarget] = ResourceAccess::Undefined;
                }
                ++itor;
            }
        }

        if( mRenderPassDesc )
        {
            //Check <anything> -> RT
            ResourceLayoutMap::iterator currentLayout;
            for( int i=0; i<mRenderPassDesc->getNumColourEntries(); ++i )
            {
                currentLayout = resourcesLayout.find( mRenderPassDesc->mColour[i].texture );
                if( (currentLayout->second != ResourceLayout::RenderTarget && explicitApi) ||
                    currentLayout->second == ResourceLayout::Uav )
                {
                    addResourceTransition( currentLayout,
                                           ResourceLayout::RenderTarget,
                                           ReadBarrier::RenderTarget );
                }
            }
            if( mRenderPassDesc->mDepth.texture )
            {
                currentLayout = resourcesLayout.find( mRenderPassDesc->mDepth.texture );
                if( currentLayout == resourcesLayout.end() )
                {
                    resourcesLayout[mRenderPassDesc->mDepth.texture] = ResourceLayout::Undefined;
                    currentLayout = resourcesLayout.find( mRenderPassDesc->mDepth.texture );
                }

                if( (currentLayout->second != ResourceLayout::RenderDepth && explicitApi) ||
                    currentLayout->second == ResourceLayout::Uav )
                {
                    addResourceTransition( currentLayout,
                                           ResourceLayout::RenderDepth,
                                           ReadBarrier::DepthStencil );
                }
            }
            if( mRenderPassDesc->mStencil.texture &&
                mRenderPassDesc->mStencil.texture != mRenderPassDesc->mDepth.texture )
            {
                currentLayout = resourcesLayout.find( mRenderPassDesc->mStencil.texture );
                if( currentLayout == resourcesLayout.end() )
                {
                    resourcesLayout[mRenderPassDesc->mStencil.texture] = ResourceLayout::Undefined;
                    currentLayout = resourcesLayout.find( mRenderPassDesc->mStencil.texture );
                }

                if( (currentLayout->second != ResourceLayout::RenderDepth && explicitApi) ||
                    currentLayout->second == ResourceLayout::Uav )
                {
                    addResourceTransition( currentLayout,
                                           ResourceLayout::RenderDepth,
                                           ReadBarrier::DepthStencil );
                }
            }
        }

        {
            //Check <anything> -> Texture
            CompositorTextureVec::const_iterator itDep = mTextureDependencies.begin();
            CompositorTextureVec::const_iterator enDep = mTextureDependencies.end();

            while( itDep != enDep )
            {
                TextureGpu *renderTarget = itDep->texture;

                ResourceLayoutMap::iterator currentLayout = resourcesLayout.find( renderTarget );

                if( (currentLayout->second != ResourceLayout::Texture && explicitApi) ||
                     currentLayout->second == ResourceLayout::Uav )
                {
                    addResourceTransition( currentLayout,
                                           ResourceLayout::Texture,
                                           ReadBarrier::Texture );
                }

                ++itDep;
            }
        }

        //Check <anything> -> UAV; including UAV -> UAV
        //Except for RaR (Read after Read) and some WaW (Write after Write),
        //Uavs are always hazardous, an UAV->UAV 'transition' is just a memory barrier.
        CompositorPassDef::UavDependencyVec::const_iterator itor = mDefinition->mUavDependencies.begin();
        CompositorPassDef::UavDependencyVec::const_iterator end  = mDefinition->mUavDependencies.end();

        while( itor != end )
        {
            GpuTrackedResource *uavRt = boundUavs[itor->uavSlot].rttOrBuffer;

            if( !uavRt )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "No UAV is bound at slot " + StringConverter::toString( itor->uavSlot ) +
                             " but it is marked as used by node " +
                             mParentNode->getName().getFriendlyText() + "; pass #" +
                             StringConverter::toString( mParentNode->getPassNumber( this ) ),
                             "CompositorPass::emulateUavExecution" );
            }

            if( !(itor->access & boundUavs[itor->uavSlot].boundAccess) )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "Node " + mParentNode->getName().getFriendlyText() + "; pass #" +
                             StringConverter::toString( mParentNode->getPassNumber( this ) ) +
                             " marked " + ResourceAccess::toString( itor->access ) +
                             " access to UAV at slot " +
                             StringConverter::toString( itor->uavSlot ) + " but this UAV is bound for " +
                             ResourceAccess::toString( boundUavs[itor->uavSlot].boundAccess ) +
                             " access.", "CompositorPass::emulateUavExecution" );
            }

            ResourceAccessMap::iterator itResAccess = uavsAccess.find( uavRt );

            if( itResAccess == uavsAccess.end() )
            {
                //First time accessing the UAV. If we need a barrier,
                //we will see it in the second pass.
                uavsAccess[uavRt] = ResourceAccess::Undefined;
                itResAccess = uavsAccess.find( uavRt );
            }

            ResourceLayoutMap::iterator currentLayout = resourcesLayout.find( uavRt );

            if( currentLayout->second != ResourceLayout::Uav ||
                !( (itor->access == ResourceAccess::Read &&
                    itResAccess->second == ResourceAccess::Read) ||
                   (itor->access == ResourceAccess::Write &&
                    itResAccess->second == ResourceAccess::Write &&
                    itor->allowWriteAfterWrite) ||
                   itResAccess->second == ResourceAccess::Undefined ) )
            {
                //Not RaR (or not WaW when they're explicitly allowed). Insert the barrier.
                //We also may need the barrier if the resource wasn't an UAV.
                addResourceTransition( currentLayout, ResourceLayout::Uav, ReadBarrier::Uav );
            }

            itResAccess->second = itor->access;

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::_removeAllBarriers(void)
    {
        assert( mNumValidResourceTransitions <= mResourceTransitions.size() );

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        ResourceTransitionVec::iterator itor = mResourceTransitions.begin();

        for( size_t i=0; i<mNumValidResourceTransitions; ++i )
        {
            renderSystem->_resourceTransitionDestroyed( &(*itor) );
            ++itor;
        }

        mNumValidResourceTransitions = 0;
        mResourceTransitions.clear();
    }
    //-----------------------------------------------------------------------------------
    bool CompositorPass::notifyRecreated( const TextureGpu *channel )
    {
        if( !mRenderPassDesc )
            return false;

        bool usedByUs = false;

        if( mRenderPassDesc->mDepth.texture == channel )
            usedByUs = true;
        if( mRenderPassDesc->mStencil.texture == channel )
            usedByUs = true;

        for( int i=0; i<OGRE_MAX_MULTIPLE_RENDER_TARGETS && !usedByUs; ++i )
        {
            if( mRenderPassDesc->mColour[i].texture == channel ||
                mRenderPassDesc->mColour[i].resolveTexture == channel )
            {
                usedByUs = true;
            }
        }

        if( usedByUs )
        {
            mNumPassesLeft = mDefinition->mNumInitialPasses;

            //Reset texture pointers and setup RenderPassDescriptor again
            mRenderPassDesc->mDepth.texture = 0;
            mRenderPassDesc->mStencil.texture = 0;

            for( int i=0; i<OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
            {
                mRenderPassDesc->mColour[i].texture = 0;
                mRenderPassDesc->mColour[i].resolveTexture = 0;
            }

            const CompositorNodeDef *nodeDef = mParentNode->getDefinition();
            const CompositorTargetDef *targetDef = mDefinition->getParentTargetDef();
            const RenderTargetViewDef *rtvDef =
                    nodeDef->getRenderTargetViewDef( targetDef->getRenderTargetName() );
            setupRenderPassDesc( rtvDef );
        }

        return usedByUs;
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::notifyRecreated( const UavBufferPacked *oldBuffer, UavBufferPacked *newBuffer )
    {
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::notifyDestroyed( TextureGpu *channel )
    {
        if( !mRenderPassDesc )
            return;

        bool usedByUs = false;

        if( mRenderPassDesc->mDepth.texture == channel )
            usedByUs = true;
        if( mRenderPassDesc->mStencil.texture == channel )
            usedByUs = true;

        for( int i=0; i<OGRE_MAX_MULTIPLE_RENDER_TARGETS && !usedByUs; ++i )
        {
            if( mRenderPassDesc->mColour[i].texture == channel ||
                mRenderPassDesc->mColour[i].resolveTexture == channel )
            {
                usedByUs = true;
            }
        }

        if( usedByUs )
        {
            RenderSystem *renderSystem = mParentNode->getRenderSystem();
            renderSystem->destroyRenderPassDescriptor( mRenderPassDesc );
            mRenderPassDesc = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::notifyDestroyed( const UavBufferPacked *buffer )
    {
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::notifyCleared(void)
    {
        if( mRenderPassDesc )
        {
            RenderSystem *renderSystem = mParentNode->getRenderSystem();
            renderSystem->destroyRenderPassDescriptor( mRenderPassDesc );
            mRenderPassDesc = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::resetNumPassesLeft(void)
    {
        mNumPassesLeft = mDefinition->mNumInitialPasses;
    }
    //-----------------------------------------------------------------------------------
    Vector2 CompositorPass::getActualDimensions(void) const
    {
        return Vector2( floorf( (mAnyTargetTexture->getWidth() >> mAnyMipLevel) *
                                mDefinition->mVpWidth ),
                        floorf( (mAnyTargetTexture->getHeight() >> mAnyMipLevel) *
                                mDefinition->mVpHeight ) );
    }
}
