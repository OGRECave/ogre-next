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

#include "Compositor/Pass/PassDepthCopy/OgreCompositorPassDepthCopy.h"

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreCompositorNodeDef.h"
#include "Compositor/OgreCompositorWorkspace.h"
#include "Compositor/OgreCompositorWorkspaceListener.h"
#include "Compositor/Pass/PassDepthCopy/OgreCompositorPassDepthCopyDef.h"
#include "OgreDepthBuffer.h"
#include "OgreRenderSystem.h"
#include "OgreTextureBox.h"

namespace Ogre
{
    void CompositorPassDepthCopyDef::setDepthTextureCopy( const String &srcTextureName,
                                                          const String &dstTextureName )
    {
        if( srcTextureName.find( "global_" ) == 0 )
        {
            mParentNodeDef->addTextureSourceName( srcTextureName, 0,
                                                  TextureDefinitionBase::TEXTURE_GLOBAL );
        }
        if( dstTextureName.find( "global_" ) == 0 )
        {
            mParentNodeDef->addTextureSourceName( dstTextureName, 0,
                                                  TextureDefinitionBase::TEXTURE_GLOBAL );
        }

        mSrcDepthTextureName = srcTextureName;
        mDstDepthTextureName = dstTextureName;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    CompositorPassDepthCopy::CompositorPassDepthCopy( const CompositorPassDepthCopyDef *definition,
                                                      const RenderTargetViewDef *rtv,
                                                      CompositorNode *parentNode ) :
        CompositorPass( definition, parentNode ),
        mDefinition( definition )
    {
        initialize( 0, true );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassDepthCopy::execute( const Camera *lodCamera )
    {
        // Execute a limited number of times?
        if( mNumPassesLeft != std::numeric_limits<uint32>::max() )
        {
            if( !mNumPassesLeft )
                return;
            --mNumPassesLeft;
        }

        notifyPassEarlyPreExecuteListeners();

        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        renderSystem->endRenderPassDescriptor();

        analyzeBarriers();
        executeResourceTransitions();

        // Fire the listener in case it wants to change anything
        notifyPassPreExecuteListeners();

        // Should we retrieve every update, or cache the return values
        // and listen to notifyRecreated and family of funtions?
        TextureGpu *srcChannel = mParentNode->getDefinedTexture( mDefinition->mSrcDepthTextureName );
        TextureGpu *dstChannel = mParentNode->getDefinedTexture( mDefinition->mDstDepthTextureName );

        TextureBox srcBox = srcChannel->getEmptyBox( 0 );
        TextureBox dstBox = dstChannel->getEmptyBox( 0 );
        srcChannel->copyTo( dstChannel, dstBox, 0, srcBox, 0, false,
                            CopyEncTransitionMode::AlreadyInLayoutThenAuto,
                            CopyEncTransitionMode::AlreadyInLayoutThenAuto );

        notifyPassPosExecuteListeners();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassDepthCopy::analyzeBarriers( const bool bClearBarriers )
    {
        RenderSystem *renderSystem = mParentNode->getRenderSystem();
        renderSystem->endCopyEncoder();

        if( bClearBarriers )
            mResourceTransitions.clear();

        // Do not use base class'
        // CompositorPass::analyzeBarriers( bClearBarriers );

        TextureGpu *srcChannel = mParentNode->getDefinedTexture( mDefinition->mSrcDepthTextureName );
        TextureGpu *dstChannel = mParentNode->getDefinedTexture( mDefinition->mDstDepthTextureName );

        resolveTransition( srcChannel, ResourceLayout::CopySrc, ResourceAccess::Read, 0u );
        resolveTransition( dstChannel, ResourceLayout::CopyDst, ResourceAccess::Write, 0u );
    }
}  // namespace Ogre
