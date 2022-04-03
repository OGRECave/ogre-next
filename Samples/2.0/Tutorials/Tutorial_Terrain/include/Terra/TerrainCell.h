/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2021 Torus Knot Software Ltd

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

#ifndef _OgreTerrainCell_H_
#define _OgreTerrainCell_H_

#include "OgrePrerequisites.h"

#include "OgreRenderable.h"

namespace Ogre
{
    class Terra;
    struct GridPoint;

    class TerrainCell : public Renderable
    {
        int32  m_gridX;
        int32  m_gridZ;
        uint32 m_lodLevel;
        uint32 m_verticesPerLine;

        uint32 m_sizeX;
        uint32 m_sizeZ;

        VaoManager *m_vaoManager;

        Terra *m_parentTerra;

        bool m_useSkirts;

    public:
        TerrainCell( Terra *parentTerra );
        ~TerrainCell() override;

        bool getUseSkirts() const { return m_useSkirts; }

        bool isZUp() const;

        void initialize( VaoManager *vaoManager, bool useSkirts );

        void setOrigin( const GridPoint &gridPos, uint32 horizontalPixelDim, uint32 verticalPixelDim,
                        uint32 lodLevel );

        /** Merges another TerrainCell into 'this' for reducing batch counts.
            e.g.
                Two 32x32 cells will merge into one 64x32 or 32x64
                Two 64x32 cells will merge into one 64x64
                A 32x64 cell cannot merge with a 32x32 one.
                A 64x32 cell cannot merge with a 32x32 one.
        @remarks
            Merge will only happen if the cells are of the same LOD level and are contiguous.
        @param next
            The other TerrainCell to merge with.
        @return
            False if couldn't merge, true on success.
        */
        bool merge( TerrainCell *next );

        void uploadToGpu( uint32 *RESTRICT_ALIAS gpuPtr ) const;

        Terra *getParentTerra() const { return m_parentTerra; }

        // Renderable overloads
        const LightList &getLights() const override;
        void             getRenderOperation( v1::RenderOperation &op, bool casterPass ) override;
        void             getWorldTransforms( Matrix4 *xform ) const override;
        bool             getCastsShadows() const override;
    };
}  // namespace Ogre

#endif
