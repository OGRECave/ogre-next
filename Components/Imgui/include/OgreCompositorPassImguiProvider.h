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

#ifndef _OgreCompositorPassImguiProvider_H_
#define _OgreCompositorPassImguiProvider_H_

#include "OgreImguiPrerequisites.h"

#include "Compositor/Pass/OgreCompositorPassProvider.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    /** @ingroup Api_Backend
    @class CompositorPassImguiProvider
        Generates the CompositorPassImgui pass needed to render imgui UI
        Also ensures the compositor scripts recognize the pass type "dear_imgui"
        i.e.
        @code
            pass custom dear_imgui
            {
            }
        @endcode
    */
    class _OgreImguiExport CompositorPassImguiProvider : public CompositorPassProvider
    {
        ImguiManager *mImguiManager;

    public:
        CompositorPassImguiProvider( ImguiManager *imguiManager );

        ImguiManager *getColibriManager() const { return mImguiManager; }

        /** Called from CompositorTargetDef::addPass when adding a Compositor Pass of type 'custom'
        @param passType
        @param customId
            Arbitrary ID in case there is more than one type of custom pass you want to implement.
            Defaults to IdString()
        @param rtIndex
        @param parentNodeDef
        @return
        */
        CompositorPassDef *addPassDef( CompositorPassType passType, IdString customId,
                                       CompositorTargetDef *parentTargetDef,
                                       CompositorNodeDef   *parentNodeDef ) override;

        /** Creates a CompositorPass from a CompositorPassDef for Compositor Pass of type 'custom'
        @remarks    If you have multiple custom pass types then you will need to use dynamic_cast<>()
                    on the CompositorPassDef to determine what custom pass it is.
        */
        CompositorPass *addPass( const CompositorPassDef *definition, Camera *defaultCamera,
                                 CompositorNode *parentNode, const RenderTargetViewDef *rtvDef,
                                 SceneManager *sceneManager ) override;

        void translateCustomPass( ScriptCompiler *compiler, const AbstractNodePtr &node,
                                  IdString customId, CompositorPassDef *customPassDef ) override;
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#endif
