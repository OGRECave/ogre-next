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
#ifndef OGREIMAGERESAMPLER_H
#define OGREIMAGERESAMPLER_H

#include <algorithm>

// this file is inlined into OgreImage.cpp!
// do not include anywhere else.
namespace Ogre {
    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Image
    *  @{
    */

// variable name hints:
// sx_48 = 16/48-bit fixed-point x-position in source
// stepx = difference between adjacent sx_48 values
// sx1 = lower-bound integer x-position in source
// sx2 = upper-bound integer x-position in source
// sxf = fractional weight between sx1 and sx2
// x,y,z = location of output pixel in destination

// nearest-neighbor resampler, does not convert formats.
// templated on bytes-per-pixel to allow compiler optimizations, such
// as simplifying memcpy() and replacing multiplies with bitshifts
template<unsigned int elemsize>
struct NearestResampler
{
    static void scale(const TextureBox& src, const TextureBox& dst)
    {
        // srcdata stays at beginning, pdst is a moving pointer
        uchar* srcdata = (uchar*)src.atFromOffsettedOrigin( 0, 0, 0 );
        uchar* dstdata = (uchar*)dst.atFromOffsettedOrigin( 0, 0, 0 );

        // sx_48,sy_48,sz_48 represent current position in source
        // using 16/48-bit fixed precision, incremented by steps
        uint64 stepx = ((uint64)src.width << 48) / dst.width;
        uint64 stepy = ((uint64)src.height << 48) / dst.height;
        uint64 stepz = ((uint64)src.depth << 48) / dst.depth;

        // note: ((stepz>>1) - 1) is an extra half-step increment to adjust
        // for the center of the destination pixel, not the top-left corner
        uint64 sz_48 = (stepz >> 1) - 1;
        for (size_t z = 0; z < dst.getDepthOrSlices(); z++, sz_48 += stepz) {
            size_t srczoff = (size_t)(sz_48 >> 48) * src.bytesPerImage;
            
            uint64 sy_48 = (stepy >> 1) - 1;
            for (size_t y = 0; y < dst.height; y++, sy_48 += stepy) {
                size_t srcyoff = (size_t)(sy_48 >> 48) * src.bytesPerRow;
            
                uchar *psrc = srcdata + srcyoff + srczoff;
                uchar *pdst = dstdata + y * dst.bytesPerRow + z * dst.bytesPerImage;

                uint64 sx_48 = (stepx >> 1) - 1;
                for (size_t x = 0; x < dst.width; x++, sx_48 += stepx) {
                    memcpy( pdst, psrc + elemsize * ( size_t )( sx_48 >> 48 ), elemsize );
                    pdst += elemsize;
                }
            }
        }
    }
};


// default floating-point linear resampler, does format conversion
struct LinearResampler
{
    static void scale( const TextureBox& src, PixelFormatGpu srcFormat,
                       const TextureBox& dst, PixelFormatGpu dstFormat )
    {
        size_t srcelemsize = PixelFormatGpuUtils::getBytesPerPixel( srcFormat );
        size_t dstelemsize = PixelFormatGpuUtils::getBytesPerPixel( dstFormat );

        // srcdata stays at beginning, pdst is a moving pointer
        uchar* srcdata = (uchar*)src.atFromOffsettedOrigin( 0, 0, 0 );
        uchar* dstdata = (uchar*)dst.atFromOffsettedOrigin( 0, 0, 0 );
        
        // sx_48,sy_48,sz_48 represent current position in source
        // using 16/48-bit fixed precision, incremented by steps
        uint64 stepx = ((uint64)src.width << 48) / dst.width;
        uint64 stepy = ((uint64)src.height << 48) / dst.height;
        uint64 stepz = ((uint64)src.depth << 48) / dst.depth;
        
        // note: ((stepz>>1) - 1) is an extra half-step increment to adjust
        // for the center of the destination pixel, not the top-left corner
        uint64 sz_48 = (stepz >> 1) - 1;
        for (size_t z = 0; z < dst.getDepthOrSlices(); z++, sz_48+=stepz) {
            // temp is 16/16 bit fixed precision, used to adjust a source
            // coordinate (x, y, or z) backwards by half a pixel so that the
            // integer bits represent the first sample (eg, sx1) and the
            // fractional bits are the blend weight of the second sample
            unsigned int temp = static_cast<unsigned int>(sz_48 >> 32);

            temp = (temp > 0x8000)? temp - 0x8000 : 0;
            uint32 sz1 = temp >> 16;                 // src z, sample #1
            uint32 sz2 = std::min(sz1+1,src.depth-1);// src z, sample #2
            float szf = (temp & 0xFFFF) / 65536.f; // weight of sample #2

            uint64 sy_48 = (stepy >> 1) - 1;
            for (size_t y = 0; y < dst.height; y++, sy_48+=stepy) {
                temp = static_cast<unsigned int>(sy_48 >> 32);
                temp = (temp > 0x8000)? temp - 0x8000 : 0;
                uint32 sy1 = temp >> 16;                    // src y #1
                uint32 sy2 = std::min(sy1+1,src.height-1);// src y #2
                float syf = (temp & 0xFFFF) / 65536.f; // weight of #2

                uchar *pdst = dstdata + y * dst.bytesPerRow + z * dst.bytesPerImage;

                uint64 sx_48 = (stepx >> 1) - 1;
                for (size_t x = 0; x < dst.width; x++, sx_48+=stepx) {
                    temp = static_cast<unsigned int>(sx_48 >> 32);
                    temp = (temp > 0x8000)? temp - 0x8000 : 0;
                    uint32 sx1 = temp >> 16;                    // src x #1
                    uint32 sx2 = std::min(sx1+1,src.width-1);// src x #2
                    float sxf = (temp & 0xFFFF) / 65536.f; // weight of #2
                
                    ColourValue x1y1z1, x2y1z1, x1y2z1, x2y2z1;
                    ColourValue x1y1z2, x2y1z2, x1y2z2, x2y2z2;

#define UNPACK(dst,x,y,z) PixelFormatGpuUtils::unpackColour(&dst, srcFormat, \
    srcdata + srcelemsize*(x)+(y)*src.bytesPerRow+(z)*src.bytesPerImage)

                    UNPACK(x1y1z1,sx1,sy1,sz1); UNPACK(x2y1z1,sx2,sy1,sz1);
                    UNPACK(x1y2z1,sx1,sy2,sz1); UNPACK(x2y2z1,sx2,sy2,sz1);
                    UNPACK(x1y1z2,sx1,sy1,sz2); UNPACK(x2y1z2,sx2,sy1,sz2);
                    UNPACK(x1y2z2,sx1,sy2,sz2); UNPACK(x2y2z2,sx2,sy2,sz2);
#undef UNPACK

                    ColourValue accum =
                        x1y1z1 * ((1.0f - sxf)*(1.0f - syf)*(1.0f - szf)) +
                        x2y1z1 * (        sxf *(1.0f - syf)*(1.0f - szf)) +
                        x1y2z1 * ((1.0f - sxf)*        syf *(1.0f - szf)) +
                        x2y2z1 * (        sxf *        syf *(1.0f - szf)) +
                        x1y1z2 * ((1.0f - sxf)*(1.0f - syf)*        szf ) +
                        x2y1z2 * (        sxf *(1.0f - syf)*        szf ) +
                        x1y2z2 * ((1.0f - sxf)*        syf *        szf ) +
                        x2y2z2 * (        sxf *        syf *        szf );

                    PixelFormatGpuUtils::packColour( accum, dstFormat, pdst );

                    pdst += dstelemsize;
                }
            }
        }
    }
};


// float32 linear resampler, converts FLOAT32_RGB/FLOAT32_RGBA only.
// avoids overhead of pixel unpack/repack function calls
struct LinearResampler_Float32
{
    static void scale( const TextureBox &src, PixelFormatGpu srcFormat,
                       const TextureBox &dst, PixelFormatGpu dstFormat )
    {
        size_t srcelemsize = PixelFormatGpuUtils::getBytesPerPixel( srcFormat );
        size_t dstelemsize = PixelFormatGpuUtils::getBytesPerPixel( dstFormat );
        size_t minchannels = std::min( srcelemsize, dstelemsize ) / sizeof( float );

        // srcdata stays at beginning, pdst is a moving pointer
        uchar* srcdata = (uchar*)src.atFromOffsettedOrigin( 0, 0, 0 );
        uchar* dstdata = (uchar*)dst.atFromOffsettedOrigin( 0, 0, 0 );
        
        // sx_48,sy_48,sz_48 represent current position in source
        // using 16/48-bit fixed precision, incremented by steps
        uint64 stepx = ((uint64)src.width << 48) / dst.width;
        uint64 stepy = ((uint64)src.height << 48) / dst.height;
        uint64 stepz = ((uint64)src.depth << 48) / dst.depth;
        
        // note: ((stepz>>1) - 1) is an extra half-step increment to adjust
        // for the center of the destination pixel, not the top-left corner
        uint64 sz_48 = (stepz >> 1) - 1;
        for (size_t z = 0; z < dst.getDepthOrSlices(); z++, sz_48+=stepz) {
            // temp is 16/16 bit fixed precision, used to adjust a source
            // coordinate (x, y, or z) backwards by half a pixel so that the
            // integer bits represent the first sample (eg, sx1) and the
            // fractional bits are the blend weight of the second sample
            unsigned int temp = static_cast<unsigned int>(sz_48 >> 32);

            temp = (temp > 0x8000)? temp - 0x8000 : 0;
            uint32 sz1 = temp >> 16;                 // src z, sample #1
            uint32 sz2 = std::min(sz1+1,src.depth-1);// src z, sample #2
            float szf = (temp & 0xFFFF) / 65536.f; // weight of sample #2

            uint64 sy_48 = (stepy >> 1) - 1;
            for (size_t y = 0; y < dst.height; y++, sy_48+=stepy) {
                temp = static_cast<unsigned int>(sy_48 >> 32);
                temp = (temp > 0x8000)? temp - 0x8000 : 0;
                uint32 sy1 = temp >> 16;                    // src y #1
                uint32 sy2 = std::min(sy1+1,src.height-1);// src y #2
                float syf = (temp & 0xFFFF) / 65536.f; // weight of #2

                uchar *pdst = (uchar*)(dstdata + y * dst.bytesPerRow + z * dst.bytesPerImage);

                uint64 sx_48 = (stepx >> 1) - 1;
                for (size_t x = 0; x < dst.width; x++, sx_48+=stepx) {
                    temp = static_cast<unsigned int>(sx_48 >> 32);
                    temp = (temp > 0x8000)? temp - 0x8000 : 0;
                    uint32 sx1 = temp >> 16;                    // src x #1
                    uint32 sx2 = std::min(sx1+1,src.width-1);// src x #2
                    float sxf = (temp & 0xFFFF) / 65536.f; // weight of #2
                    
                    // process R,G,B,A simultaneously for cache coherence?
                    float accum[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

#define ACCUM4(x,y,z,factor) \
    { float f = factor; \
    float *psrc = (float*)(srcdata + x * srcelemsize + y * src.bytesPerRow + z * src.bytesPerImage); \
    accum[0] += f*psrc[0]; accum[1] += f*psrc[1]; accum[2] += f*psrc[2]; accum[3] += f*psrc[3]; }

#define ACCUM3(x,y,z,factor) \
    { float f = factor; \
    float *psrc = (float*)(srcdata + x * srcelemsize + y * src.bytesPerRow + z * src.bytesPerImage); \
    accum[0] += f * psrc[0]; accum[1] += f * psrc[1]; accum[2] += f * psrc[2]; }

#define ACCUM2(x,y,z,factor) \
    { float f = factor; \
    float *psrc = (float*)(srcdata + x * srcelemsize + y * src.bytesPerRow + z * src.bytesPerImage); \
    accum[0] += f * psrc[0]; accum[1] += f * psrc[1]; }

#define ACCUM1(x,y,z,factor) \
    { float f = factor; \
    float *psrc = (float*)(srcdata + x * srcelemsize + y * src.bytesPerRow + z * src.bytesPerImage); \
    accum[0] += f * psrc[0]; }

                    switch( minchannels )
                    {
                    case 4: // RGBA
                        ACCUM4(sx1,sy1,sz1,(1.0f-sxf)*(1.0f-syf)*(1.0f-szf));
                        ACCUM4(sx2,sy1,sz1,      sxf *(1.0f-syf)*(1.0f-szf));
                        ACCUM4(sx1,sy2,sz1,(1.0f-sxf)*      syf *(1.0f-szf));
                        ACCUM4(sx2,sy2,sz1,      sxf *      syf *(1.0f-szf));
                        ACCUM4(sx1,sy1,sz2,(1.0f-sxf)*(1.0f-syf)*      szf );
                        ACCUM4(sx2,sy1,sz2,      sxf *(1.0f-syf)*      szf );
                        ACCUM4(sx1,sy2,sz2,(1.0f-sxf)*      syf *      szf );
                        ACCUM4(sx2,sy2,sz2,      sxf *      syf *      szf );
                        break;
                    case 3: // RGB
                        ACCUM3(sx1,sy1,sz1,(1.0f-sxf)*(1.0f-syf)*(1.0f-szf));
                        ACCUM3(sx2,sy1,sz1,      sxf *(1.0f-syf)*(1.0f-szf));
                        ACCUM3(sx1,sy2,sz1,(1.0f-sxf)*      syf *(1.0f-szf));
                        ACCUM3(sx2,sy2,sz1,      sxf *      syf *(1.0f-szf));
                        ACCUM3(sx1,sy1,sz2,(1.0f-sxf)*(1.0f-syf)*      szf );
                        ACCUM3(sx2,sy1,sz2,      sxf *(1.0f-syf)*      szf );
                        ACCUM3(sx1,sy2,sz2,(1.0f-sxf)*      syf *      szf );
                        ACCUM3(sx2,sy2,sz2,      sxf *      syf *      szf );
                        break;
                    case 2: // RG
                        ACCUM2(sx1,sy1,sz1,(1.0f-sxf)*(1.0f-syf)*(1.0f-szf));
                        ACCUM2(sx2,sy1,sz1,      sxf *(1.0f-syf)*(1.0f-szf));
                        ACCUM2(sx1,sy2,sz1,(1.0f-sxf)*      syf *(1.0f-szf));
                        ACCUM2(sx2,sy2,sz1,      sxf *      syf *(1.0f-szf));
                        ACCUM2(sx1,sy1,sz2,(1.0f-sxf)*(1.0f-syf)*      szf );
                        ACCUM2(sx2,sy1,sz2,      sxf *(1.0f-syf)*      szf );
                        ACCUM2(sx1,sy2,sz2,(1.0f-sxf)*      syf *      szf );
                        ACCUM2(sx2,sy2,sz2,      sxf *      syf *      szf );
                        break;
                    case 1: // R
                        ACCUM1(sx1,sy1,sz1,(1.0f-sxf)*(1.0f-syf)*(1.0f-szf));
                        ACCUM1(sx2,sy1,sz1,      sxf *(1.0f-syf)*(1.0f-szf));
                        ACCUM1(sx1,sy2,sz1,(1.0f-sxf)*      syf *(1.0f-szf));
                        ACCUM1(sx2,sy2,sz1,      sxf *      syf *(1.0f-szf));
                        ACCUM1(sx1,sy1,sz2,(1.0f-sxf)*(1.0f-syf)*      szf );
                        ACCUM1(sx2,sy1,sz2,      sxf *(1.0f-syf)*      szf );
                        ACCUM1(sx1,sy2,sz2,(1.0f-sxf)*      syf *      szf );
                        ACCUM1(sx2,sy2,sz2,      sxf *      syf *      szf );
                        break;
                    }

                    memcpy(pdst, accum, dstelemsize);

#undef ACCUM4
#undef ACCUM3
#undef ACCUM2
#undef ACCUM1

                    pdst += dstelemsize;
                }
            }
        }
    }
};



// byte linear resampler, does not do any format conversions.
// only handles pixel formats that use 1 byte per color channel.
// 2D only; punts 3D pixelboxes to default LinearResampler (slow).
// templated on bytes-per-pixel to allow compiler optimizations, such
// as unrolling loops and replacing multiplies with bitshifts
template<unsigned int channels>
struct LinearResampler_Byte
{
    static void scale( const TextureBox& src, PixelFormatGpu srcFormat,
                       const TextureBox &dst, PixelFormatGpu dstFormat)
    {
        // only optimized for 2D
        if (src.getDepthOrSlices() > 1 || dst.getDepthOrSlices() > 1) {
            LinearResampler::scale( src, srcFormat, dst, dstFormat );
            return;
        }

        // srcdata stays at beginning of slice, pdst is a moving pointer
        uchar* srcdata = (uchar*)src.atFromOffsettedOrigin( 0, 0, 0 );
        uchar* dstdata = (uchar*)dst.atFromOffsettedOrigin( 0, 0, 0 );

        // sx_48,sy_48 represent current position in source
        // using 16/48-bit fixed precision, incremented by steps
        uint64 stepx = ((uint64)src.width << 48) / dst.width;
        uint64 stepy = ((uint64)src.height << 48) / dst.height;
        
        uint64 sy_48 = (stepy >> 1) - 1;
        for (size_t y = 0; y < dst.height; y++, sy_48+=stepy) {
            // bottom 28 bits of temp are 16/12 bit fixed precision, used to
            // adjust a source coordinate backwards by half a pixel so that the
            // integer bits represent the first sample (eg, sx1) and the
            // fractional bits are the blend weight of the second sample
            unsigned int temp = static_cast<unsigned int>(sy_48 >> 36);
            temp = (temp > 0x800)? temp - 0x800: 0;
            unsigned int syf = temp & 0xFFF;
            uint32 sy1 = temp >> 12;
            uint32 sy2 = std::min(sy1+1, src.height-1);
            size_t syoff1 = sy1 * src.bytesPerRow;
            size_t syoff2 = sy2 * src.bytesPerRow;

            uchar *pdst = (uchar *)( dstdata + y * dst.bytesPerRow );

            uint64 sx_48 = (stepx >> 1) - 1;
            for (size_t x = 0; x < dst.width; x++, sx_48+=stepx) {
                temp = static_cast<unsigned int>(sx_48 >> 36);
                temp = (temp > 0x800)? temp - 0x800 : 0;
                unsigned int sxf = temp & 0xFFF;
                uint32 sx1 = temp >> 12;
                uint32 sx2 = std::min(sx1+1, src.width-1);

                unsigned int sxfsyf = sxf*syf;
                for (unsigned int k = 0; k < channels; k++) {
                    unsigned int accum =
                        srcdata[syoff1+sx1*channels+k]*(0x1000000-(sxf<<12)-(syf<<12)+sxfsyf) +
                        srcdata[syoff1+sx2*channels+k]*((sxf<<12)-sxfsyf) +
                        srcdata[syoff2+sx1*channels+k]*((syf<<12)-sxfsyf) +
                        srcdata[syoff2+sx2*channels+k]*sxfsyf;
                    // accum is computed using 8/24-bit fixed-point math
                    // (maximum is 0xFF000000; rounding will not cause overflow)
                    *pdst++ = static_cast<uchar>((accum + 0x800000) >> 24);
                }
            }
        }
    }
};
/** @} */
/** @} */

}

#endif
