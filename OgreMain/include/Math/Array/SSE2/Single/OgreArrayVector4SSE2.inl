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
        DEFINE_OPERATION( ArrayVector4, ArrayVector4, +, _mm_add_ps );
        Means: "define '+' operator that takes both arguments as ArrayVector4 and use
        the _mm_add_ps intrinsic to do the job"

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
    // clang-format off
    // Arithmetic operations
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
        ArrayReal lhs = _mm_set1_ps( fScalar );\
        return ArrayVector4(\
                op_func( lhs, rhs.mChunkBase[0] ),\
                op_func( lhs, rhs.mChunkBase[1] ),\
                op_func( lhs, rhs.mChunkBase[2] ),\
                op_func( lhs, rhs.mChunkBase[3] ) );\
    }
#define DEFINE_R_SCALAR_OPERATION( leftClass, rightType, op, op_func )\
    inline ArrayVector4 operator op ( const leftClass &lhs, const rightType fScalar )\
    {\
        ArrayReal rhs = _mm_set1_ps( fScalar );\
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
        ArrayReal lhs = _mm_set1_ps( fScalar );\
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
        ArrayReal rhs = _mm_set1_ps( fInv );\
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
                assert( _mm_movemask_ps( _mm_cmpeq_ps( values, _mm_setzero_ps() ) ) == 0 &&\
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
        ArrayReal rhs = MathlibSSE2::Inv4( r );\
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
        ArrayReal a = _mm_set1_ps( fScalar );\
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
        ArrayReal a = _mm_set1_ps( fInv );\
        mChunkBase[0] = op_func( mChunkBase[0], a );\
        mChunkBase[1] = op_func( mChunkBase[1], a );\
        mChunkBase[2] = op_func( mChunkBase[2], a );\
        mChunkBase[3] = op_func( mChunkBase[3], a );\
    }
#define DEFINE_UPDATE_R_DIVISION( rightType, op, op_func )\
    inline void ArrayVector4::operator op ( const rightType _a )\
    {\
        ASSERT_DIV_BY_ZERO( _a );\
        ArrayReal a = MathlibSSE2::Inv4( _a );\
        mChunkBase[0] = op_func( mChunkBase[0], a );\
        mChunkBase[1] = op_func( mChunkBase[1], a );\
        mChunkBase[2] = op_func( mChunkBase[2], a );\
        mChunkBase[3] = op_func( mChunkBase[3], a );\
    }

    inline const ArrayVector4& ArrayVector4::operator + () const
    {
        return *this;
    };
    //-------------------------------------------------------------------------
    inline ArrayVector4 ArrayVector4::operator - () const
    {
        return ArrayVector4(
            _mm_xor_ps( mChunkBase[0], MathlibSSE2::SIGN_MASK ),    //-x
            _mm_xor_ps( mChunkBase[1], MathlibSSE2::SIGN_MASK ),    //-y
            _mm_xor_ps( mChunkBase[2], MathlibSSE2::SIGN_MASK ),    //-z
            _mm_xor_ps( mChunkBase[3], MathlibSSE2::SIGN_MASK ));   //-w
    }
    //-------------------------------------------------------------------------

    // + Addition
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, +, _mm_add_ps );
    DEFINE_L_SCALAR_OPERATION( Real, ArrayVector4, +, _mm_add_ps );
    DEFINE_R_SCALAR_OPERATION( ArrayVector4, Real, +, _mm_add_ps );

    DEFINE_L_OPERATION( ArrayReal, ArrayVector4, +, _mm_add_ps );
    DEFINE_R_OPERATION( ArrayVector4, ArrayReal, +, _mm_add_ps );

    // - Subtraction
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, -, _mm_sub_ps );
    DEFINE_L_SCALAR_OPERATION( Real, ArrayVector4, -, _mm_sub_ps );
    DEFINE_R_SCALAR_OPERATION( ArrayVector4, Real, -, _mm_sub_ps );

    DEFINE_L_OPERATION( ArrayReal, ArrayVector4, -, _mm_sub_ps );
    DEFINE_R_OPERATION( ArrayVector4, ArrayReal, -, _mm_sub_ps );

    // * Multiplication
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, *, _mm_mul_ps );
    DEFINE_L_SCALAR_OPERATION( Real, ArrayVector4, *, _mm_mul_ps );
    DEFINE_R_SCALAR_OPERATION( ArrayVector4, Real, *, _mm_mul_ps );

    DEFINE_L_OPERATION( ArrayReal, ArrayVector4, *, _mm_mul_ps );
    DEFINE_R_OPERATION( ArrayVector4, ArrayReal, *, _mm_mul_ps );

    // / Division (scalar versions use mul instead of div, because they mul against the reciprocal)
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, /, _mm_div_ps );
    DEFINE_L_SCALAR_DIVISION( Real, ArrayVector4, /, _mm_div_ps );
    DEFINE_R_SCALAR_DIVISION( ArrayVector4, Real, /, _mm_mul_ps );

    DEFINE_L_DIVISION( ArrayReal, ArrayVector4, /, _mm_div_ps );
    DEFINE_R_DIVISION( ArrayVector4, ArrayReal, /, _mm_mul_ps );

    inline ArrayVector4 ArrayVector4::Cmov4( const ArrayVector4 &arg1, const ArrayVector4 &arg2, ArrayReal mask )
    {
        return ArrayVector4(
                MathlibSSE2::Cmov4( arg1.mChunkBase[0], arg2.mChunkBase[0], mask ),
                MathlibSSE2::Cmov4( arg1.mChunkBase[1], arg2.mChunkBase[1], mask ),
                MathlibSSE2::Cmov4( arg1.mChunkBase[2], arg2.mChunkBase[2], mask ),
                MathlibSSE2::Cmov4( arg1.mChunkBase[3], arg2.mChunkBase[3], mask ) );
    }

    // Update operations
    // +=
    DEFINE_UPDATE_OPERATION(            ArrayVector4,       +=, _mm_add_ps );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               +=, _mm_add_ps );
    DEFINE_UPDATE_R_OPERATION(          ArrayReal,          +=, _mm_add_ps );

    // -=
    DEFINE_UPDATE_OPERATION(            ArrayVector4,       -=, _mm_sub_ps );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               -=, _mm_sub_ps );
    DEFINE_UPDATE_R_OPERATION(          ArrayReal,          -=, _mm_sub_ps );

    // *=
    DEFINE_UPDATE_OPERATION(            ArrayVector4,       *=, _mm_mul_ps );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               *=, _mm_mul_ps );
    DEFINE_UPDATE_R_OPERATION(          ArrayReal,          *=, _mm_mul_ps );

    // /=
    DEFINE_UPDATE_DIVISION(             ArrayVector4,       /=, _mm_div_ps );
    DEFINE_UPDATE_R_SCALAR_DIVISION(    Real,               /=, _mm_mul_ps );
    DEFINE_UPDATE_R_DIVISION(           ArrayReal,          /=, _mm_mul_ps );
    // clang-format on

    // Functions
    //-------------------------------------------------------------------------
    inline void ArrayVector4::fma3x1( const ArrayReal m, const ArrayReal a,  //
                                      const ArrayReal m2, const ArrayReal a2 )
    {
        mChunkBase[0] = _mm_madd_ps( mChunkBase[0], m, a );
        mChunkBase[1] = _mm_madd_ps( mChunkBase[1], m, a );
        mChunkBase[2] = _mm_madd_ps( mChunkBase[2], m, a );

        mChunkBase[3] = _mm_madd_ps( mChunkBase[3], m2, a2 );
    }
    //-------------------------------------------------------------------------
    inline void ArrayVector4::makeFloor( const ArrayVector4 &cmp )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = cmp.mChunkBase;
        aChunkBase[0] = _mm_min_ps( aChunkBase[0], bChunkBase[0] );
        aChunkBase[1] = _mm_min_ps( aChunkBase[1], bChunkBase[1] );
        aChunkBase[2] = _mm_min_ps( aChunkBase[2], bChunkBase[2] );
        aChunkBase[3] = _mm_min_ps( aChunkBase[3], bChunkBase[3] );
    }
    //-------------------------------------------------------------------------
    inline void ArrayVector4::makeCeil( const ArrayVector4 &cmp )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = cmp.mChunkBase;
        aChunkBase[0] = _mm_max_ps( aChunkBase[0], bChunkBase[0] );
        aChunkBase[1] = _mm_max_ps( aChunkBase[1], bChunkBase[1] );
        aChunkBase[2] = _mm_max_ps( aChunkBase[2], bChunkBase[2] );
        aChunkBase[3] = _mm_max_ps( aChunkBase[3], bChunkBase[3] );
    }
    //-------------------------------------------------------------------------
    inline void ArrayVector4::inverseLeaveZeroes()
    {
        // Use InvNonZero, we're gonna nuke the NaNs anyway.
        mChunkBase[0] =
            MathlibSSE2::CmovRobust( mChunkBase[0], MathlibSSE2::InvNonZero4( mChunkBase[0] ),
                                     _mm_cmpeq_ps( mChunkBase[0], _mm_setzero_ps() ) );
        mChunkBase[1] =
            MathlibSSE2::CmovRobust( mChunkBase[1], MathlibSSE2::InvNonZero4( mChunkBase[1] ),
                                     _mm_cmpeq_ps( mChunkBase[1], _mm_setzero_ps() ) );
        mChunkBase[2] =
            MathlibSSE2::CmovRobust( mChunkBase[2], MathlibSSE2::InvNonZero4( mChunkBase[2] ),
                                     _mm_cmpeq_ps( mChunkBase[2], _mm_setzero_ps() ) );
        mChunkBase[3] =
            MathlibSSE2::CmovRobust( mChunkBase[3], MathlibSSE2::InvNonZero4( mChunkBase[3] ),
                                     _mm_cmpeq_ps( mChunkBase[3], _mm_setzero_ps() ) );
    }
    //-------------------------------------------------------------------------
    inline int ArrayVector4::isNaN() const
    {
        ArrayReal mask = _mm_and_ps( _mm_and_ps( _mm_cmpeq_ps( mChunkBase[0], mChunkBase[0] ),
                                                 _mm_cmpeq_ps( mChunkBase[1], mChunkBase[1] ) ),
                                     _mm_and_ps( _mm_cmpeq_ps( mChunkBase[2], mChunkBase[2] ),
                                                 _mm_cmpeq_ps( mChunkBase[3], mChunkBase[3] ) ) );

        return _mm_movemask_ps( mask ) ^ 0x0000000f;
    }
    //-------------------------------------------------------------------------
    inline void ArrayVector4::Cmov4( ArrayReal mask, const ArrayVector4 &replacement )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = replacement.mChunkBase;
        aChunkBase[0] = MathlibSSE2::Cmov4( aChunkBase[0], bChunkBase[0], mask );
        aChunkBase[1] = MathlibSSE2::Cmov4( aChunkBase[1], bChunkBase[1], mask );
        aChunkBase[2] = MathlibSSE2::Cmov4( aChunkBase[2], bChunkBase[2], mask );
        aChunkBase[3] = MathlibSSE2::Cmov4( aChunkBase[3], bChunkBase[3], mask );
    }
    //-------------------------------------------------------------------------
    inline void ArrayVector4::CmovRobust( ArrayReal mask, const ArrayVector4 &replacement )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = replacement.mChunkBase;
        aChunkBase[0] = MathlibSSE2::CmovRobust( aChunkBase[0], bChunkBase[0], mask );
        aChunkBase[1] = MathlibSSE2::CmovRobust( aChunkBase[1], bChunkBase[1], mask );
        aChunkBase[2] = MathlibSSE2::CmovRobust( aChunkBase[2], bChunkBase[2], mask );
        aChunkBase[3] = MathlibSSE2::CmovRobust( aChunkBase[3], bChunkBase[3], mask );
    }
    //-------------------------------------------------------------------------

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
