/*
-----------------------------------------------------------------------------
This source file is part of OGRE
(Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#ifndef _OgreRectangle2D2_H_
#define _OgreRectangle2D2_H_

#include "OgrePrerequisites.h"

#include "OgreMovableObject.h"
#include "OgreRenderable.h"
#include "Vao/OgreBufferPacked.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    class _OgreExport Rectangle2D : public MovableObject, public Renderable
    {
    public:
        enum GeometryFlags
        {
            // clang-format off
            /// Fullscreen quad instead of triangle
            GeometryFlagQuad        = 2u << 1u,
            /// For stereo
            GeometryFlagStereo      = 1u << 1u,
            GeometryFlagNormals     = 1u << 2u,
            GeometryFlagReserved0   = 1u << 3u | 1u << 4u | 1u << 5u,
            ///
            GeometryFlagHollowFsRect= 1 << 6u
            // clang-format on
        };

    protected:
        enum Corner
        {
            CornerBottomLeft,
            CornerUpperLeft,
            CornerBottomRight,
            CornerUpperRight,
            NumCorners
        };

        bool mChanged;
        uint32 mGeometryFlags;
        Vector3 mNormals[NumCorners];

        Vector2 mPosition;
        Vector2 mSize;

        void createBuffers( void );
        void fillHollowFsRect( float *RESTRICT_ALIAS vertexData, size_t maxElements );
        void fillBuffer( float *RESTRICT_ALIAS vertexData, size_t maxElements );

    public:
        Rectangle2D( IdType id, ObjectMemoryManager *objectMemoryManager, SceneManager *manager );
        virtual ~Rectangle2D();

        bool isQuad( void ) const;
        bool isStereo( void ) const;
        bool hasNormals( void ) const;
        BufferType getBufferType( void ) const;
        bool isHollowFullscreenRect( void ) const;

        uint32 calculateNumVertices( void ) const;

        void setGeometry( const Vector2 &pos, const Vector2 &size );

        void setNormals( const Vector3 &upperLeft, const Vector3 &bottomLeft,  //
                         const Vector3 &upperRight, const Vector3 &bottomRight );

        void setHollowRectRadius( Real radius );
        Real getHollowRectRadius( void ) const { return mNormals[0].x; }

        void initialize( BufferType bufferType, uint32 geometryFlags );

        void update( void );

        // Overrides from MovableObject
        virtual const String &getMovableType( void ) const;

        // Overrides from Renderable
        virtual const LightList &getLights( void ) const;
        virtual void getRenderOperation( v1::RenderOperation &op, bool casterPass );
        virtual void getWorldTransforms( Matrix4 *xform ) const;
        virtual bool getCastsShadows( void ) const;
    };

    /** Factory object for creating Entity instances */
    class _OgreExport Rectangle2DFactory : public MovableObjectFactory
    {
    protected:
        virtual MovableObject *createInstanceImpl( IdType id, ObjectMemoryManager *objectMemoryManager,
                                                   SceneManager *manager,
                                                   const NameValuePairList *params = 0 );

    public:
        Rectangle2DFactory() {}
        ~Rectangle2DFactory() {}

        static String FACTORY_TYPE_NAME;

        const String &getType( void ) const;
        void destroyInstance( MovableObject *obj );
    };
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
