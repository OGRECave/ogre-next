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

#ifndef _OgreCompositorPassShadows_H_
#define _OgreCompositorPassShadows_H_

#include "Compositor/Pass/OgreCompositorPass.h"

#include "Compositor/OgreCompositorCommon.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class CompositorPassShadowsDef;

    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Effects
     *  @{
     */

    /** @brief CompositorPassShadows
        This class enables force-updating multiple shadow nodes in batch in its own pass

        This is useful because shadow nodes may "break" a render pass in 3:

            - Normal rendering to RT
            - Shadow node update
            - Continue Normal rendering to the same RT

        This is an unnecessary performance hit on mobile (TBDR) thus executing them
        earlier allows for a smooth:

            - Shadow node update (all of them? Up to you)
            - Normal rendering to RT

        Don't forget to set shadow nodes to reuse in the pass scene passes or
        else you may overwrite them unnecessarily
    @author
        Matias N. Goldberg
    @version
        1.0
    */
    class _OgreExport CompositorPassShadows : public CompositorPass
    {
        CompositorPassShadowsDef const *mDefinition;

    protected:
        FastArray<CompositorShadowNode *> mShadowNodes;

        Camera *mCamera;
        Camera *mLodCamera;
        Camera *mCullCamera;

    public:
        CompositorPassShadows( const CompositorPassShadowsDef *definition, Camera *defaultCamera,
                               CompositorNode *parentNode, const RenderTargetViewDef *rtv );
        ~CompositorPassShadows() override;

        void execute( const Camera *lodCamera ) override;

        Camera *getCullCamera() const { return mCullCamera; }

        const FastArray<CompositorShadowNode *> &getShadowNodes() const { return mShadowNodes; }
    };

    /** @} */
    /** @} */
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
