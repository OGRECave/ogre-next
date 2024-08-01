/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2023 Torus Knot Software Ltd

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

#ifndef __CompositorPassWarmUp_H__
#define __CompositorPassWarmUp_H__

#include "Compositor/OgreCompositorCommon.h"
#include "Compositor/Pass/OgreCompositorPass.h"
#include "OgreMaterial.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorPassWarmUpDef;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Effects
     *  @{
     */

    /** Implementation of CompositorPass

        This implementation simulates rendering but doesn't actually render and tries to bypass
        as much as possible.

        The main goal is to reach shader compilation and PSO creation of all objects on scene,
        taking visibility flags and RenderQueue IDs into consideration but ignoring camera frustum
        culling.

        Due to some technical challenges, we require more information than what should be strictly
        necessary. For example we require a valid Camera pointer because some V1 objects include
        Vertex Layout creation inside updateRenderQueue (which requires a camera to perform
        per-billboard orientation facing), however a Camera in theory shouldn't be necessary.
    @author
        Matias N. Goldberg
    @version
        1.0
    */
    class _OgreExport CompositorPassWarmUp : public CompositorPass
    {
        CompositorPassWarmUpDef const *mDefinition;

    protected:
        CompositorShadowNode *mShadowNode;
        Camera               *mCamera;
        bool                  mUpdateShadowNode;

        void notifyPassSceneAfterShadowMapsListeners();

    public:
        CompositorPassWarmUp( const CompositorPassWarmUpDef *definition, Camera *defaultCamera,
                              CompositorNode *parentNode, const RenderTargetViewDef *rtv );
        ~CompositorPassWarmUp() override = default;

        void execute( const Camera *lodCamera ) override;

        Camera *getCamera() { return mCamera; }

        CompositorShadowNode *getShadowNode() const { return mShadowNode; }

        void _setUpdateShadowNode( bool update ) { mUpdateShadowNode = update; }

        const CompositorPassWarmUpDef *getDefinition() const { return mDefinition; }
    };

    class _OgreExport WarmUpHelper
    {
        static size_t calculateNumTargetPasses( const CompositorNodeDef *refNode );

        static size_t calculateNumScenePasses( const CompositorTargetDef *baseTargetDef );

        static void createFromRtv( CompositorNodeDef *warmUpNodeDef, const CompositorNodeDef *refNode,
                                   const IdString textureName, std::set<IdString> &seenTextures );

        static void createFromRtv( CompositorNodeDef *warmUpNodeDef, const CompositorNodeDef *refNode,
                                   const RenderTargetViewEntry &rtvEntry,
                                   std::set<IdString>          &seenTextures );

    public:
        /** Utility to programmatically create a node that contains only warm_up passes
            by basing them off the render_pass passes from a reference node.
        @param compositorManager
        @param nodeDefinitionName
            Name to give to the node definition containing the warm ups.
            Must be unique and not exist already.
        @param refNodeDefinitionName
            Name of the reference node definition to analyze.
        @param bCopyAllInputChannels
            False to only generate input channels that are actually needed.
            True to generate all input channels. This is useful if you must enforce certain order.
        */
        static void createFrom( CompositorManager2 *compositorManager, const String &nodeDefinitionName,
                                const IdString refNodeDefinitionName, const bool bCopyAllInputChannels );

        /** @} */
        /** @} */
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
