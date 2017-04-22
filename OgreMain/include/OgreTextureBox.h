/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2017 Torus Knot Software Ltd

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

#ifndef _OgreTextureBox_H_
#define _OgreTextureBox_H_

#include "OgrePrerequisites.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /// For cubemaps, the face is in sliceStart, (see CubemapSide::CubemapSide)
    /// For cubemap arrays, the cubemaps are addressed as sliceStart * 6.
    struct TextureBox
    {
        uint32 x, y, z, sliceStart;
        uint32 width, height, depth, numSlices;
        size_t bytesPerRow;
        size_t bytesPerImage;
        /// Pointer is never owned by us. Do not alter where
        /// data points to (e.g. do not increment it)
        void *data;

        TextureBox() :
            x( 0 ), y( 0 ), z( 0 ), sliceStart( 0 ),
            width( 0 ), height( 0 ), depth( 0 ), numSlices( 0 ),
            bytesPerRow( 0 ), bytesPerImage( 0 ),
            data( 0 )
        {
        }

        TextureBox( uint32 _width, uint32 _height, uint32 _depth, uint32 _numSlices ) :
            x( 0 ), y( 0 ), z( 0 ), sliceStart( 0 ),
            width( _width ), height( _height ), depth( _depth ), numSlices( _numSlices ),
            bytesPerRow( 0 ), bytesPerImage( 0 ),
            data( 0 )
        {
        }

        uint32 getMaxX(void) const      { return x + width; }
        uint32 getMaxY(void) const      { return y + height; }
        uint32 getMaxZ(void) const      { return z + depth; }
        uint32 getMaxSlice(void) const  { return sliceStart + numSlices; }
        uint32 getDepthOrSlices(void) const { return std::max( depth, numSlices ); }

        /// Returns true if 'other' & 'this' have the same dimensions.
        bool equalSize( const TextureBox &other ) const
        {
            return  this->width  == other.width &&
                    this->height == other.height &&
                    this->depth  == other.depth &&
                    this->numSlices == other.numSlices;
        }

        /// Returns true if 'other' fits inside 'this' (fully, not partially)
        bool fullyContains( const TextureBox &other ) const
        {
            return other.x >= this->x && other.getMaxX() <= this->getMaxX() &&
                   other.y >= this->y && other.getMaxY() <= this->getMaxY() &&
                   other.z >= this->z && other.getMaxZ() <= this->getMaxZ() &&
                   other.sliceStart >= this->sliceStart && other.getMaxSlice() <= this->getMaxSlice();
        }

        /// Returns true if 'this' and 'other' are in partial or full collision.
        bool overlaps( const TextureBox &other ) const
        {
            return!(other.x >= this->getMaxX() ||
                    other.y >= this->getMaxY() ||
                    other.z >= this->getMaxZ() ||
                    other.sliceStart >= this->getMaxSlice() ||
                    other.getMaxX() <= this->x ||
                    other.getMaxY() <= this->y ||
                    other.getMaxZ() <= this->z ||
                    other.getMaxSlice() <= this->sliceStart);
        }
    };
}

#include "OgreHeaderSuffix.h"

#endif
