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
#include "OgrePixelFormatGpuUtils.h"
#include "OgreColourValue.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /// For cubemaps, the face is in sliceStart, (see CubemapSide::CubemapSide)
    /// For cubemap arrays, the cubemaps are addressed as sliceStart * 6.
    struct _OgreExport TextureBox
    {
        uint32 x, y, z, sliceStart;
        uint32 width, height, depth, numSlices;
        /// When TextureBox contains a compressed format, bytesPerPixel contains
        /// the pixel format instead. See getCompressedPixelFormat.
        size_t bytesPerPixel;
        size_t bytesPerRow;
        size_t bytesPerImage;
        /// Pointer is never owned by us. Do not alter where
        /// data points to (e.g. do not increment it)
        void *data;

        TextureBox() :
            x( 0 ), y( 0 ), z( 0 ), sliceStart( 0 ),
            width( 0 ), height( 0 ), depth( 0 ), numSlices( 0 ),
            bytesPerPixel( 0 ), bytesPerRow( 0 ), bytesPerImage( 0 ),
            data( 0 )
        {
        }

        TextureBox( uint32 _width, uint32 _height, uint32 _depth, uint32 _numSlices,
                    uint32 _bytesPerPixel, uint32 _bytesPerRow, uint32 _bytesPerImage ) :
            x( 0 ), y( 0 ), z( 0 ), sliceStart( 0 ),
            width( _width ), height( _height ), depth( _depth ), numSlices( _numSlices ),
            bytesPerPixel( _bytesPerPixel ),
            bytesPerRow( _bytesPerRow ),
            bytesPerImage( _bytesPerImage ),
            data( 0 )
        {
        }

        uint32 getMaxX(void) const      { return x + width; }
        uint32 getMaxY(void) const      { return y + height; }
        uint32 getMaxZ(void) const      { return z + depth; }
        uint32 getMaxSlice(void) const  { return sliceStart + numSlices; }
        uint32 getDepthOrSlices(void) const { return std::max( depth, numSlices ); }
        uint32 getZOrSlice(void) const  { return std::max( z, sliceStart ); }

        size_t getSizeBytes(void) const { return bytesPerImage * std::max( depth, numSlices ); }

        void setCompressedPixelFormat( PixelFormatGpu pixelFormat )
        {
            assert( PixelFormatGpuUtils::isCompressed( pixelFormat ) );
            bytesPerPixel = 0xF0000000 + pixelFormat;
        }
        PixelFormatGpu getCompressedPixelFormat(void) const
        {
            if( bytesPerPixel < 0xF0000000 )
                return PFG_UNKNOWN;
            return static_cast<PixelFormatGpu>( bytesPerPixel - 0xF0000000 );
        }

        bool isCompressed(void) const
        {
            return bytesPerPixel >= 0xF0000000;
        }

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

        /// x, y & z are in pixels. Only works for non-compressed formats.
        /// It can work for compressed formats if xPos & yPos are 0.
        void* at( size_t xPos, size_t yPos, size_t zPos ) const
        {
            if( !isCompressed() )
            {
                return reinterpret_cast<uint8*>( data ) +
                        zPos * bytesPerImage + yPos * bytesPerRow + xPos * bytesPerPixel;
            }
            else
            {
                const PixelFormatGpu pixelFormat = getCompressedPixelFormat();
                const size_t blockSize = PixelFormatGpuUtils::getCompressedBlockSize( pixelFormat );
                const uint32 blockWidth = PixelFormatGpuUtils::getCompressedBlockWidth( pixelFormat,
                                                                                        false );
                const uint32 blockHeight= PixelFormatGpuUtils::getCompressedBlockHeight( pixelFormat,
                                                                                         false );
                const size_t yBlock = yPos / blockHeight;
                const size_t xBlock = xPos / blockWidth;
                return reinterpret_cast<uint8*>( data ) +
                        zPos * bytesPerImage + yBlock * bytesPerRow + xBlock * blockSize;
            }

        }

        void* atFromOffsettedOrigin( size_t xPos, size_t yPos, size_t zPos ) const
        {
            return at( xPos + x, yPos + y, zPos + getZOrSlice() );
        }

        void copyFrom( const TextureBox &src )
        {
            assert( this->width  == src.width &&
                    this->height == src.height &&
                    this->getDepthOrSlices() >= src.getDepthOrSlices() );

            const uint32 finalDepthOrSlices = src.getDepthOrSlices();
            const uint32 srcZorSlice = src.getZOrSlice();
            const uint32 dstZorSlice = this->getZOrSlice();

            if( this->x == 0 && src.x == 0 &&
                this->y == 0 && src.y == 0 &&
                this->bytesPerRow == src.bytesPerRow &&
                this->bytesPerImage == src.bytesPerImage )
            {
                //Raw copy
                const void *srcData = src.at( 0, 0, srcZorSlice );
                void *dstData       = this->at( 0, 0, dstZorSlice );
                memcpy( dstData, srcData, bytesPerImage * finalDepthOrSlices );
            }
            else
            {
                if( !isCompressed() )
                {
                    //Copy row by row, uncompressed.
                    const uint32 finalHeight        = this->height;
                    const size_t finalBytesPerRow   = std::min( this->bytesPerRow, src.bytesPerRow );
                    for( size_t _z=0; _z<finalDepthOrSlices; ++_z )
                    {
                        for( size_t _y=0; _y<finalHeight; ++_y )
                        {
                            const void *srcData = src.at( src.x,   _y + src.y,   _z + srcZorSlice );
                            void *dstData       = this->at( this->x, _y + this->y, _z + dstZorSlice );
                            memcpy( dstData, srcData, finalBytesPerRow );
                        }
                    }
                }
                else
                {
                    //Copy row of blocks by row of blocks, compressed.
                    const PixelFormatGpu pixelFormat = getCompressedPixelFormat();
                    const uint32 blockHeight= PixelFormatGpuUtils::getCompressedBlockHeight( pixelFormat,
                                                                                             false );
                    const uint32 finalHeight        = this->height;
                    const size_t finalBytesPerRow   = std::min( this->bytesPerRow, src.bytesPerRow );
                    for( size_t _z=0; _z<finalDepthOrSlices; ++_z )
                    {
                        for( size_t _y=0; _y<finalHeight; _y += blockHeight )
                        {
                            const void *srcData = src.at( src.x,   _y + src.y,   _z + srcZorSlice );
                            void *dstData       = this->at( this->x, _y + this->y, _z + dstZorSlice );
                            memcpy( dstData, srcData, finalBytesPerRow );
                        }
                    }
                }
            }
        }

        void copyFrom( void *srcData, uint32 _width, uint32 _height, uint32 _bytesPerRow )
        {
            TextureBox box( _width, _height, 1u, 1u, 0, _bytesPerRow, _bytesPerRow * _height );
            box.data = srcData;
            copyFrom( box );
        }

        /// Get colour value from a certain location in the image.
        ColourValue getColourAt( size_t _x, size_t _y, size_t _z, PixelFormatGpu pixelFormat ) const;

        /// Set colour value at a certain location in the image.
        void setColourAt( const ColourValue &cv, size_t _x, size_t _y, size_t _z,
                          PixelFormatGpu pixelFormat );
    };
}

#include "OgreHeaderSuffix.h"

#endif
