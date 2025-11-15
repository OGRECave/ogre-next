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

#ifndef _OgreImguiRenderable_H_
#define _OgreImguiRenderable_H_

#include <OgreRenderOperation.h>
#include <OgreRenderable.h>

struct ImDrawList;
typedef uint16_t ImDrawIdx;
struct ImDrawVert;

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    OGRE_ASSUME_NONNULL_BEGIN

    class ImguiRenderable final : public Renderable
    {
        void recreateBuffers( VaoManager *vaoManager, VertexBufferPacked *vertexBuffer,
                              IndexBufferPacked *indexBuffer );

        size_t getVertexCount() const;
        size_t getIndexCount() const;

    public:
        ImguiRenderable();
        ~ImguiRenderable() override;

        void destroyBuffers( VaoManager *vaoManager );

        // builds the vertex buffer
        void updateVertexData( const ImDrawVert *vtxBuf, const ImDrawIdx *idxBuf, unsigned int vtxCount,
                               unsigned int idxCount, VaoManager *vaoManager );

        // Overrides from Renderable
        void getWorldTransforms( Matrix4 *xform ) const override;
        void getRenderOperation( v1::RenderOperation &op, bool casterPass ) override;

        const LightList &getLights( void ) const override;
    };

    OGRE_ASSUME_NONNULL_END
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
