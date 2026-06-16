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

#include "OgreImguiRenderable.h"

#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include <imgui.h>

using namespace Ogre;

ImguiRenderable::ImguiRenderable()
{
    // use identity projection and view matrices
    mUseIdentityProjection = true;
    mUseIdentityView = true;

    // By default we want ImguiRenderables to still work in wireframe mode
    mPolygonModeOverrideable = false;
}
//-----------------------------------------------------------------------------
ImguiRenderable::~ImguiRenderable()
{
}
//-----------------------------------------------------------------------------
size_t ImguiRenderable::getVertexCount() const
{
    if( mVaoPerLod[0].empty() )
        return 0u;
    return mVaoPerLod[0].front()->getBaseVertexBuffer()->getNumElements();
}
//-----------------------------------------------------------------------------
size_t ImguiRenderable::getIndexCount() const
{
    if( mVaoPerLod[0].empty() )
        return 0u;
    return mVaoPerLod[0].front()->getIndexBuffer()->getNumElements();
}
//-----------------------------------------------------------------------------
void ImguiRenderable::recreateBuffers( VaoManager *vaoManager, VertexBufferPacked *newVertexBuffer,
                                       IndexBufferPacked *newIndexBuffer )
{
    for( VertexArrayObject *vao : mVaoPerLod[0] )
    {
        const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();
        for( VertexBufferPacked *vertexBuffer : vertexBuffers )
        {
            if( newVertexBuffer )
            {
                if( vertexBuffer->getMappingState() != MS_UNMAPPED )
                    vertexBuffer->unmap( UO_UNMAP_ALL );
                vaoManager->destroyVertexBuffer( vertexBuffer );
            }
            else
            {
                newVertexBuffer = vertexBuffer;
            }
        }

        if( newIndexBuffer )
        {
            IndexBufferPacked *indexBuffer = vao->getIndexBuffer();
            if( indexBuffer )
            {
                if( indexBuffer->getMappingState() != MS_UNMAPPED )
                    indexBuffer->unmap( UO_UNMAP_ALL );
                vaoManager->destroyIndexBuffer( indexBuffer );
            }
        }
        else
        {
            newIndexBuffer = vao->getIndexBuffer();
        }
        vaoManager->destroyVertexArrayObject( vao );
    }

    mVaoPerLod[0].clear();
    mVaoPerLod[0].push_back(
        vaoManager->createVertexArrayObject( { newVertexBuffer }, newIndexBuffer, OT_TRIANGLE_LIST ) );
}
//-----------------------------------------------------------------------------
void ImguiRenderable::destroyBuffers( VaoManager *vaoManager )
{
    for( VertexArrayObject *vao : mVaoPerLod[0] )
    {
        const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();
        for( VertexBufferPacked *vertexBuffer : vertexBuffers )
        {
            if( vertexBuffer->getMappingState() != MS_UNMAPPED )
                vertexBuffer->unmap( UO_UNMAP_ALL );
            vaoManager->destroyVertexBuffer( vertexBuffer );
        }

        IndexBufferPacked *indexBuffer = vao->getIndexBuffer();
        if( indexBuffer )
        {
            if( indexBuffer->getMappingState() != MS_UNMAPPED )
                indexBuffer->unmap( UO_UNMAP_ALL );
            vaoManager->destroyIndexBuffer( indexBuffer );
        }
        vaoManager->destroyVertexArrayObject( vao );
    }
}
//-----------------------------------------------------------------------------
void ImguiRenderable::updateVertexData( const ImDrawVert *vtxBuf, const ImDrawIdx *idxBuf,
                                        unsigned int vtxCount, unsigned int idxCount,
                                        VaoManager *vaoManager )
{
    VertexBufferPacked *vertexBuffer = 0;
    IndexBufferPacked *indexBuffer = 0;
    if( vtxCount > getVertexCount() )
    {
        VertexElement2Vec vertexElements;
        vertexElements.push_back( VertexElement2( VET_FLOAT2, VES_POSITION ) );
        vertexElements.push_back( VertexElement2( VET_FLOAT2, VES_TEXTURE_COORDINATES ) );
        vertexElements.push_back( VertexElement2( VET_COLOUR, VES_DIFFUSE ) );

        vertexBuffer = vaoManager->createVertexBuffer( vertexElements, size_t( vtxCount ),
                                                       BT_DYNAMIC_PERSISTENT, nullptr, false );
    }
    if( idxCount > getIndexCount() )
    {
        indexBuffer = vaoManager->createIndexBuffer( IndexBufferPacked::IT_16BIT, size_t( idxCount ),
                                                     BT_DYNAMIC_PERSISTENT, nullptr, false );
    }

    if( vertexBuffer || indexBuffer )
        recreateBuffers( vaoManager, vertexBuffer, indexBuffer );

    VertexArrayObject *vao = mVaoPerLod[0][0];
    vao->setPrimitiveRange( 0u, idxCount );

    void *vtxDst = vao->getBaseVertexBuffer()->map( 0u, vtxCount );
    void *idxDst = vao->getIndexBuffer()->map( 0u, idxCount );

    // Copy all vertices
    memcpy( vtxDst, vtxBuf, vtxCount * sizeof( ImDrawVert ) );
    memcpy( idxDst, idxBuf, idxCount * sizeof( ImDrawIdx ) );

    vao->getBaseVertexBuffer()->unmap( UO_KEEP_PERSISTENT, 0u, vtxCount );
    vao->getIndexBuffer()->unmap( UO_KEEP_PERSISTENT, 0u, idxCount );
}
//-----------------------------------------------------------------------------
void ImguiRenderable::getWorldTransforms( Matrix4 *xform ) const
{
    OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                 "ImguiRenderable do not implement getWorldTransforms."
                 " You've put a v2 object in "
                 "the wrong RenderQueue ID (which is set to be compatible with "
                 "v1::Entity). Do not mix v2 and v1 objects",
                 "ImguiRenderable::getWorldTransforms" );
}
//-----------------------------------------------------------------------------
void ImguiRenderable::getRenderOperation( v1::RenderOperation &op, bool casterPass )
{
    OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                 "ImguiRenderable do not implement getRenderOperation."
                 " You've put a v2 object in "
                 "the wrong RenderQueue ID (which is set to be compatible with "
                 "v1::Entity). Do not mix v2 and v1 objects",
                 "ImguiRenderable::getRenderOperation" );
}
//-----------------------------------------------------------------------------
const LightList &ImguiRenderable::getLights( void ) const
{
    static const LightList l;
    return l;
}
