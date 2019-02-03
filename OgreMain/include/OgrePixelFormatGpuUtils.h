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
    class _OgreExport PixelFormatGpuUtils
    {
    public:
        enum PixelFormatFlags
        {
            /// Pixel Format is an actual float (32-bit float)
            PFF_FLOAT       = 1u << 0u,
            /// Pixel Format is 16-bit float
            PFF_HALF        = 1u << 1u,
            /// Pixel Format is float, but is neither 32-bit nor 16-bit
            /// (some weird one, could have shared exponent or not)
            PFF_FLOAT_RARE  = 1u << 2u,
            /// Pixel Format is (signed/unsigned) integer. May be
            /// normalized if PFF_NORMALIZED is present.
            PFF_INTEGER     = 1u << 3u,
            /// Pixel Format is either in range [-1; 1] or [0; 1]
            PFF_NORMALIZED  = 1u << 4u,
            /// Pixel Format can only be positive or negative.
            /// It's implicit for float/half formats. Lack
            /// of this flag means it's unsigned.
            PFF_SIGNED      = 1u << 5u,
            /// This is a depth format. Can be combined with
            /// PFF_FLOAT/PFF_HALF/PFF_INTEGER/PFF_NORMALIZED to get
            /// more info about the depth.
            PFF_DEPTH       = 1u << 6u,
            /// This format has stencil.
            PFF_STENCIL     = 1u << 7u,
            /// Format is in sRGB space.
            PFF_SRGB        = 1u << 8u,
            /// Format is compressed
            PFF_COMPRESSED  = 1u << 9u,
            /// Format is palletized
            PFF_PALLETE     = 1u << 10u
        };

    protected:
        struct PixelFormatDesc
        {
            const char  *name;
            uint32      components;
            uint32      bytesPerPixel;
            uint32      flags;
        };

        static PixelFormatDesc msPixelFormatDesc[PFG_COUNT+1u];

        static inline const PixelFormatDesc& getDescriptionFor( const PixelFormatGpu fmt );

        template <typename T>
        static void convertFromFloat( const float *rgbaPtr, void *dstPtr,
                                      size_t numComponents, uint32 flags );
        template <typename T>
        static void convertToFloat( float *rgbaPtr, const void *srcPtr,
                                    size_t numComponents, uint32 flags );

    public:
        static size_t getBytesPerPixel( PixelFormatGpu format );
        static size_t getNumberOfComponents( PixelFormatGpu format );

        static size_t getSizeBytes( uint32 width, uint32 height, uint32 depth,
                                    uint32 slices, PixelFormatGpu format,
                                    uint32 rowAlignment=1u );

        static size_t calculateSizeBytes( uint32 width, uint32 height, uint32 depth,
                                          uint32 slices, PixelFormatGpu format,
                                          uint8 numMipmaps, uint32 rowAlignment=1u );

        /** Returns the maximum number of mipmaps given the resolution
            e.g. at 4x4 there's 3 mipmaps. At 1x1 there's 1 mipmaps.
        @note
            Can return 0 if maxResolution = 0.
        @return
            Mip count.
        */
        static uint8 getMaxMipmapCount( uint32 maxResolution );
        static uint8 getMaxMipmapCount( uint32 width, uint32 height );
        static uint8 getMaxMipmapCount( uint32 width, uint32 height, uint32 depth );

        /// For SW mipmaps, see Image2::supportsSwMipmaps
        static bool supportsHwMipmaps( PixelFormatGpu format );

        /** Returns the minimum width for block compressed schemes. ie. DXT1 compresses in blocks
            of 4x4 pixels. A texture with a width of 2 is just padded to 4.
            When building UV atlases composed of already compressed data being stitched together,
            the block size is very important to know as the resolution of the individual textures
            must be a multiple of this size.
         @remarks
            If the format is not compressed, returns 1.
         @par
            The function can return a value of 0 (as happens with PVRTC & ETC1 compression); this is
            because although they may compress in blocks (i.e. PVRTC uses a 4x4 or 8x4 block), this
            information is useless as the compression scheme doesn't have isolated blocks (modifying
            a single pixel can change the binary data of the entire stream) making it useless for
            subimage sampling or creating UV atlas.
         @param format
            The format to query for. Can be compressed or not.
         @param apiStrict
            When true, obeys the rules of most APIs (i.e. ETC1 can't update subregions according to
            GLES specs). When false, becomes more practical if manipulating by hand (i.e. ETC1's
            subregions can be updated just fine by @bulkCompressedSubregion)
         @return
            The width of compression block, in pixels. Can be 0 (see remarks). If format is not
            compressed, returns 1.
        */
        static uint32 getCompressedBlockWidth( PixelFormatGpu format, bool apiStrict=true );

        /// See getCompressedBlockWidth
        static uint32 getCompressedBlockHeight( PixelFormatGpu format, bool apiStrict=true );

        /// Returns in bytes, the size of the compressed block
        static size_t getCompressedBlockSize( PixelFormatGpu format );

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

        /// Takes an image allocated for GPU usage (i.e. rowAlignment = 4u) from the beginning of
        /// its base mip level 0, and returns a pointer at the beginning of the specified mipLevel.
        static void* advancePointerToMip( void *basePtr, uint32 width, uint32 height,
                                          uint32 depth, uint32 numSlices, uint8 mipLevel,
                                          PixelFormatGpu format );

        static float toSRGB( float x );
        static float fromSRGB( float x );

        static void packColour( const float *rgbaPtr, PixelFormatGpu pf, void *dstPtr );
        static void unpackColour( float *rgbaPtr, PixelFormatGpu pf, const void *srcPtr );
        static void packColour( const ColourValue &rgbaPtr, PixelFormatGpu pf, void *dstPtr );
        static void unpackColour( ColourValue *rgbaPtr, PixelFormatGpu pf, const void *srcPtr );

        static void convertForNormalMapping( TextureBox src, PixelFormatGpu srcFormat,
                                             TextureBox dst, PixelFormatGpu dstFormat );
        static void bulkPixelConversion( const TextureBox &src, PixelFormatGpu srcFormat,
                                         TextureBox &dst, PixelFormatGpu dstFormat );

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

        static bool hasSRGBEquivalent( PixelFormatGpu format );
        static PixelFormatGpu getEquivalentSRGB( PixelFormatGpu format );
        static PixelFormatGpu getEquivalentLinear( PixelFormatGpu sRgbFormat );

        static PixelFormatGpu getFamily( PixelFormatGpu format );
    };

    /** @} */
    /** @} */
}

#include "OgreHeaderSuffix.h"

#endif
