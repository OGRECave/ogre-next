/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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
#ifndef _OgrePixelFormatGpuUtils_H_
#define _OgrePixelFormatGpuUtils_H_

#include "OgrePixelFormatGpu.h"

#include "OgreHeaderPrefix.h"

namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Image
    *  @{
    */
    /** The pixel format used for images, textures, and render surfaces */
    class PixelFormatGpuUtils
    {
    public:
        enum PixelFormatFlags
        {
            /// Pixel Format is an actual float (32-bit float)
            PFF_FLOAT,
            /// Pixel Format is 16-bit float
            PFF_HALF,
            /// Pixel Format is float, but is neither 32-bit nor 16-bit
            /// (some weird one, could have shared exponent or not)
            PFF_FLOAT_RARE,
            /// Pixel Format is (signed/unsigned) integer. May be
            /// normalized if PFF_NORMALIZED is present.
            PFF_INTEGER,
            /// Pixel Format is either in range [-1; 1] or [0; 1]
            PFF_NORMALIZED,
            /// Pixel Format can only be positive or negative.
            /// It's implicit for float/half formats. Lack
            /// of this flag means it's unsigned.
            PFF_SIGNED,
            /// This is a depth format. Can be combined with
            /// PFF_FLOAT/PFF_HALF/PFF_INTEGER/PFF_NORMALIZED to get
            /// more info about the depth.
            PFF_DEPTH,
            /// This format has stencil.
            PFF_STENCIL,
            /// Format is in sRGB space.
            PFF_SRGB,
            /// Format is compressed
            PFF_COMPRESSED,
            /// Format is palletized
            PFF_PALLETE
        };

    protected:
        struct PixelFormatDesc
        {
            const char  *name;
            size_t      bytesPerPixel;
            uint32      flags;
        };

        static PixelFormatDesc msPixelFormatDesc[PFG_COUNT+1u];

        static inline const PixelFormatDesc& getDescriptionFor( const PixelFormatGpu fmt );

    public:
        static size_t getBytesPerPixel( PixelFormatGpu format );

        static size_t getSizeBytes( uint32 width, uint32 height, uint32 depth,
                                    uint32 slices, PixelFormatGpu format,
                                    uint32 rowAlignment=1u );

        static const char* toString( PixelFormatGpu format );

        /** Makes a O(N) search to return the PixelFormatGpu based on its string version.
            Opposite version of toString
        @param name
            Name of the pixel format. e.g. PFG_RGBA8_UNORM_SRGB
        @param exclusionFlags
            Use PixelFormatFlags to exclude certain formats. For example if you don't want
            compressed and depth formats to be returned, pass PFF_COMPRESSED|PFF_DEPTH
        @return
            The format you're looking for, PFG_UNKNOWN if not found.
        */
        static PixelFormatGpu getFormatFromName( const char *name, uint32 exclusionFlags=0 );
        static PixelFormatGpu getFormatFromName( const String &name, uint32 exclusionFlags=0 );

        /// See PixelFormatFlags
        static uint32 getFlags( PixelFormatGpu format );
        static bool isFloat( PixelFormatGpu format );
        static bool isHalf( PixelFormatGpu format );
        static bool isFloatRare( PixelFormatGpu format );
        static bool isInteger( PixelFormatGpu format );
        static bool isNormalized( PixelFormatGpu format );
        static bool isSigned( PixelFormatGpu format );
        static bool isDepth( PixelFormatGpu format );
        static bool isStencil( PixelFormatGpu format );
        static bool isSRgb( PixelFormatGpu format );
        static bool isCompressed( PixelFormatGpu format );
        static bool isPallete( PixelFormatGpu format );
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
