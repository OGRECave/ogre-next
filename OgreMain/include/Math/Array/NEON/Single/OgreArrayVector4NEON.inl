/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2023 Torus Knot Software Ltd

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

namespace Ogre
{
    /** HOW THIS WORKS:

        Instead of writing like 12 times the same code, we use macros:
        DEFINE_OPERATION( ArrayVector4, ArrayVector4, +, vaddq_f32 );
        Means: "define '+' operator that takes both arguments as ArrayVector4 and use
        the vaddq_f32 intrinsic to do the job"

        Note that for scalars (i.e. floats) we use DEFINE_L_SCALAR_OPERATION/DEFINE_R_SCALAR_OPERATION
        depending on whether the scalar is on the left or right side of the operation
        (i.e. 2 * a vs a * 2)
        And for ArrayReal scalars we use DEFINE_L_OPERATION/DEFINE_R_OPERATION

        As for division, we use specific scalar versions to increase performance (calculate
        the inverse of the scalar once, then multiply) as well as placing asserts in
        case of trying to divide by zero.

        I've considered using templates, but decided against it because wrong operator usage
        would raise cryptic compile error messages, and templates inability to limit which
        types are being used, leave the slight possibility of mixing completely unrelated
        types (i.e. ArrayQuaternion + ArrayVector4) and quietly compiling wrong code.
        Furthermore some platforms may not have decent compilers handling templates for
        major operator usage.

        Advantages:
            * Increased readability
            * Ease of reading & understanding
        Disadvantages:
            * Debugger can't go inside the operator. See workaround.

        As a workaround to the disadvantage, you can compile this code using cl.exe /EP /P /C to
        generate this file with macros replaced as actual code (very handy!)
    */
    // Arithmetic operations
    // clang-format off
#define DEFINE_OPERATION( leftClass, rightClass, op, op_func )\
    inline ArrayVector4 operator op ( const leftClass &lhs, const rightClass &rhs )\
    {\
        const ArrayReal * RESTRICT_ALIAS lhsChunkBase = lhs.mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS rhsChunkBase = rhs.mChunkBase;\
        return ArrayVector4(\
                op_func( lhsChunkBase[0], rhsChunkBase[0] ),\
                op_func( lhsChunkBase[1], rhsChunkBase[1] ),\
                op_func( lhsChunkBase[2], rhsChunkBase[2] ),\
                op_func( lhsChunkBase[3], rhsChunkBase[3] ) );\
       }
#define DEFINE_L_SCALAR_OPERATION( leftType, rightClass, op, op_func )\
    inline ArrayVector4 operator op ( const leftType fScalar, const rightClass &rhs )\
    {\
        ArrayReal lhs = vdupq_n_f32( fScalar );\
        return ArrayVector4(\
                op_func( lhs, rhs.mChunkBase[0] ),\
                op_func( lhs, rhs.mChunkBase[1] ),\
                op_func( lhs, rhs.mChunkBase[2] ),\
                op_func( lhs, rhs.mChunkBase[3] ) );\
    }
#define DEFINE_R_SCALAR_OPERATION( leftClass, rightType, op, op_func )\
    inline ArrayVector4 operator op ( const leftClass &lhs, const rightType fScalar )\
    {\
        ArrayReal rhs = vdupq_n_f32( fScalar );\
        return ArrayVector4(\
                op_func( lhs.mChunkBase[0], rhs ),\
                op_func( lhs.mChunkBase[1], rhs ),\
                op_func( lhs.mChunkBase[2], rhs ),\
                op_func( lhs.mChunkBase[3], rhs ) );\
    }
#define DEFINE_L_OPERATION( leftType, rightClass, op, op_func )\
    inline ArrayVector4 operator op ( const leftType lhs, const rightClass &rhs )\
    {\
        return ArrayVector4(\
                op_func( lhs, rhs.mChunkBase[0] ),\
                op_func( lhs, rhs.mChunkBase[1] ),\
                op_func( lhs, rhs.mChunkBase[2] ),\
                op_func( lhs, rhs.mChunkBase[3] ) );\
    }
#define DEFINE_R_OPERATION( leftClass, rightType, op, op_func )\
    inline ArrayVector4 operator op ( const leftClass &lhs, const rightType rhs )\
    {\
        return ArrayVector4(\
                op_func( lhs.mChunkBase[0], rhs ),\
                op_func( lhs.mChunkBase[1], rhs ),\
                op_func( lhs.mChunkBase[2], rhs ),\
                op_func( lhs.mChunkBase[3], rhs ) );\
    }

#define DEFINE_L_SCALAR_DIVISION( leftType, rightClass, op, op_func )\
    inline ArrayVector4 operator op ( const leftType fScalar, const rightClass &rhs )\
    {\
        ArrayReal lhs = vdupq_n_f32( fScalar );\
        return ArrayVector4(\
                op_func( lhs, rhs.mChunkBase[0] ),\
                op_func( lhs, rhs.mChunkBase[1] ),\
                op_func( lhs, rhs.mChunkBase[2] ),\
                op_func( lhs, rhs.mChunkBase[3] ) );\
    }
#define DEFINE_R_SCALAR_DIVISION( leftClass, rightType, op, op_func )\
    inline ArrayVector4 operator op ( const leftClass &lhs, const rightType fScalar )\
    {\
        assert( fScalar != 0.0 );\
        Real fInv = 1.0f / fScalar;\
        ArrayReal rhs = vdupq_n_f32( fInv );\
        return ArrayVector4(\
                op_func( lhs.mChunkBase[0], rhs ),\
                op_func( lhs.mChunkBase[1], rhs ),\
                op_func( lhs.mChunkBase[2], rhs ),\
                op_func( lhs.mChunkBase[3], rhs ) );\
    }

#ifdef NDEBUG
    #define ASSERT_DIV_BY_ZERO( values ) ((void)0)
#else
    #define ASSERT_DIV_BY_ZERO( values ) {\
                assert( vmovemaskq_u32( vceqq_f32( values, vdupq_n_f32(0.0f) ) ) == 0 &&\
                "One of the 4 floats is a zero. Can't divide by zero" ); }
#endif

#define DEFINE_L_DIVISION( leftType, rightClass, op, op_func )\
    inline ArrayVector4 operator op ( const leftType lhs, const rightClass &rhs )\
    {\
        return ArrayVector4(\
                op_func( lhs, rhs.mChunkBase[0] ),\
                op_func( lhs, rhs.mChunkBase[1] ),\
                op_func( lhs, rhs.mChunkBase[2] ),\
                op_func( lhs, rhs.mChunkBase[3] ) );\
    }
#define DEFINE_R_DIVISION( leftClass, rightType, op, op_func )\
    inline ArrayVector4 operator op ( const leftClass &lhs, const rightType r )\
    {\
        ASSERT_DIV_BY_ZERO( r );\
        ArrayReal rhs = MathlibNEON::Inv4( r );\
        return ArrayVector4(\
                op_func( lhs.mChunkBase[0], rhs ),\
                op_func( lhs.mChunkBase[1], rhs ),\
                op_func( lhs.mChunkBase[2], rhs ),\
                op_func( lhs.mChunkBase[3], rhs ) );\
    }

    // Update operations
#define DEFINE_UPDATE_OPERATION( leftClass, op, op_func )\
    inline void ArrayVector4::operator op ( const leftClass &a )\
    {\
        ArrayReal * RESTRICT_ALIAS chunkBase = mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS aChunkBase = a.mChunkBase;\
        chunkBase[0] = op_func( chunkBase[0], aChunkBase[0] );\
        chunkBase[1] = op_func( chunkBase[1], aChunkBase[1] );\
        chunkBase[2] = op_func( chunkBase[2], aChunkBase[2] );\
        chunkBase[3] = op_func( chunkBase[3], aChunkBase[3] );\
    }
#define DEFINE_UPDATE_R_SCALAR_OPERATION( rightType, op, op_func )\
    inline void ArrayVector4::operator op ( const rightType fScalar )\
    {\
        ArrayReal a = vdupq_n_f32( fScalar );\
        mChunkBase[0] = op_func( mChunkBase[0], a );\
        mChunkBase[1] = op_func( mChunkBase[1], a );\
        mChunkBase[2] = op_func( mChunkBase[2], a );\
        mChunkBase[3] = op_func( mChunkBase[3], a );\
    }
#define DEFINE_UPDATE_R_OPERATION( rightType, op, op_func )\
    inline void ArrayVector4::operator op ( const rightType a )\
    {\
        mChunkBase[0] = op_func( mChunkBase[0], a );\
        mChunkBase[1] = op_func( mChunkBase[1], a );\
        mChunkBase[2] = op_func( mChunkBase[2], a );\
        mChunkBase[3] = op_func( mChunkBase[3], a );\
    }
#define DEFINE_UPDATE_DIVISION( leftClass, op, op_func )\
    inline void ArrayVector4::operator op ( const leftClass &a )\
    {\
        ArrayReal * RESTRICT_ALIAS chunkBase = mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS aChunkBase = a.mChunkBase;\
        chunkBase[0] = op_func( chunkBase[0], aChunkBase[0] );\
        chunkBase[1] = op_func( chunkBase[1], aChunkBase[1] );\
        chunkBase[2] = op_func( chunkBase[2], aChunkBase[2] );\
        chunkBase[3] = op_func( chunkBase[3], aChunkBase[3] );\
    }
#define DEFINE_UPDATE_R_SCALAR_DIVISION( rightType, op, op_func )\
    inline void ArrayVector4::operator op ( const rightType fScalar )\
    {\
        assert( fScalar != 0.0 );\
        Real fInv = 1.0f / fScalar;\
        ArrayReal a = vdupq_n_f32( fInv );\
        mChunkBase[0] = op_func( mChunkBase[0], a );\
        mChunkBase[1] = op_func( mChunkBase[1], a );\
        mChunkBase[2] = op_func( mChunkBase[2], a );\
        mChunkBase[3] = op_func( mChunkBase[3], a );\
    }
#define DEFINE_UPDATE_R_DIVISION( rightType, op, op_func )\
    inline void ArrayVector4::operator op ( const rightType _a )\
    {\
        ASSERT_DIV_BY_ZERO( _a );\
        ArrayReal a = MathlibNEON::Inv4( _a );\
        mChunkBase[0] = op_func( mChunkBase[0], a );\
        mChunkBase[1] = op_func( mChunkBase[1], a );\
        mChunkBase[2] = op_func( mChunkBase[2], a );\
        mChunkBase[3] = op_func( mChunkBase[3], a );\
    }

    inline const ArrayVector4& ArrayVector4::operator + () const
    {
        return *this;
    };
    //-----------------------------------------------------------------------------------
    inline ArrayVector4 ArrayVector4::operator - () const
    {
        return ArrayVector4( veorq_f32( mChunkBase[0], MathlibNEON::SIGN_MASK ),    //-x
                             veorq_f32( mChunkBase[1], MathlibNEON::SIGN_MASK ),    //-y
                             veorq_f32( mChunkBase[2], MathlibNEON::SIGN_MASK ),    //-z
                             veorq_f32( mChunkBase[3], MathlibNEON::SIGN_MASK ) );  //-w
    }
    //-----------------------------------------------------------------------------------

    // + Addition
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, +, vaddq_f32 );
    DEFINE_L_SCALAR_OPERATION( Real, ArrayVector4, +, vaddq_f32 );
    DEFINE_R_SCALAR_OPERATION( ArrayVector4, Real, +, vaddq_f32 );

    DEFINE_L_OPERATION( ArrayReal, ArrayVector4, +, vaddq_f32 );
    DEFINE_R_OPERATION( ArrayVector4, ArrayReal, +, vaddq_f32 );

    // - Subtraction
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, -, vsubq_f32 );
    DEFINE_L_SCALAR_OPERATION( Real, ArrayVector4, -, vsubq_f32 );
    DEFINE_R_SCALAR_OPERATION( ArrayVector4, Real, -, vsubq_f32 );

    DEFINE_L_OPERATION( ArrayReal, ArrayVector4, -, vsubq_f32 );
    DEFINE_R_OPERATION( ArrayVector4, ArrayReal, -, vsubq_f32 );

    // * Multiplication
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, *, vmulq_f32 );
    DEFINE_L_SCALAR_OPERATION( Real, ArrayVector4, *, vmulq_f32 );
    DEFINE_R_SCALAR_OPERATION( ArrayVector4, Real, *, vmulq_f32 );

    DEFINE_L_OPERATION( ArrayReal, ArrayVector4, *, vmulq_f32 );
    DEFINE_R_OPERATION( ArrayVector4, ArrayReal, *, vmulq_f32 );

    // / Division (scalar versions use mul instead of div, because they mul against the reciprocal)
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, /, vdivq_f32 );
    DEFINE_L_SCALAR_DIVISION( Real, ArrayVector4, /, vdivq_f32 );
    DEFINE_R_SCALAR_DIVISION( ArrayVector4, Real, /, vmulq_f32 );

    DEFINE_L_DIVISION( ArrayReal, ArrayVector4, /, vdivq_f32 );
    DEFINE_R_DIVISION( ArrayVector4, ArrayReal, /, vmulq_f32 );

    inline ArrayVector4 ArrayVector4::Cmov4( const ArrayVector4 &arg1, const ArrayVector4 &arg2, ArrayMaskR mask )
    {
        return ArrayVector4(
                MathlibNEON::Cmov4( arg1.mChunkBase[0], arg2.mChunkBase[0], mask ),
                MathlibNEON::Cmov4( arg1.mChunkBase[1], arg2.mChunkBase[1], mask ),
                MathlibNEON::Cmov4( arg1.mChunkBase[2], arg2.mChunkBase[2], mask ),
                MathlibNEON::Cmov4( arg1.mChunkBase[3], arg2.mChunkBase[3], mask ) );
    }

    // Update operations
    // +=
    DEFINE_UPDATE_OPERATION(            ArrayVector4,       +=, vaddq_f32 );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               +=, vaddq_f32 );
    DEFINE_UPDATE_R_OPERATION(          ArrayReal,          +=, vaddq_f32 );

    // -=
    DEFINE_UPDATE_OPERATION(            ArrayVector4,       -=, vsubq_f32 );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               -=, vsubq_f32 );
    DEFINE_UPDATE_R_OPERATION(          ArrayReal,          -=, vsubq_f32 );

    // *=
    DEFINE_UPDATE_OPERATION(            ArrayVector4,       *=, vmulq_f32 );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               *=, vmulq_f32 );
    DEFINE_UPDATE_R_OPERATION(          ArrayReal,          *=, vmulq_f32 );

    // /=
    DEFINE_UPDATE_DIVISION(             ArrayVector4,       /=, vdivq_f32 );
    DEFINE_UPDATE_R_SCALAR_DIVISION(    Real,               /=, vmulq_f32 );
    DEFINE_UPDATE_R_DIVISION(           ArrayReal,          /=, vmulq_f32 );
    // clang-format on

    // Functions
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::fma3x1( const ArrayReal m, const ArrayReal a,  //
                                      const ArrayReal m2, const ArrayReal a2 )
    {
        mChunkBase[0] = _mm_madd_ps( mChunkBase[0], m, a );
        mChunkBase[1] = _mm_madd_ps( mChunkBase[1], m, a );
        mChunkBase[2] = _mm_madd_ps( mChunkBase[2], m, a );

        mChunkBase[3] = _mm_madd_ps( mChunkBase[3], m2, a2 );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::makeFloor( const ArrayVector4 &cmp )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = cmp.mChunkBase;
        aChunkBase[0] = vminq_f32( aChunkBase[0], bChunkBase[0] );
        aChunkBase[1] = vminq_f32( aChunkBase[1], bChunkBase[1] );
        aChunkBase[2] = vminq_f32( aChunkBase[2], bChunkBase[2] );
        aChunkBase[3] = vminq_f32( aChunkBase[3], bChunkBase[3] );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::makeCeil( const ArrayVector4 &cmp )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = cmp.mChunkBase;
        aChunkBase[0] = vmaxq_f32( aChunkBase[0], bChunkBase[0] );
        aChunkBase[1] = vmaxq_f32( aChunkBase[1], bChunkBase[1] );
        aChunkBase[2] = vmaxq_f32( aChunkBase[2], bChunkBase[2] );
        aChunkBase[3] = vmaxq_f32( aChunkBase[3], bChunkBase[3] );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::inverseLeaveZeroes()
    {
        // Use InvNonZero, we're gonna nuke the NaNs anyway.
        mChunkBase[0] =
            MathlibNEON::CmovRobust( mChunkBase[0], MathlibNEON::InvNonZero4( mChunkBase[0] ),
                                     vceqq_f32( mChunkBase[0], vdupq_n_f32( 0.0f ) ) );
        mChunkBase[1] =
            MathlibNEON::CmovRobust( mChunkBase[1], MathlibNEON::InvNonZero4( mChunkBase[1] ),
                                     vceqq_f32( mChunkBase[1], vdupq_n_f32( 0.0f ) ) );
        mChunkBase[2] =
            MathlibNEON::CmovRobust( mChunkBase[2], MathlibNEON::InvNonZero4( mChunkBase[2] ),
                                     vceqq_f32( mChunkBase[2], vdupq_n_f32( 0.0f ) ) );
        mChunkBase[3] =
            MathlibNEON::CmovRobust( mChunkBase[3], MathlibNEON::InvNonZero4( mChunkBase[3] ),
                                     vceqq_f32( mChunkBase[3], vdupq_n_f32( 0.0f ) ) );
    }
    //-----------------------------------------------------------------------------------
    inline int ArrayVector4::isNaN() const
    {
        ArrayMaskR mask = vandq_u32( vandq_u32( vceqq_f32( mChunkBase[0], mChunkBase[0] ),
                                                vceqq_f32( mChunkBase[1], mChunkBase[1] ) ),
                                     vandq_u32( vceqq_f32( mChunkBase[2], mChunkBase[2] ),
                                                vceqq_f32( mChunkBase[3], mChunkBase[3] ) ) );

        return int( vmovemaskq_u32( mask ) ^ 0x0000000f );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::Cmov4( ArrayMaskR mask, const ArrayVector4 &replacement )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = replacement.mChunkBase;
        aChunkBase[0] = MathlibNEON::Cmov4( aChunkBase[0], bChunkBase[0], mask );
        aChunkBase[1] = MathlibNEON::Cmov4( aChunkBase[1], bChunkBase[1], mask );
        aChunkBase[2] = MathlibNEON::Cmov4( aChunkBase[2], bChunkBase[2], mask );
        aChunkBase[3] = MathlibNEON::Cmov4( aChunkBase[3], bChunkBase[3], mask );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::CmovRobust( ArrayMaskR mask, const ArrayVector4 &replacement )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = replacement.mChunkBase;
        aChunkBase[0] = MathlibNEON::CmovRobust( aChunkBase[0], bChunkBase[0], mask );
        aChunkBase[1] = MathlibNEON::CmovRobust( aChunkBase[1], bChunkBase[1], mask );
        aChunkBase[2] = MathlibNEON::CmovRobust( aChunkBase[2], bChunkBase[2], mask );
        aChunkBase[3] = MathlibNEON::CmovRobust( aChunkBase[3], bChunkBase[3], mask );
    }
    //-----------------------------------------------------------------------------------

#undef DEFINE_OPERATION
#undef DEFINE_L_SCALAR_OPERATION
#undef DEFINE_R_SCALAR_OPERATION
#undef DEFINE_L_OPERATION
#undef DEFINE_R_OPERATION
#undef DEFINE_DIVISION
#undef DEFINE_L_DIVISION
#undef DEFINE_R_SCALAR_DIVISION
#undef DEFINE_L_SCALAR_DIVISION
#undef DEFINE_R_DIVISION

#undef DEFINE_UPDATE_OPERATION
#undef DEFINE_UPDATE_R_SCALAR_OPERATION
#undef DEFINE_UPDATE_R_OPERATION
#undef DEFINE_UPDATE_DIVISION
#undef DEFINE_UPDATE_R_SCALAR_DIVISION
#undef DEFINE_UPDATE_R_DIVISION
}  // namespace Ogre
