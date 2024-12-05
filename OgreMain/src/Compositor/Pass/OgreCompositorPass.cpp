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

#include "Compositor/Pass/OgreCompositorPass.h"

#include "Compositor/OgreCompositorChannel.h"
#include "Compositor/OgreCompositorManager2.h"
#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "OgreLogManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreProfiler.h"
#include "OgreRenderSystem.h"
#include "OgreStringConverter.h"
#include "OgreViewport.h"

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
    // Can NOT use UNIT_X/UNIT_Y and Degree here because on iOS C++ initialization
    // order screws us and thus key global variables are still 0. Hardcode the values.
    const Quaternion CompositorPass::CubemapRotations[6] = {
        Quaternion( Radian( -1.570796f ), Vector3( 0, 1, 0 ) ),  //+X
        Quaternion( Radian( 1.570796f ), Vector3( 0, 1, 0 ) ),   //-X
        Quaternion( Radian( 1.570796f ), Vector3( 1, 0, 0 ) ),   //+Y
        Quaternion( Radian( -1.570796f ), Vector3( 1, 0, 0 ) ),  //-Y
        Quaternion( 1, 0, 0, 0 ),                                //+Z
        Quaternion( Radian( 3.1415927f ), Vector3( 0, 1, 0 ) )   //-Z
    };

    CompositorPass::CompositorPass( const CompositorPassDef *definition, CompositorNode *parentNode ) :
        mDefinition( definition ),
        mRenderPassDesc( 0 ),
        mAnyTargetTexture( 0 ),
        mAnyMipLevel( 0u ),
        mNumPassesLeft( definition->mNumInitialPasses ),
        mParentNode( parentNode ),
        mBarrierSolver( parentNode->getWorkspace()->getCompositorManager()->getBarrierSolver() )
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
                         "Only a few compositor passes support this "
                         "(e.g. Compute passes, Shadows passes)\n"
                         "Node with error: " +
                             mDefinition->getParentTargetDef()->getParentNodeDef()->getNameStr() + "\n",
                         "CompositorPass::CompositorPass" );
        }

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        if( rtv )
        {
            mRenderPassDesc = renderSystem->createRenderPassDescriptor();
            setupRenderPassDesc( rtv );

            mRenderPassDesc->findAnyTexture( &mAnyTargetTexture, mAnyMipLevel );
        }

        populateTextureDependenciesFromExposedTextures();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::setViewportSizeToViewport( size_t vpIdx, Viewport *outVp )
    {
        if( mDefinition->mNumViewports == 0u )
            return;

        CompositorWorkspace *workspace = mParentNode->getWorkspace();
        uint8 workspaceVpMask = workspace->getViewportModifierMask();

        bool applyModifier = ( workspaceVpMask & mDefinition->mViewportModifierMask ) != 0;
        Vector4 vpModifier = applyModifier ? workspace->getViewportModifier() : Vector4( 0, 0, 1, 1 );

        vpIdx = std::min<size_t>( vpIdx, mDefinition->mNumViewports - 1u );

        Vector4 vpSize;
        Vector4 scissors;

        const Real left = mDefinition->mVpRect[vpIdx].mVpLeft + vpModifier.x;
        const Real top = mDefinition->mVpRect[vpIdx].mVpTop + vpModifier.y;
        const Real width = mDefinition->mVpRect[vpIdx].mVpWidth * vpModifier.z;
        const Real height = mDefinition->mVpRect[vpIdx].mVpHeight * vpModifier.w;

        const Real scLeft = mDefinition->mVpRect[vpIdx].mVpScissorLeft + vpModifier.x;
        const Real scTop = mDefinition->mVpRect[vpIdx].mVpScissorTop + vpModifier.y;
        const Real scWidth = mDefinition->mVpRect[vpIdx].mVpScissorWidth * vpModifier.z;
        const Real scHeight = mDefinition->mVpRect[vpIdx].mVpScissorHeight * vpModifier.w;

        vpSize = Vector4( left, top, width, height );
        scissors = Vector4( scLeft, scTop, scWidth, scHeight );

        outVp->setDimensions( mAnyTargetTexture, vpSize, scissors, mAnyMipLevel );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::setRenderPassDescToCurrent()
    {
        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        if( mDefinition->mSkipLoadStoreSemantics )
        {
            OGRE_ASSERT_MEDIUM(
                ( mDefinition->getType() == PASS_QUAD || mDefinition->getType() == PASS_SCENE ||
                  mDefinition->getType() == PASS_WARM_UP || mDefinition->getType() == PASS_CUSTOM ) &&
                "mSkipLoadStoreSemantics is only intended for use with pass quad, scene. warm_up & "
                "custom" );

            const RenderPassDescriptor *currRenderPassDesc = renderSystem->getCurrentPassDescriptor();
            if( !currRenderPassDesc )
            {
                LogManager::getSingleton().logMessage(
                    "mSkipLoadStoreSemantics was requested but there is no active render pass "
                    "descriptor. Disable this setting for pass " +
                        mDefinition->mProfilingId + " with RT " +
                        mDefinition->getParentTargetDef()->getRenderTargetNameStr() +
                        "\n - Check the contents of CompositorPass::mResourceTransitions to see if you "
                        "can "
                        "move them to a barrier pass",
                    LML_CRITICAL );
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "mSkipLoadStoreSemantics was requested but there is no active render pass "
                             "descriptor.\nSee Ogre.log for details",
                             "CompositorPass::setRenderPassDescToCurrent" );
            }

            return;
        }

        CompositorWorkspace *workspace = mParentNode->getWorkspace();
        uint8 workspaceVpMask = workspace->getViewportModifierMask();

        bool applyModifier = ( workspaceVpMask & mDefinition->mViewportModifierMask ) != 0;
        Vector4 vpModifier = applyModifier ? workspace->getViewportModifier() : Vector4( 0, 0, 1, 1 );

        const uint32 numViewports = mDefinition->mNumViewports;
        Vector4 vpSize[16];
        Vector4 scissors[16];

        for( size_t i = 0; i < numViewports; ++i )
        {
            Real left = mDefinition->mVpRect[i].mVpLeft + vpModifier.x;
            Real top = mDefinition->mVpRect[i].mVpTop + vpModifier.y;
            Real width = mDefinition->mVpRect[i].mVpWidth * vpModifier.z;
            Real height = mDefinition->mVpRect[i].mVpHeight * vpModifier.w;

            Real scLeft = mDefinition->mVpRect[i].mVpScissorLeft + vpModifier.x;
            Real scTop = mDefinition->mVpRect[i].mVpScissorTop + vpModifier.y;
            Real scWidth = mDefinition->mVpRect[i].mVpScissorWidth * vpModifier.z;
            Real scHeight = mDefinition->mVpRect[i].mVpScissorHeight * vpModifier.w;

            vpSize[i] = Vector4( left, top, width, height );
            scissors[i] = Vector4( scLeft, scTop, scWidth, scHeight );
        }

        renderSystem->beginRenderPassDescriptor(
            mRenderPassDesc, mAnyTargetTexture, mAnyMipLevel, vpSize, scissors, numViewports,
            mDefinition->mIncludeOverlays, mDefinition->mWarnIfRtvWasFlushed );
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
                tmpRtv.depthBufferId = texture->getDepthBufferPoolId();
                tmpRtv.preferDepthTexture = texture->getPreferDepthTexture();
                tmpRtv.depthBufferFormat = texture->getDesiredDepthBufferFormat();
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
                             " colour attachments for RTV in " +
                             mParentNode->getName().getFriendlyText(),
                         "CompositorPass::createRenderPassDesc" );
        }

        for( size_t i = 0; i < numColourAttachments; ++i )
        {
            renderPassDesc->mColour[i].loadAction = mDefinition->mLoadActionColour[i];
            renderPassDesc->mColour[i].storeAction = mDefinition->mStoreActionColour[i];
            renderPassDesc->mColour[i].clearColour = mDefinition->mClearColour[i];
            renderPassDesc->mColour[i].allLayers = rtv->colourAttachments[i].colourAllLayers;
            setupRenderPassTarget( &renderPassDesc->mColour[i], rtv->colourAttachments[i], true );
        }

        renderPassDesc->mDepth.loadAction = mDefinition->mLoadActionDepth;
        renderPassDesc->mDepth.storeAction = mDefinition->mStoreActionDepth;
        renderPassDesc->mDepth.clearDepth = mDefinition->mClearDepth;
        renderPassDesc->mDepth.readOnly = rtv->depthReadOnly || mDefinition->mReadOnlyDepth;
        setupRenderPassTarget( &renderPassDesc->mDepth, rtv->depthAttachment, false,
                               renderPassDesc->mColour[0].texture, rtv->depthBufferId,
                               rtv->preferDepthTexture, rtv->depthBufferFormat );

        // Check if stencil is required.
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
            renderPassDesc->mStencil.loadAction = mDefinition->mLoadActionStencil;
            renderPassDesc->mStencil.storeAction = mDefinition->mStoreActionStencil;
            renderPassDesc->mStencil.clearStencil = mDefinition->mClearStencil;
            renderPassDesc->mStencil.readOnly = rtv->stencilReadOnly || mDefinition->mReadOnlyStencil;
            setupRenderPassTarget( &renderPassDesc->mStencil, rtv->stencilAttachment, false,
                                   renderPassDesc->mColour[0].texture, rtv->depthBufferId,
                                   rtv->preferDepthTexture, rtv->depthBufferFormat );
        }

        postRenderPassDescriptorSetup( renderPassDesc );

        try
        {
            renderPassDesc->entriesModified( RenderPassDescriptor::All );
        }
        catch( Exception &e )
        {
            LogManager::getSingleton().logMessage(
                "The compositor pass '" + mDefinition->mProfilingId + "' from Node: '" +
                    mParentNode->getDefinition()->getNameStr() + "' in Workspace: '" +
                    mParentNode->getWorkspace()->getDefinition()->getNameStr() +
                    "' threw the following Exception:",
                LML_CRITICAL );
            throw;
        }
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
            // This is depth or stencil
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
                renderPassTargetAttachment->texture = renderSystem->getDepthBufferFor(
                    linkedColourAttachment, depthBufferId, preferDepthTexture, depthBufferFormat );
            }
        }
        else if( rtvEntry.textureName != IdString() )
        {
            // This is colour
            renderPassTargetAttachment->texture = mParentNode->getDefinedTexture( rtvEntry.textureName );

            if( !renderPassTargetAttachment->texture )
            {
                OGRE_EXCEPT(
                    Exception::ERR_ITEM_NOT_FOUND,
                    "Couldn't find texture for RTV with name " + rtvEntry.textureName.getFriendlyText(),
                    "CompositorPass::setupRenderPassTarget" );
            }

            // Deal with MSAA resolve textures.
            if( renderPassTargetAttachment->texture->isMultisample() )
            {
                if( renderPassTargetAttachment->storeAction == StoreAction::MultisampleResolve ||
                    renderPassTargetAttachment->storeAction == StoreAction::StoreAndMultisampleResolve ||
                    ( renderPassTargetAttachment->storeAction == StoreAction::StoreOrResolve &&
                      ( !renderPassTargetAttachment->texture->hasMsaaExplicitResolves() ||
                        rtvEntry.resolveTextureName != IdString() ) ) )
                {
                    // If we're here, the texture is MSAA _AND_ we'll resolve it.
                    if( rtvEntry.resolveTextureName == IdString() )
                    {
                        if( !allowResolveStoreActionsWithoutResolveTexture() )
                        {
                            if( renderPassTargetAttachment->texture->hasMsaaExplicitResolves() )
                            {
                                OGRE_EXCEPT(
                                    Exception::ERR_INVALIDPARAMS,
                                    "Must specify resolveTextureName for RTV when using explicit "
                                    "resolves and store action is either "
                                    "StoreAction::MultisampleResolve, "
                                    "StoreAction::StoreAndMultisampleResolve. "
                                    "Texture: " +
                                        renderPassTargetAttachment->texture->getNameStr(),
                                    "CompositorPass::setupRenderPassTarget" );
                            }

                            renderPassTargetAttachment->resolveTexture =
                                renderPassTargetAttachment->texture;
                        }
                        else
                        {
                            renderPassTargetAttachment->resolveTexture = 0;
                        }
                    }
                    else
                    {
                        // Resolve texture is explicitly defined.
                        // Allow this even if the texture has implicit resolves.
                        renderPassTargetAttachment->resolveTexture =
                            mParentNode->getDefinedTexture( rtvEntry.resolveTextureName );

                        if( !renderPassTargetAttachment->resolveTexture )
                        {
                            OGRE_EXCEPT( Exception::ERR_ITEM_NOT_FOUND,
                                         "Couldn't find resolve texture for RTV with name " +
                                             rtvEntry.resolveTextureName.getFriendlyText(),
                                         "CompositorPass::setupRenderPassTarget" );
                        }

                        if( renderPassTargetAttachment->resolveTexture->isMultisample() )
                        {
                            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                         "Workspace: '" +
                                             mParentNode->getWorkspace()->getDefinition()->getNameStr() +
                                             "' Node: '" + mParentNode->getName().getFriendlyText() +
                                             "'\nCannot specify a non-MSAA texture for resolving an "
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

        renderPassTargetAttachment->mipLevel = rtvEntry.mipLevel;
        renderPassTargetAttachment->resolveMipLevel = rtvEntry.resolveMipLevel;
        if( !isColourAttachment || mDefinition->getRtIndex() == 0 )
        {
            renderPassTargetAttachment->slice = rtvEntry.slice;
            renderPassTargetAttachment->resolveSlice = rtvEntry.resolveSlice;
        }
        else
        {
            // This is colour target asking to override the RTV.
            // It's common when doing MSAA that only the resolveSlice can be > 0 so we check for that
            if( renderPassTargetAttachment->texture &&
                mDefinition->getRtIndex() < renderPassTargetAttachment->texture->getNumSlices() )
            {
                renderPassTargetAttachment->slice = (uint16)mDefinition->getRtIndex();
            }
            renderPassTargetAttachment->resolveSlice = (uint16)mDefinition->getRtIndex();
        }

        if( renderPassTargetAttachment->storeAction == StoreAction::StoreOrResolve )
        {
            if( !renderPassTargetAttachment->texture )
                renderPassTargetAttachment->storeAction = StoreAction::DontCare;
            else
            {
                if( renderPassTargetAttachment->texture->isMultisample() &&
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
    void CompositorPass::profilingBegin()
    {
#if OGRE_PROFILING
        if( !mParentNode->getWorkspace()->getAmalgamatedProfiling() )
        {
            OgreProfileBeginDynamic( mDefinition->mProfilingId.c_str() );
            OgreProfileGpuBeginDynamic( mDefinition->mProfilingId );
        }
#endif
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        {
            RenderSystem *renderSystem = mParentNode->getRenderSystem();
            renderSystem->debugAnnotationPush( mDefinition->mProfilingId );
        }
#endif
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::profilingEnd()
    {
#if OGRE_DEBUG_MODE >= OGRE_DEBUG_MEDIUM
        {
            RenderSystem *renderSystem = mParentNode->getRenderSystem();
            renderSystem->debugAnnotationPop();
        }
#endif
        if( mDefinition->mFlushCommandBuffers )
        {
            RenderSystem *renderSystem = mParentNode->getRenderSystem();
            renderSystem->flushCommands();
        }
#if OGRE_PROFILING
        if( !mParentNode->getWorkspace()->getAmalgamatedProfiling() )
        {
            OgreProfileEnd( mDefinition->mProfilingId );
            OgreProfileGpuEnd( mDefinition->mProfilingId );
        }
#endif
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::populateTextureDependenciesFromExposedTextures()
    {
        IdStringVec::const_iterator itor = mDefinition->mExposedTextures.begin();
        IdStringVec::const_iterator endt = mDefinition->mExposedTextures.end();

        while( itor != endt )
        {
            TextureGpu *channel = mParentNode->getDefinedTexture( *itor );
            mTextureDependencies.push_back( CompositorTexture( *itor, channel ) );

            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::executeResourceTransitions()
    {
        OGRE_ASSERT_MEDIUM( ( mResourceTransitions.empty() || !mDefinition->mSkipLoadStoreSemantics ) &&
                            "Cannot set mSkipLoadStoreSemantics if there will be resource "
                            "transitions. Try englobing all affected passes in a barrier pass" );
        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        renderSystem->executeResourceTransition( mResourceTransitions );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::notifyPassEarlyPreExecuteListeners()
    {
        const CompositorWorkspaceListenerVec &listeners = mParentNode->getWorkspace()->getListeners();

        CompositorWorkspaceListenerVec::const_iterator itor = listeners.begin();
        CompositorWorkspaceListenerVec::const_iterator endt = listeners.end();

        while( itor != endt )
        {
            ( *itor )->passEarlyPreExecute( this );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::notifyPassPreExecuteListeners()
    {
        const CompositorWorkspaceListenerVec &listeners = mParentNode->getWorkspace()->getListeners();

        CompositorWorkspaceListenerVec::const_iterator itor = listeners.begin();
        CompositorWorkspaceListenerVec::const_iterator endt = listeners.end();

        while( itor != endt )
        {
            ( *itor )->passPreExecute( this );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::notifyPassPosExecuteListeners()
    {
        const CompositorWorkspaceListenerVec &listeners = mParentNode->getWorkspace()->getListeners();

        CompositorWorkspaceListenerVec::const_iterator itor = listeners.begin();
        CompositorWorkspaceListenerVec::const_iterator endt = listeners.end();

        while( itor != endt )
        {
            ( *itor )->passPosExecute( this );
            ++itor;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::resolveTransition( TextureGpu *texture, ResourceLayout::Layout newLayout,
                                            ResourceAccess::ResourceAccess access, uint8 stageMask )
    {
        mBarrierSolver.resolveTransition( mResourceTransitions, texture, newLayout, access, stageMask );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::resolveTransition( GpuTrackedResource *bufferRes,
                                            ResourceAccess::ResourceAccess access, uint8 stageMask )
    {
        mBarrierSolver.resolveTransition( mResourceTransitions, bufferRes, access, stageMask );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::analyzeBarriers( const bool bClearBarriers )
    {
        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        renderSystem->endCopyEncoder();

        if( bClearBarriers )
            mResourceTransitions.clear();

        if( mRenderPassDesc && !mDefinition->mSkipLoadStoreSemantics )
        {
            // Check <anything> -> RT
            for( int i = 0; i < mRenderPassDesc->getNumColourEntries(); ++i )
            {
                RenderPassColourTarget &colourRenderPass = mRenderPassDesc->mColour[i];
                if( !colourRenderPass.resolveTexture )
                {
                    resolveTransition( colourRenderPass.texture, ResourceLayout::RenderTarget,
                                       ResourceAccess::ReadWrite, 0u );
                }
                else

                {
                    resolveTransition( colourRenderPass.resolveTexture, ResourceLayout::ResolveDest,
                                       ResourceAccess::Write, 0u );
                    if( colourRenderPass.resolveTexture != colourRenderPass.texture )
                    {
                        resolveTransition( colourRenderPass.texture, ResourceLayout::RenderTarget,
                                           ResourceAccess::ReadWrite, 0u );
                    }
                }
            }
            if( mRenderPassDesc->mDepth.texture )
            {
                if( !mRenderPassDesc->mDepth.readOnly )
                {
                    resolveTransition( mRenderPassDesc->mDepth.texture, ResourceLayout::RenderTarget,
                                       ResourceAccess::ReadWrite, 0u );
                }
                else
                {
                    resolveTransition( mRenderPassDesc->mDepth.texture,
                                       ResourceLayout::RenderTargetReadOnly, ResourceAccess::Read, 0u );
                }
            }
            if( mRenderPassDesc->mStencil.texture &&
                mRenderPassDesc->mStencil.texture != mRenderPassDesc->mDepth.texture )
            {
                if( !mRenderPassDesc->mDepth.readOnly && !mRenderPassDesc->mStencil.readOnly )
                {
                    resolveTransition( mRenderPassDesc->mStencil.texture, ResourceLayout::RenderTarget,
                                       ResourceAccess::ReadWrite, 0u );
                }
                else
                {
                    resolveTransition( mRenderPassDesc->mStencil.texture,
                                       ResourceLayout::RenderTargetReadOnly, ResourceAccess::Read, 0u );
                }
            }
        }

        {
            // Check <anything> -> Texture
            CompositorTextureVec::const_iterator itDep = mTextureDependencies.begin();
            CompositorTextureVec::const_iterator enDep = mTextureDependencies.end();

            while( itDep != enDep )
            {
                TextureGpu *renderTarget = itDep->texture;

                resolveTransition( renderTarget, ResourceLayout::Texture, ResourceAccess::Read,
                                   c_allGraphicStagesMask );
                ++itDep;
            }
        }

        // Check <anything> -> UAV
        CompositorPassDef::UavDependencyVec::const_iterator itor = mDefinition->mUavDependencies.begin();
        CompositorPassDef::UavDependencyVec::const_iterator endt = mDefinition->mUavDependencies.end();

        while( itor != endt )
        {
            BoundUav boundUav = renderSystem->getBoundUav( itor->uavSlot );
            GpuTrackedResource *uavRt = boundUav.rttOrBuffer;

            if( !uavRt )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "No UAV is bound at slot " + StringConverter::toString( itor->uavSlot ) +
                                 " but it is marked as used by node " +
                                 mParentNode->getName().getFriendlyText() + "; pass #" +
                                 StringConverter::toString( mParentNode->getPassNumber( this ) ),
                             "CompositorPass::emulateUavExecution" );
            }

            if( !( itor->access & boundUav.boundAccess ) )
            {
                OGRE_EXCEPT( Exception::ERR_INVALID_STATE,
                             "Node " + mParentNode->getName().getFriendlyText() + "; pass #" +
                                 StringConverter::toString( mParentNode->getPassNumber( this ) ) +
                                 " marked " + ResourceAccess::toString( itor->access ) +
                                 " access to UAV at slot " + StringConverter::toString( itor->uavSlot ) +
                                 " but this UAV is bound for " +
                                 ResourceAccess::toString( boundUav.boundAccess ) + " access.",
                             "CompositorPass::emulateUavExecution" );
            }

            if( uavRt->isTextureGpu() )
            {
                OGRE_ASSERT_HIGH( dynamic_cast<TextureGpu *>( uavRt ) );
                resolveTransition( static_cast<TextureGpu *>( uavRt ), ResourceLayout::Uav, itor->access,
                                   c_allGraphicStagesMask );
            }
            else
            {
                resolveTransition( uavRt, itor->access, c_allGraphicStagesMask );
            }

            ++itor;
        }
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

        for( int i = 0; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS && !usedByUs; ++i )
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

            // Reset texture pointers and setup RenderPassDescriptor again
            RenderSystem *renderSystem = mParentNode->getRenderSystem();
            if( mRenderPassDesc->mDepth.texture )
            {
                renderSystem->_dereferenceSharedDepthBuffer( mRenderPassDesc->mDepth.texture );
                mRenderPassDesc->mDepth.texture = 0;

                if( mRenderPassDesc->mStencil.texture &&
                    mRenderPassDesc->mStencil.texture == mRenderPassDesc->mDepth.texture )
                {
                    renderSystem->_dereferenceSharedDepthBuffer( mRenderPassDesc->mStencil.texture );
                    mRenderPassDesc->mStencil.texture = 0;
                }
            }
            if( mRenderPassDesc->mStencil.texture )
            {
                renderSystem->_dereferenceSharedDepthBuffer( mRenderPassDesc->mStencil.texture );
                mRenderPassDesc->mStencil.texture = 0;
            }

            for( int i = 0; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS; ++i )
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

        for( int i = 0; i < OGRE_MAX_MULTIPLE_RENDER_TARGETS && !usedByUs; ++i )
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
    void CompositorPass::notifyDestroyed( const UavBufferPacked *buffer ) {}
    //-----------------------------------------------------------------------------------
    void CompositorPass::notifyCleared()
    {
        if( mRenderPassDesc )
        {
            RenderSystem *renderSystem = mParentNode->getRenderSystem();
            renderSystem->destroyRenderPassDescriptor( mRenderPassDesc );
            mRenderPassDesc = 0;
        }
    }
    //-----------------------------------------------------------------------------------
    void CompositorPass::resetNumPassesLeft() { mNumPassesLeft = mDefinition->mNumInitialPasses; }
    //-----------------------------------------------------------------------------------
    Real CompositorPass::getViewportAspectRatio( size_t vpIdx )
    {
        if( mDefinition->mNumViewports == 0u )
            return 1.0f;

        CompositorWorkspace *workspace = mParentNode->getWorkspace();
        uint8 workspaceVpMask = workspace->getViewportModifierMask();

        bool applyModifier = ( workspaceVpMask & mDefinition->mViewportModifierMask ) != 0;
        Vector4 vpModifier = applyModifier ? workspace->getViewportModifier() : Vector4( 0, 0, 1, 1 );

        vpIdx = std::min<size_t>( vpIdx, mDefinition->mNumViewports - 1u );

        const Real relWidth = mDefinition->mVpRect[vpIdx].mVpWidth * vpModifier.z;
        const Real relHeight = mDefinition->mVpRect[vpIdx].mVpHeight * vpModifier.w;

        const Real fWidth =
            mAnyTargetTexture ? (Real)( mAnyTargetTexture->getWidth() >> mAnyMipLevel ) : 0.0f;
        const Real fHeight =
            mAnyTargetTexture ? (Real)( mAnyTargetTexture->getHeight() >> mAnyMipLevel ) : 0.0f;

        int actWidth = (int)( relWidth * fWidth );
        int actHeight = (int)( relHeight * fHeight );

        return (Real)actWidth / (Real)std::max( 1, actHeight );
    }
    //-----------------------------------------------------------------------------------
    Vector2 CompositorPass::getActualDimensions() const
    {
        return Vector2( floorf( float( mAnyTargetTexture->getWidth() >> mAnyMipLevel ) *
                                mDefinition->mVpRect[0].mVpWidth ),
                        floorf( float( mAnyTargetTexture->getHeight() >> mAnyMipLevel ) *
                                mDefinition->mVpRect[0].mVpHeight ) );
    }
}  // namespace Ogre
