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
#ifndef _OgreImguiManager_H_
#define _OgreImguiManager_H_

#include "OgreImguiPrerequisites.h"

struct ImDrawList;
typedef uint16_t ImDrawIdx;
struct ImDrawVert;

#include <map>

#include "OgreHeaderPrefix.h"

struct ImFontAtlas;
struct ImDrawData;

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class _OgreImguiExport ImguiManager
    {
        typedef std::vector<ImguiRenderable *>             ImguiRenderableVec;
        typedef std::map<TextureGpu *, ImguiRenderableVec> ImguiRenderableMap;

        ImguiRenderableMap mAvailableRenderables;
        ImguiRenderableVec mScheduledRenderables;

        ImDrawData const *mDrawData;

        IndirectBufferPacked *ogre_nullable mIndirectBuffer;
        CommandBuffer                      *mCommandBuffer;

        TextureGpu *mFontTex;

        MovableObject *mDummyMovableObject;

        /// Ensures all shaders are created.
        void createPrograms();

        /// Creates a Material for the given texture. Currently, Texture MUST not be nullptr.
        /// Assumes shaders exist (see createPrograms()).
        MaterialPtr createMaterialFor( TextureGpu *texture );

        /// Recycles or creates a new Renderable that can display the given texture.
        ImguiRenderable *getAvailableRenderable( TextureGpu *texture );

        /// Frees all resources.
        void destroyAllResources();

        Matrix4 getProjectionMatrix( RenderSystem *rs, const bool bRequiresTextureFlipping,
                                     const Camera *currentCamera ) const;

    public:
        ImguiManager();
        ~ImguiManager();

        /** Setups the font texture provided by IMGUI and uploads it to GPU.
        @param fontAtlas
            Imgui's font atlas.
        @param bUseSynchronousUpload
            When true, this call will return once we're done uploading to GPU.
            When false, this call will return almost immediately and upload will happen in the
            background. Note however if you never call TextureGpuManager::waitForStreamingCompletion or
            mFontTex->waitForData(), the texture may "glitch" if it's presented on screen before we are
            done uploading it.
        */
        void setupFont( ImFontAtlas *fontAtlas, bool bUseSynchronousUpload );

        void prepareForRender( SceneManager *sceneManager );

        void drawIntoCompositor( RenderPassDescriptor *renderPassDesc, TextureGpu *anyTargetTexture,
                                 SceneManager *sceneManager, const Camera *currentCamera );
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
