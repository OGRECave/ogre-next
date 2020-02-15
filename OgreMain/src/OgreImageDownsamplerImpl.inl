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

#include "OgreMatrix3.h"
#include "OgreVector3.h"

namespace Ogre
{
    void DOWNSAMPLE_NAME( uint8 *_dstPtr, uint8 const *_srcPtr, int32 dstWidth, int32 dstHeight,
                          int32 dstBytesPerRow, int32 srcWidth, int32 srcBytesPerRow,
                          const uint8 kernel[5][5], const int8 kernelStartX, const int8 kernelEndX,
                          const int8 kernelStartY, const int8 kernelEndY )
    {
        OGRE_UINT8 *dstPtr = reinterpret_cast<OGRE_UINT8 *>( _dstPtr );
        OGRE_UINT8 const *srcPtr = reinterpret_cast<OGRE_UINT8 const *>( _srcPtr );

        srcBytesPerRow /= sizeof( OGRE_UINT8 );
        dstBytesPerRow /= sizeof( OGRE_UINT8 );

        int32 srcBytesPerRowSkip = srcBytesPerRow - srcWidth * OGRE_TOTAL_SIZE;
        int32 dstBytesPerRowSkip = dstBytesPerRow - dstWidth * OGRE_TOTAL_SIZE;

        for( int32 y = 0; y < dstHeight; ++y )
        {
            for( int32 x = 0; x < dstWidth; ++x )
            {
                int kStartY = std::max<int>( -y, kernelStartY );
                int kEndY = std::min<int>( dstHeight - y - 1, kernelEndY );

#ifdef OGRE_DOWNSAMPLE_R
                OGRE_UINT32 accumR = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                OGRE_UINT32 accumG = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                OGRE_UINT32 accumB = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                OGRE_UINT32 accumA = 0;
#endif

                uint32 divisor = 0;

                for( int k_y = kStartY; k_y <= kEndY; ++k_y )
                {
                    int kStartX = std::max<int>( -x, kernelStartX );
                    int kEndX = std::min<int>( dstWidth - 1 - x, kernelEndX );

                    for( int k_x = kStartX; k_x <= kEndX; ++k_x )
                    {
                        uint32 kernelVal = kernel[k_y + 2][k_x + 2];

#ifdef OGRE_DOWNSAMPLE_R
                        OGRE_UINT32 r =
                            srcPtr[k_y * srcBytesPerRow + k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_R];
                        accumR += OGRE_GAM_TO_LIN( r ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                        OGRE_UINT32 g =
                            srcPtr[k_y * srcBytesPerRow + k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_G];
                        accumG += OGRE_GAM_TO_LIN( g ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                        OGRE_UINT32 b =
                            srcPtr[k_y * srcBytesPerRow + k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_B];
                        accumB += OGRE_GAM_TO_LIN( b ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                        OGRE_UINT32 a =
                            srcPtr[k_y * srcBytesPerRow + k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_A];
                        accumA += a * kernelVal;
#endif

                        divisor += kernelVal;
                    }
                }

#if defined( OGRE_DOWNSAMPLE_R ) || defined( OGRE_DOWNSAMPLE_G ) || defined( OGRE_DOWNSAMPLE_B )
                float invDivisor = 1.0f / divisor;
#endif

#ifdef OGRE_DOWNSAMPLE_R
                dstPtr[OGRE_DOWNSAMPLE_R] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumR * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_G
                dstPtr[OGRE_DOWNSAMPLE_G] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumG * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_B
                dstPtr[OGRE_DOWNSAMPLE_B] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumB * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_A
                dstPtr[OGRE_DOWNSAMPLE_A] =
                    static_cast<OGRE_UINT8>( ( accumA + divisor - 1u ) / divisor );
#endif

                dstPtr += OGRE_TOTAL_SIZE;
                srcPtr += OGRE_TOTAL_SIZE * 2;
            }

            dstPtr += dstBytesPerRowSkip;
            srcPtr += ( srcWidth - dstWidth * 2 ) * OGRE_TOTAL_SIZE + srcBytesPerRowSkip;
            srcPtr += srcBytesPerRow;
        }
    }
    //-----------------------------------------------------------------------------------
    void DOWNSAMPLE_CUBE_NAME( uint8 *_dstPtr, uint8 const **_allPtr, int32 dstWidth, int32 dstHeight,
                               int32 dstBytesPerRow, int32 srcWidth, int32 srcHeight,
                               int32 srcBytesPerRow, const uint8 kernel[5][5], const int8 kernelStartX,
                               const int8 kernelEndX, const int8 kernelStartY, const int8 kernelEndY,
                               uint8 currentFace )
    {
        OGRE_UINT8 *dstPtr = reinterpret_cast<OGRE_UINT8 *>( _dstPtr );
        OGRE_UINT8 const **allPtr = reinterpret_cast<OGRE_UINT8 const **>( _allPtr );

        srcBytesPerRow /= sizeof( OGRE_UINT8 );
        dstBytesPerRow /= sizeof( OGRE_UINT8 );

        Quaternion kRotations[5][5];

        {
            Radian xRadStep( Ogre::Math::PI / ( srcWidth * 2.0f ) );
            Radian yRadStep( Ogre::Math::PI / ( srcHeight * 2.0f ) );

            for( int y = kernelStartY; y <= kernelEndY; ++y )
            {
                for( int x = kernelStartX; x <= kernelEndX; ++x )
                {
                    Matrix3 m3;
                    m3.FromEulerAnglesXYZ( (Real)-y * yRadStep, (Real)x * xRadStep, Radian( 0 ) );
                    kRotations[y + 2][x + 2] = Quaternion( m3 );
                }
            }
        }

        const FaceSwizzle &faceSwizzle = c_faceSwizzles[currentFace];

        Real invSrcWidth = 1.0f / srcWidth;
        Real invSrcHeight = 1.0f / srcHeight;

        int32 dstBytesPerRowSkip = dstBytesPerRow - dstWidth * OGRE_TOTAL_SIZE;

        OGRE_UINT8 const *srcPtr = 0;

        for( int32 y = 0; y < dstHeight; ++y )
        {
            for( int32 x = 0; x < dstWidth; ++x )
            {
#ifdef OGRE_DOWNSAMPLE_R
                OGRE_UINT32 accumR = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                OGRE_UINT32 accumG = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                OGRE_UINT32 accumB = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                OGRE_UINT32 accumA = 0;
#endif

                uint32 divisor = 0;

                Vector3 vForwardSample( ( x * 2 + 0.5f ) * invSrcWidth * 2.0f - 1.0f,
                                        ( y * 2 + 0.5f ) * invSrcHeight * -2.0f + 1.0f, 1.0f );

                for( int k_y = kernelStartY; k_y <= kernelEndY; ++k_y )
                {
                    for( int k_x = kernelStartX; k_x <= kernelEndX; ++k_x )
                    {
                        uint32 kernelVal = kernel[k_y + 2][k_x + 2];

                        Vector3 tmp = kRotations[k_y + 2][k_x + 2] * vForwardSample;

                        Vector3 vSample;
                        vSample.ptr()[0] = tmp.ptr()[faceSwizzle.iX] * faceSwizzle.signX;
                        vSample.ptr()[1] = tmp.ptr()[faceSwizzle.iY] * faceSwizzle.signY;
                        vSample.ptr()[2] = tmp.ptr()[faceSwizzle.iZ] * faceSwizzle.signZ;

                        CubemapUVI uvi = cubeMapProject( vSample );

                        int iu =
                            std::min( static_cast<int>( floorf( uvi.u * srcWidth ) ), srcWidth - 1 );
                        int iv =
                            std::min( static_cast<int>( floorf( uvi.v * srcHeight ) ), srcHeight - 1 );

                        srcPtr = allPtr[uvi.face] + iv * srcBytesPerRow + iu * OGRE_TOTAL_SIZE;

#ifdef OGRE_DOWNSAMPLE_R
                        OGRE_UINT32 r = srcPtr[OGRE_DOWNSAMPLE_R];
                        accumR += OGRE_GAM_TO_LIN( r ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                        OGRE_UINT32 g = srcPtr[OGRE_DOWNSAMPLE_G];
                        accumG += OGRE_GAM_TO_LIN( g ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                        OGRE_UINT32 b = srcPtr[OGRE_DOWNSAMPLE_B];
                        accumB += OGRE_GAM_TO_LIN( b ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                        OGRE_UINT32 a = srcPtr[OGRE_DOWNSAMPLE_A];
                        accumA += a * kernelVal;
#endif

                        divisor += kernelVal;
                    }
                }

#if defined( OGRE_DOWNSAMPLE_R ) || defined( OGRE_DOWNSAMPLE_G ) || defined( OGRE_DOWNSAMPLE_B )
                float invDivisor = 1.0f / divisor;
#endif

#ifdef OGRE_DOWNSAMPLE_R
                dstPtr[OGRE_DOWNSAMPLE_R] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumR * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_G
                dstPtr[OGRE_DOWNSAMPLE_G] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumG * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_B
                dstPtr[OGRE_DOWNSAMPLE_B] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumB * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_A
                dstPtr[OGRE_DOWNSAMPLE_A] =
                    static_cast<OGRE_UINT8>( ( accumA + divisor - 1u ) / divisor );
#endif

                dstPtr += OGRE_TOTAL_SIZE;
            }

            dstPtr += dstBytesPerRowSkip;
        }
    }
    //-----------------------------------------------------------------------------------
    void BLUR_NAME( uint8 *_tmpPtr, uint8 *_srcDstPtr, int32 width, int32 height, int32 bytesPerRow,
                    const uint8 kernel[5], const int8 kernelStart, const int8 kernelEnd )
    {
        OGRE_UINT8 *dstPtr = reinterpret_cast<OGRE_UINT8 *>( _tmpPtr );
        OGRE_UINT8 const *srcPtr = reinterpret_cast<OGRE_UINT8 const *>( _srcDstPtr );

        bytesPerRow /= sizeof( OGRE_UINT8 );

        const int32 bytesPerRowSkip = bytesPerRow - width * OGRE_TOTAL_SIZE;

        for( int32 y = 0; y < height; ++y )
        {
            for( int32 x = 0; x < width; ++x )
            {
#ifdef OGRE_DOWNSAMPLE_R
                OGRE_UINT32 accumR = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                OGRE_UINT32 accumG = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                OGRE_UINT32 accumB = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                OGRE_UINT32 accumA = 0;
#endif

                uint32 divisor = 0;

                int kStartX = std::max<int>( -x, kernelStart );
                int kEndX = std::min<int>( width - 1 - x, kernelEnd );

                for( int k_x = kStartX; k_x <= kEndX; ++k_x )
                {
                    uint32 kernelVal = kernel[k_x + 2];

#ifdef OGRE_DOWNSAMPLE_R
                    OGRE_UINT32 r = srcPtr[k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_R];
                    accumR += OGRE_GAM_TO_LIN( r ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                    OGRE_UINT32 g = srcPtr[k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_G];
                    accumG += OGRE_GAM_TO_LIN( g ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                    OGRE_UINT32 b = srcPtr[k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_B];
                    accumB += OGRE_GAM_TO_LIN( b ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                    OGRE_UINT32 a = srcPtr[k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_A];
                    accumA += a * kernelVal;
#endif

                    divisor += kernelVal;
                }

#if defined( OGRE_DOWNSAMPLE_R ) || defined( OGRE_DOWNSAMPLE_G ) || defined( OGRE_DOWNSAMPLE_B )
                float invDivisor = 1.0f / divisor;
#endif

#ifdef OGRE_DOWNSAMPLE_R
                dstPtr[OGRE_DOWNSAMPLE_R] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumR * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_G
                dstPtr[OGRE_DOWNSAMPLE_G] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumG * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_B
                dstPtr[OGRE_DOWNSAMPLE_B] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumB * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_A
                dstPtr[OGRE_DOWNSAMPLE_A] =
                    static_cast<OGRE_UINT8>( ( accumA + divisor - 1u ) / divisor );
#endif

                dstPtr += OGRE_TOTAL_SIZE;
                srcPtr += OGRE_TOTAL_SIZE;
            }

            dstPtr += bytesPerRowSkip;
            srcPtr += bytesPerRowSkip;
        }

        dstPtr = reinterpret_cast<OGRE_UINT8 *>( _srcDstPtr );
        srcPtr = reinterpret_cast<OGRE_UINT8 const *>( _tmpPtr );

        for( int32 y = 0; y < height; ++y )
        {
            for( int32 x = 0; x < width; ++x )
            {
#ifdef OGRE_DOWNSAMPLE_R
                OGRE_UINT32 accumR = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                OGRE_UINT32 accumG = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                OGRE_UINT32 accumB = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                OGRE_UINT32 accumA = 0;
#endif

                uint32 divisor = 0;

                int kStartY = std::max<int>( -y, kernelStart );
                int kEndY = std::min<int>( height - y - 1, kernelEnd );

                for( int k_y = kStartY; k_y <= kEndY; ++k_y )
                {
                    uint32 kernelVal = kernel[k_y + 2];

#ifdef OGRE_DOWNSAMPLE_R
                    OGRE_UINT32 r = srcPtr[k_y * bytesPerRow + OGRE_DOWNSAMPLE_R];
                    accumR += OGRE_GAM_TO_LIN( r ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                    OGRE_UINT32 g = srcPtr[k_y * bytesPerRow + OGRE_DOWNSAMPLE_G];
                    accumG += OGRE_GAM_TO_LIN( g ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                    OGRE_UINT32 b = srcPtr[k_y * bytesPerRow + OGRE_DOWNSAMPLE_B];
                    accumB += OGRE_GAM_TO_LIN( b ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                    OGRE_UINT32 a = srcPtr[k_y * bytesPerRow + OGRE_DOWNSAMPLE_A];
                    accumA += a * kernelVal;
#endif

                    divisor += kernelVal;
                }

#if defined( OGRE_DOWNSAMPLE_R ) || defined( OGRE_DOWNSAMPLE_G ) || defined( OGRE_DOWNSAMPLE_B )
                float invDivisor = 1.0f / divisor;
#endif

#ifdef OGRE_DOWNSAMPLE_R
                dstPtr[OGRE_DOWNSAMPLE_R] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumR * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_G
                dstPtr[OGRE_DOWNSAMPLE_G] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumG * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_B
                dstPtr[OGRE_DOWNSAMPLE_B] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumB * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_A
                dstPtr[OGRE_DOWNSAMPLE_A] =
                    static_cast<OGRE_UINT8>( ( accumA + divisor - 1u ) / divisor );
#endif

                dstPtr += OGRE_TOTAL_SIZE;
                srcPtr += OGRE_TOTAL_SIZE;
            }

            dstPtr += bytesPerRowSkip;
            srcPtr += bytesPerRowSkip;
        }
    }
    //-----------------------------------------------------------------------------------
    void BLUR_NAME( uint8 *_tmpPtr, uint8 *_srcDstPtr, int32 width, int32 height, const uint8 kernel[5],
                    const int8 kernelStart, const int8 kernelEnd )
    {
        OGRE_UINT8 *dstPtr = reinterpret_cast<OGRE_UINT8 *>( _tmpPtr );
        OGRE_UINT8 const *srcPtr = reinterpret_cast<OGRE_UINT8 const *>( _srcDstPtr );

        const size_t bytesPerRow = width * OGRE_TOTAL_SIZE;

        for( int32 y = 0; y < height; ++y )
        {
            for( int32 x = 0; x < width; ++x )
            {
#ifdef OGRE_DOWNSAMPLE_R
                OGRE_UINT32 accumR = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                OGRE_UINT32 accumG = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                OGRE_UINT32 accumB = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                OGRE_UINT32 accumA = 0;
#endif

                uint32 divisor = 0;

                int kStartX = std::max<int>( -x, kernelStart );
                int kEndX = std::min<int>( width - 1 - x, kernelEnd );

                for( int k_x = kStartX; k_x <= kEndX; ++k_x )
                {
                    uint32 kernelVal = kernel[k_x + 2];

#ifdef OGRE_DOWNSAMPLE_R
                    OGRE_UINT32 r = srcPtr[k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_R];
                    accumR += OGRE_GAM_TO_LIN( r ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                    OGRE_UINT32 g = srcPtr[k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_G];
                    accumG += OGRE_GAM_TO_LIN( g ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                    OGRE_UINT32 b = srcPtr[k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_B];
                    accumB += OGRE_GAM_TO_LIN( b ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                    OGRE_UINT32 a = srcPtr[k_x * OGRE_TOTAL_SIZE + OGRE_DOWNSAMPLE_A];
                    accumA += a * kernelVal;
#endif

                    divisor += kernelVal;
                }

#if defined( OGRE_DOWNSAMPLE_R ) || defined( OGRE_DOWNSAMPLE_G ) || defined( OGRE_DOWNSAMPLE_B )
                float invDivisor = 1.0f / divisor;
#endif

#ifdef OGRE_DOWNSAMPLE_R
                dstPtr[OGRE_DOWNSAMPLE_R] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumR * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_G
                dstPtr[OGRE_DOWNSAMPLE_G] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumG * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_B
                dstPtr[OGRE_DOWNSAMPLE_B] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumB * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_A
                dstPtr[OGRE_DOWNSAMPLE_A] =
                    static_cast<OGRE_UINT8>( ( accumA + divisor - 1u ) / divisor );
#endif

                dstPtr += OGRE_TOTAL_SIZE;
                srcPtr += OGRE_TOTAL_SIZE;
            }
        }

        dstPtr = reinterpret_cast<OGRE_UINT8 *>( _srcDstPtr );
        srcPtr = reinterpret_cast<OGRE_UINT8 const *>( _tmpPtr );

        for( int32 y = 0; y < height; ++y )
        {
            for( int32 x = 0; x < width; ++x )
            {
#ifdef OGRE_DOWNSAMPLE_R
                OGRE_UINT32 accumR = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                OGRE_UINT32 accumG = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                OGRE_UINT32 accumB = 0;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                OGRE_UINT32 accumA = 0;
#endif

                uint32 divisor = 0;

                int kStartY = std::max<int>( -y, kernelStart );
                int kEndY = std::min<int>( height - y - 1, kernelEnd );

                for( int k_y = kStartY; k_y <= kEndY; ++k_y )
                {
                    uint32 kernelVal = kernel[k_y + 2];

#ifdef OGRE_DOWNSAMPLE_R
                    OGRE_UINT32 r = srcPtr[k_y * bytesPerRow + OGRE_DOWNSAMPLE_R];
                    accumR += OGRE_GAM_TO_LIN( r ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_G
                    OGRE_UINT32 g = srcPtr[k_y * bytesPerRow + OGRE_DOWNSAMPLE_G];
                    accumG += OGRE_GAM_TO_LIN( g ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_B
                    OGRE_UINT32 b = srcPtr[k_y * bytesPerRow + OGRE_DOWNSAMPLE_B];
                    accumB += OGRE_GAM_TO_LIN( b ) * kernelVal;
#endif
#ifdef OGRE_DOWNSAMPLE_A
                    OGRE_UINT32 a = srcPtr[k_y * bytesPerRow + OGRE_DOWNSAMPLE_A];
                    accumA += a * kernelVal;
#endif

                    divisor += kernelVal;
                }

#if defined( OGRE_DOWNSAMPLE_R ) || defined( OGRE_DOWNSAMPLE_G ) || defined( OGRE_DOWNSAMPLE_B )
                float invDivisor = 1.0f / divisor;
#endif

#ifdef OGRE_DOWNSAMPLE_R
                dstPtr[OGRE_DOWNSAMPLE_R] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumR * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_G
                dstPtr[OGRE_DOWNSAMPLE_G] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumG * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_B
                dstPtr[OGRE_DOWNSAMPLE_B] =
                    static_cast<OGRE_UINT8>( OGRE_LIN_TO_GAM( accumB * invDivisor ) + OGRE_ROUND_HALF );
#endif
#ifdef OGRE_DOWNSAMPLE_A
                dstPtr[OGRE_DOWNSAMPLE_A] =
                    static_cast<OGRE_UINT8>( ( accumA + divisor - 1u ) / divisor );
#endif

                dstPtr += OGRE_TOTAL_SIZE;
                srcPtr += OGRE_TOTAL_SIZE;
            }
        }
    }
}  // namespace Ogre

#undef OGRE_DOWNSAMPLE_A
#undef OGRE_DOWNSAMPLE_R
#undef OGRE_DOWNSAMPLE_G
#undef OGRE_DOWNSAMPLE_B
#undef DOWNSAMPLE_NAME
#undef DOWNSAMPLE_CUBE_NAME
#undef BLUR_NAME
#undef OGRE_TOTAL_SIZE
