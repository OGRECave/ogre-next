/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2025 Torus Knot Software Ltd

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

Original implementation by Crashy.
Further enhancements by edherbert.
General fixes: Various people https://forums.ogre3d.org/viewtopic.php?t=93889
Updated for Ogre-Next 2.3 by Vian https://forums.ogre3d.org/viewtopic.php?t=96798
Reworked for Ogre-Next 4.0 by Matias N. Goldberg.

Based on the implementation in https://github.com/edherbert/ogre-next-imgui
-----------------------------------------------------------------------------
*/

#include "OgreCompositorPassImgui.h"

#include "OgreCompositorPassImguiDef.h"
#include "OgreImguiManager.h"

#include "Compositor/OgreCompositorNode.h"
#include "Compositor/OgreTextureDefinition.h"
#include "OgreCamera.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreSceneManager.h"

#include "imgui.h"

namespace Ogre
{
    CompositorPassImgui::CompositorPassImgui( const CompositorPassImguiDef *definition,
                                              Camera *defaultCamera, SceneManager *sceneManager,
                                              const RenderTargetViewDef *rtv, CompositorNode *parentNode,
                                              ImguiManager *imguiManager ) :
        CompositorPass( definition, parentNode ),
        mSceneManager( sceneManager ),
        mCamera( 0 ),
        mImguiManager( imguiManager ),
        mDefinition( definition )
    {
        initialize( rtv );

        mCamera = defaultCamera;

        mTextureName = rtv->colourAttachments[0].textureName;
        TextureGpu *texture = mParentNode->getDefinedTexture( mTextureName );
        setResolutionToImgui( texture->getWidth(), texture->getHeight() );
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassImgui::execute( const Camera *lodCamera )
    {
        // Execute a limited number of times?
        if( mNumPassesLeft != std::numeric_limits<uint32>::max() )
        {
            if( !mNumPassesLeft )
                return;
            --mNumPassesLeft;
        }

        profilingBegin();

        notifyPassEarlyPreExecuteListeners();

        analyzeBarriers();
        executeResourceTransitions();

        SceneManager *sceneManager = mCamera->getSceneManager();
        sceneManager->_setCamerasInProgress( CamerasInProgress( mCamera ) );
        sceneManager->_setCurrentCompositorPass( this );

        // Fire the listener in case it wants to change anything
        notifyPassPreExecuteListeners();

        mImguiManager->drawIntoCompositor( mRenderPassDesc, mAnyTargetTexture, mSceneManager, mCamera );

        sceneManager->_setCurrentCompositorPass( 0 );

        notifyPassPosExecuteListeners();

        profilingEnd();
    }
    //-----------------------------------------------------------------------------------
    void CompositorPassImgui::setResolutionToImgui( uint32 width, uint32 height )
    {
        if( !mDefinition->mSetsResolution )
            return;

        ImGuiIO &io = ImGui::GetIO();

        if( uint32( io.DisplaySize.x ) != width || uint32( io.DisplaySize.y ) != height )
            io.DisplaySize = ImVec2( float( width ), float( height ) );
    }
    //-----------------------------------------------------------------------------------
    bool CompositorPassImgui::notifyRecreated( const TextureGpu *channel )
    {
        bool usedByUs = CompositorPass::notifyRecreated( channel );

        if( !usedByUs )
        {
            // Because of mSkipLoadStoreSemantics, notifyRecreated may return false incorrectly.
            // Technically, we ignore mSkipLoadStoreSemantics so this block of code is useless.
            // But I'm leaving it anyway in case that changes in the future.
            TextureGpu *texture = mParentNode->getDefinedTexture( mTextureName );
            usedByUs = texture == channel;
        }

        if( usedByUs &&  //
            !PixelFormatGpuUtils::isDepth( channel->getPixelFormat() ) &&
            !PixelFormatGpuUtils::isStencil( channel->getPixelFormat() ) )
        {
            setResolutionToImgui( channel->getWidth(), channel->getHeight() );
        }

        return usedByUs;
    }
}  // namespace Ogre
