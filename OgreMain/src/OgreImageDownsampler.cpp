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

#include "OgreStableHeaders.h"

#include "OgreVector3.h"
#include "OgreMatrix3.h"

#include "OgreImageDownsampler.h"

namespace Ogre
{
    struct CubemapUVI
    {
        Real    u;
        Real    v;
        int     face;
    };

    enum MajorAxis
    {
        MajorAxisX,
        MajorAxisY,
        MajorAxisZ,
    };

    struct FaceComponents
    {
        uint8   uIdx;
        uint8   vIdx;
        Real    uSign;
        Real    vSign;

        FaceComponents( uint8 _uIdx, uint8 _vIdx, Real _uSign, Real _vSign ) :
            uIdx( _uIdx ), vIdx( _vIdx ), uSign( _uSign ), vSign( _vSign )
        {
        }
    };

    const FaceComponents c_faceComponents[6] =
    {
        FaceComponents( 2, 1, -0.5f, -0.5f ),
        FaceComponents( 2, 1,  0.5f, -0.5f ),
        FaceComponents( 0, 2,  0.5f,  0.5f ),
        FaceComponents( 0, 2,  0.5f, -0.5f ),
        FaceComponents( 0, 1,  0.5f, -0.5f ),
        FaceComponents( 0, 1, -0.5f, -0.5f )
    };

    struct FaceSwizzle
    {
        int8    iX, iY, iZ;
        Real    signX, signY, signZ;
        FaceSwizzle( int8 _iX, int8 _iY, int8 _iZ, Real _signX, Real _signY, Real _signZ ) :
            iX( _iX ), iY( _iY ), iZ( _iZ ), signX( _signX ), signY( _signY ), signZ( _signZ ) {}
    };

    static const FaceSwizzle c_faceSwizzles[6] =
    {
        FaceSwizzle( 2, 1, 0,  1,  1, -1 ),
        FaceSwizzle( 2, 1, 0, -1,  1,  1 ),
        FaceSwizzle( 0, 2, 1,  1,  1, -1 ),
        FaceSwizzle( 0, 2, 1,  1, -1,  1 ),
        FaceSwizzle( 0, 1, 2,  1,  1,  1 ),
        FaceSwizzle( 0, 1, 2, -1,  1, -1 ),
    };

    inline CubemapUVI cubeMapProject( Vector3 vDir )
    {
        CubemapUVI uvi;

        Vector3 absDir = Vector3( fabsf( vDir.x ), fabsf( vDir.y ), fabsf( vDir.z ) );

        bool majorX = (absDir.x >= absDir.y) & (absDir.x >= absDir.z);
        bool majorY = (absDir.y >= absDir.x) & (absDir.y >= absDir.z);
        bool majorZ = (absDir.z >= absDir.x) & (absDir.z >= absDir.y);

        Real fNorm;
        MajorAxis majorAxis;
        majorAxis   = majorX ? MajorAxisX : MajorAxisY;
        fNorm       = majorX ? absDir.x : absDir.y;

        majorAxis   = majorY ? MajorAxisY : majorAxis;
        fNorm       = majorY ? absDir.y : fNorm;

        majorAxis   = majorZ ? MajorAxisZ : majorAxis;
        fNorm       = majorZ ? absDir.z : fNorm;

        const Real *fDirs = vDir.ptr();

        uvi.face = fDirs[majorAxis] >= 0 ? majorAxis * 2 : majorAxis * 2 + 1;

        fNorm = 1.0f / fNorm;

        uvi.u =  c_faceComponents[uvi.face].uSign *
                    fDirs[c_faceComponents[uvi.face].uIdx] * fNorm + 0.5f;
        uvi.v =  c_faceComponents[uvi.face].vSign *
                    fDirs[c_faceComponents[uvi.face].vIdx] * fNorm + 0.5f;

        return uvi;
    }

    const FilterKernel c_filterKernels[3] =
    {
        {
            //Point
            {
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 1, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0
            },
            0, 0,
            0, 0
        },
        {
            //Linear
            {
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 1, 1, 0,
                0, 0, 1, 1, 0,
                0, 0, 0, 0, 0
            },
            0, 1,
            0, 1
        },
        {
            //Gaussian
            {
                1,  4,  7,  4, 1,
                4, 16, 26, 16, 4,
                7, 26, 41, 26, 7,
                4, 16, 26, 16, 4,
                1,  4,  7,  4, 1
            },
            -2, 2,
            -2, 2
        }
    };

    const FilterSeparableKernel c_filterSeparableKernels[1] =
    {
        {
            //Gaussian High
            {
                40, 161, 255, 161, 40
            },
            -2, 2
        }
    };
}

#define OGRE_GAM_TO_LIN( x ) x
#define OGRE_LIN_TO_GAM( x ) x
#define OGRE_UINT8 uint8
#define OGRE_UINT32 uint32
#define OGRE_ROUND_HALF 0.5f

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_DOWNSAMPLE_B 2
#define OGRE_DOWNSAMPLE_A 3
#define OGRE_TOTAL_SIZE 4
#define DOWNSAMPLE_NAME downscale2x_XXXA8888
#define DOWNSAMPLE_CUBE_NAME downscale2x_XXXA8888_cube
#define BLUR_NAME separableBlur_XXXA8888
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_DOWNSAMPLE_B 2
#define OGRE_TOTAL_SIZE 3
#define DOWNSAMPLE_NAME downscale2x_XXX888
	#define DOWNSAMPLE_CUBE_NAME downscale2x_XXX888_cube
#define BLUR_NAME separableBlur_XXX888
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_TOTAL_SIZE 2
#define DOWNSAMPLE_NAME downscale2x_XX88
	#define DOWNSAMPLE_CUBE_NAME downscale2x_XX88_cube
#define BLUR_NAME separableBlur_XX88
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_TOTAL_SIZE 1
#define DOWNSAMPLE_NAME downscale2x_X8
	#define DOWNSAMPLE_CUBE_NAME downscale2x_X8_cube
#define BLUR_NAME separableBlur_X8
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_A 0
#define OGRE_TOTAL_SIZE 1
#define DOWNSAMPLE_NAME downscale2x_A8
	#define DOWNSAMPLE_CUBE_NAME downscale2x_A8_cube
#define BLUR_NAME separableBlur_A8
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_A 1
#define OGRE_TOTAL_SIZE 2
#define DOWNSAMPLE_NAME downscale2x_XA88
	#define DOWNSAMPLE_CUBE_NAME downscale2x_XA88_cube
#define BLUR_NAME separableBlur_XA88
#include "OgreImageDownsamplerImpl.inl"

//-----------------------------------------------------------------------------------
//Signed versions
//-----------------------------------------------------------------------------------

#undef OGRE_UINT8
#undef OGRE_UINT32
#define OGRE_UINT8 int8
#define OGRE_UINT32 int32

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_DOWNSAMPLE_B 2
#define OGRE_DOWNSAMPLE_A 3
#define OGRE_TOTAL_SIZE 4
#define DOWNSAMPLE_NAME downscale2x_Signed_XXXA8888
#define DOWNSAMPLE_CUBE_NAME downscale2x_Signed_XXXA8888_cube
#define BLUR_NAME separableBlur_Signed_XXXA8888
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_DOWNSAMPLE_B 2
#define OGRE_TOTAL_SIZE 3
#define DOWNSAMPLE_NAME downscale2x_Signed_XXX888
#define DOWNSAMPLE_CUBE_NAME downscale2x_Signed_XXX888_cube
#define BLUR_NAME separableBlur_Signed_XXX888
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_TOTAL_SIZE 2
#define DOWNSAMPLE_NAME downscale2x_Signed_XX88
#define DOWNSAMPLE_CUBE_NAME downscale2x_Signed_XX88_cube
#define BLUR_NAME separableBlur_Signed_XX88
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_TOTAL_SIZE 1
#define DOWNSAMPLE_NAME downscale2x_Signed_X8
#define DOWNSAMPLE_CUBE_NAME downscale2x_Signed_X8_cube
#define BLUR_NAME separableBlur_Signed_X8
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_A 0
#define OGRE_TOTAL_SIZE 1
#define DOWNSAMPLE_NAME downscale2x_Signed_A8
#define DOWNSAMPLE_CUBE_NAME downscale2x_Signed_A8_cube
#define BLUR_NAME separableBlur_Signed_A8
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_A 1
#define OGRE_TOTAL_SIZE 2
#define DOWNSAMPLE_NAME downscale2x_Signed_XA88
#define DOWNSAMPLE_CUBE_NAME downscale2x_Signed_XA88_cube
#define BLUR_NAME separableBlur_Signed_XA88
#include "OgreImageDownsamplerImpl.inl"

//-----------------------------------------------------------------------------------
//Float32 versions
//-----------------------------------------------------------------------------------

#undef OGRE_UINT8
#undef OGRE_UINT32
#undef OGRE_ROUND_HALF
#define OGRE_UINT8 float
#define OGRE_UINT32 float
#define OGRE_ROUND_HALF 0.0f

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_DOWNSAMPLE_B 2
#define OGRE_DOWNSAMPLE_A 3
#define OGRE_TOTAL_SIZE 4
#define DOWNSAMPLE_NAME downscale2x_Float32_XXXA
#define DOWNSAMPLE_CUBE_NAME downscale2x_Float32_XXXA_cube
#define BLUR_NAME separableBlur_Float32_XXXA
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_DOWNSAMPLE_B 2
#define OGRE_TOTAL_SIZE 3
#define DOWNSAMPLE_NAME downscale2x_Float32_XXX
#define DOWNSAMPLE_CUBE_NAME downscale2x_Float32_XXX_cube
#define BLUR_NAME separableBlur_Float32_XXX
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_TOTAL_SIZE 2
#define DOWNSAMPLE_NAME downscale2x_Float32_XX
#define DOWNSAMPLE_CUBE_NAME downscale2x_Float32_XX_cube
#define BLUR_NAME separableBlur_Float32_XX
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_TOTAL_SIZE 1
#define DOWNSAMPLE_NAME downscale2x_Float32_X
#define DOWNSAMPLE_CUBE_NAME downscale2x_Float32_X_cube
#define BLUR_NAME separableBlur_Float32_X
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_A 0
#define OGRE_TOTAL_SIZE 1
#define DOWNSAMPLE_NAME downscale2x_Float32_A
#define DOWNSAMPLE_CUBE_NAME downscale2x_Float32_A_cube
#define BLUR_NAME separableBlur_Float32_A
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_A 1
#define OGRE_TOTAL_SIZE 2
#define DOWNSAMPLE_NAME downscale2x_Float32_XA
#define DOWNSAMPLE_CUBE_NAME downscale2x_Float32_XA_cube
#define BLUR_NAME separableBlur_Float32_XA
#include "OgreImageDownsamplerImpl.inl"

//-----------------------------------------------------------------------------------
//sRGB versions
//-----------------------------------------------------------------------------------

#undef OGRE_GAM_TO_LIN
#undef OGRE_LIN_TO_GAM
#define OGRE_GAM_TO_LIN( x ) x * x
#define OGRE_LIN_TO_GAM( x ) sqrtf( x )

#undef OGRE_UINT8
#undef OGRE_UINT32
#undef OGRE_ROUND_HALF
#define OGRE_UINT8 uint8
#define OGRE_UINT32 uint32
#define OGRE_ROUND_HALF 0.5f

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_DOWNSAMPLE_B 2
#define OGRE_DOWNSAMPLE_A 3
#define OGRE_TOTAL_SIZE 4
#define DOWNSAMPLE_NAME downscale2x_sRGB_XXXA8888
#define DOWNSAMPLE_CUBE_NAME downscale2x_sRGB_XXXA8888_cube
#define BLUR_NAME separableBlur_sRGB_XXXA8888
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_A 0
#define OGRE_DOWNSAMPLE_R 1
#define OGRE_DOWNSAMPLE_G 2
#define OGRE_DOWNSAMPLE_B 3
#define OGRE_TOTAL_SIZE 4
#define DOWNSAMPLE_NAME downscale2x_sRGB_AXXX8888
#define DOWNSAMPLE_CUBE_NAME downscale2x_sRGB_AXXX8888_cube
#define BLUR_NAME separableBlur_sRGB_AXXX8888
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_DOWNSAMPLE_B 2
#define OGRE_TOTAL_SIZE 3
#define DOWNSAMPLE_NAME downscale2x_sRGB_XXX888
#define DOWNSAMPLE_CUBE_NAME downscale2x_sRGB_XXX888_cube
#define BLUR_NAME separableBlur_sRGB_XXX888
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_G 1
#define OGRE_TOTAL_SIZE 2
#define DOWNSAMPLE_NAME downscale2x_sRGB_XX88
#define DOWNSAMPLE_CUBE_NAME downscale2x_sRGB_XX88_cube
#define BLUR_NAME separableBlur_sRGB_XX88
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_TOTAL_SIZE 1
#define DOWNSAMPLE_NAME downscale2x_sRGB_X8
#define DOWNSAMPLE_CUBE_NAME downscale2x_sRGB_X8_cube
#define BLUR_NAME separableBlur_sRGB_X8
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_A 0
#define OGRE_TOTAL_SIZE 1
#define DOWNSAMPLE_NAME downscale2x_sRGB_A8
#define DOWNSAMPLE_CUBE_NAME downscale2x_sRGB_A8_cube
#define BLUR_NAME separableBlur_sRGB_A8
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_R 0
#define OGRE_DOWNSAMPLE_A 1
#define OGRE_TOTAL_SIZE 2
#define DOWNSAMPLE_NAME downscale2x_sRGB_XA88
#define DOWNSAMPLE_CUBE_NAME downscale2x_sRGB_XA88_cube
#define BLUR_NAME separableBlur_sRGB_XA88
#include "OgreImageDownsamplerImpl.inl"

#define OGRE_DOWNSAMPLE_A 0
#define OGRE_DOWNSAMPLE_R 1
#define OGRE_TOTAL_SIZE 2
#define DOWNSAMPLE_NAME downscale2x_sRGB_AX88
#define DOWNSAMPLE_CUBE_NAME downscale2x_sRGB_AX88_cube
#define BLUR_NAME separableBlur_sRGB_AX88
#include "OgreImageDownsamplerImpl.inl"

#undef OGRE_GAM_TO_LIN
#undef OGRE_LIN_TO_GAM
