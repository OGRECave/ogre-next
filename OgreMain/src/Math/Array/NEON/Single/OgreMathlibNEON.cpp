/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
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

#include <OgreStableHeaders.h>

#include "OgrePlatformInformation.h"

#if __OGRE_HAVE_NEON
#    define __Mathlib_H__  // Needed to directly include OgreMathlibNEON

#    include "Math/Array/NEON/Single/OgreMathlibNEON.h"
#    include "Math/Array/NEON/Single/neon_mathfun.h"
#    include "Math/Array/OgreArrayConfig.h"
#    include "Math/Array/OgreBooleanMask.h"

namespace Ogre
{
#    define init_list_4( a ) \
        { \
            a, a, a, a \
        }
    const float32x4_ct MathlibNEON::HALF = init_list_4( 0.5f );
    const float32x4_ct MathlibNEON::ONE = init_list_4( 1.0f );
    const float32x4_ct MathlibNEON::THREE = init_list_4( 3.0f );
    const float32x4_ct MathlibNEON::NEG_ONE = init_list_4( -1.0f );
    const float32x4_ct MathlibNEON::fEpsilon = init_list_4( 1e-6f );
    const float32x4_ct MathlibNEON::fSqEpsilon = init_list_4( 1e-12f );
    const float32x4_ct MathlibNEON::OneMinusEpsilon = init_list_4( 1.0f - 1e-6f );
    const float32x4_ct MathlibNEON::FLOAT_MIN = init_list_4( std::numeric_limits<Real>::min() );
    const float32x4_ct MathlibNEON::SIGN_MASK = init_list_4( -0.0f );
    const float32x4_ct MathlibNEON::INFINITEA = init_list_4( std::numeric_limits<Real>::infinity() );
    const float32x4_ct MathlibNEON::MAX_NEG = init_list_4( -std::numeric_limits<Real>::max() );
    const float32x4_ct MathlibNEON::MAX_POS = init_list_4( std::numeric_limits<Real>::max() );
    const float32x4_ct MathlibNEON::LAST_AFFINE_COLUMN = { 0, 0, 0, 1 };

    static const Real _PI = Real( 3.14159265358979323846264338327950288 );
    // We can't use Math::fDeg2Rad & Math::fRad2Deg directly because
    // it's not guaranteed to have been initialized first
    const float32x4_ct MathlibNEON::PI = init_list_4( _PI );
    const float32x4_ct MathlibNEON::TWO_PI = init_list_4( 2.0f * _PI );
    const float32x4_ct MathlibNEON::fDeg2Rad = init_list_4( _PI / 180.0f );
    const float32x4_ct MathlibNEON::fRad2Deg = init_list_4( 180.0f / _PI );

    const float32x4_ct MathlibNEON::ONE_DIV_2PI = init_list_4( 1.0f / ( 2.0f * _PI ) );
#    undef init_list_4

    //-----------------------------------------------------------------------------------
    ArrayReal MathlibNEON::Sin4( ArrayReal x )
    {
        // Map arbitrary angle x to the range [-pi; +pi] without using division.
        // Code taken from MSDN's HLSL trick. Architectures with fused mad (i.e. NEON)
        // can replace the add, the sub, & the two muls for two mad
        ArrayReal integralPart;
        x = vaddq_f32( vmulq_f32( x, ONE_DIV_2PI ), HALF );
        x = Modf4( x, integralPart );
        x = vsubq_f32( vmulq_f32( x, TWO_PI ), PI );

        return sin_ps( x );
    }
    //-----------------------------------------------------------------------------------
    ArrayReal MathlibNEON::Cos4( ArrayReal x )
    {
        // Map arbitrary angle x to the range [-pi; +pi] without using division.
        // Code taken from MSDN's HLSL trick. Architectures with fused mad (i.e. NEON)
        // can replace the add, the sub, & the two muls for two mad
        ArrayReal integralPart;
        x = vaddq_f32( vmulq_f32( x, ONE_DIV_2PI ), HALF );
        x = Modf4( x, integralPart );
        x = vsubq_f32( vmulq_f32( x, TWO_PI ), PI );

        return cos_ps( x );
    }
    //-----------------------------------------------------------------------------------
    void MathlibNEON::SinCos4( ArrayReal x, ArrayReal &outSin, ArrayReal &outCos )
    {
        // TODO: Improve accuracy by mapping to the range [-pi/4, pi/4] and swap
        // between cos & sin depending on which quadrant it fell:
        // Quadrant | sin     |  cos
        // n = 0 ->  sin( x ),  cos( x )
        // n = 1 ->  cos( x ), -sin( x )
        // n = 2 -> -sin( x ), -cos( x )
        // n = 3 -> -sin( x ),  sin( x )
        // See ARGUMENT REDUCTION FOR HUGE ARGUMENTS:
        // Good to the Last Bit
        // K. C. Ng and themembers of the FP group of SunPro
        // http://www.derekroconnor.net/Software/Ng--ArgReduction.pdf

        // -- Perhaps we can leave this to GSoC students? --

        // Map arbitrary angle x to the range [-pi; +pi] without using division.
        // Code taken from MSDN's HLSL trick. Architectures with fused mad (i.e. NEON)
        // can replace the add, the sub, & the two muls for two mad
        ArrayReal integralPart;
        x = vaddq_f32( vmulq_f32( x, ONE_DIV_2PI ), HALF );
        x = Modf4( x, integralPart );
        x = vsubq_f32( vmulq_f32( x, TWO_PI ), PI );

        sincos_ps( x, &outSin, &outCos );
    }

    const uint32x4_ct BooleanMask4::mMasks[NUM_MASKS] = {
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },  // MASK_NONE
        { 0xffffffff, 0x00000000, 0x00000000, 0x00000000 },  // MASK_X
        { 0x00000000, 0xffffffff, 0x00000000, 0x00000000 },  // MASK_Y
        { 0xffffffff, 0xffffffff, 0x00000000, 0x00000000 },  // MASK_XY
        { 0x00000000, 0x00000000, 0xffffffff, 0x00000000 },  // MASK_Z
        { 0xffffffff, 0x00000000, 0xffffffff, 0x00000000 },  // MASK_XZ
        { 0x00000000, 0xffffffff, 0xffffffff, 0x00000000 },  // MASK_YZ
        { 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000 },  // MASK_XYZ
        { 0x00000000, 0x00000000, 0x00000000, 0xffffffff },  // MASK_W
        { 0xffffffff, 0x00000000, 0x00000000, 0xffffffff },  // MASK_XW
        { 0x00000000, 0xffffffff, 0x00000000, 0xffffffff },  // MASK_YW
        { 0xffffffff, 0xffffffff, 0x00000000, 0xffffffff },  // MASK_XYW
        { 0x00000000, 0x00000000, 0xffffffff, 0xffffffff },  // MASK_ZW
        { 0xffffffff, 0x00000000, 0xffffffff, 0xffffffff },  // MASK_XZW
        { 0x00000000, 0xffffffff, 0xffffffff, 0xffffffff },  // MASK_YZW
        { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }   // MASK_XYZW
    };
}  // namespace Ogre
#endif
