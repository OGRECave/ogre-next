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
        DEFINE_OPERATION( ArrayVector2, ArrayVector2, +, vaddq_f32 );
        Means: "define '+' operator that takes both arguments as ArrayVector2 and use
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
    // clang-format off
    // Arithmetic operations
#define DEFINE_OPERATION( leftClass, rightClass, op, op_func )\
    inline ArrayVector2 operator op ( const leftClass &lhs, const rightClass &rhs )\
    {\
        const ArrayReal * RESTRICT_ALIAS lhsChunkBase = lhs.mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS rhsChunkBase = rhs.mChunkBase;\
        return ArrayVector2(\
                op_func( lhsChunkBase[0], rhsChunkBase[0] ),\
                op_func( lhsChunkBase[1], rhsChunkBase[1] ) );\
       }
#define DEFINE_L_SCALAR_OPERATION( leftType, rightClass, op, op_func )\
    inline ArrayVector2 operator op ( const leftType fScalar, const rightClass &rhs )\
    {\
        ArrayReal lhs = vdupq_n_f32( fScalar );\
        return ArrayVector2(\
                op_func( lhs, rhs.mChunkBase[0] ),\
                op_func( lhs, rhs.mChunkBase[1] ) );\
    }
#define DEFINE_R_SCALAR_OPERATION( leftClass, rightType, op, op_func )\
    inline ArrayVector2 operator op ( const leftClass &lhs, const rightType fScalar )\
    {\
        ArrayReal rhs = vdupq_n_f32( fScalar );\
        return ArrayVector2(\
                op_func( lhs.mChunkBase[0], rhs ),\
                op_func( lhs.mChunkBase[1], rhs ) );\
    }
#define DEFINE_L_OPERATION( leftType, rightClass, op, op_func )\
    inline ArrayVector2 operator op ( const leftType lhs, const rightClass &rhs )\
    {\
        return ArrayVector2(\
                op_func( lhs, rhs.mChunkBase[0] ),\
                op_func( lhs, rhs.mChunkBase[1] ) );\
    }
#define DEFINE_R_OPERATION( leftClass, rightType, op, op_func )\
    inline ArrayVector2 operator op ( const leftClass &lhs, const rightType rhs )\
    {\
        return ArrayVector2(\
                op_func( lhs.mChunkBase[0], rhs ),\
                op_func( lhs.mChunkBase[1], rhs ) );\
    }

#define DEFINE_L_SCALAR_DIVISION( leftType, rightClass, op, op_func )\
    inline ArrayVector2 operator op ( const leftType fScalar, const rightClass &rhs )\
    {\
        ArrayReal lhs = vdupq_n_f32( fScalar );\
        return ArrayVector2(\
                op_func( lhs, rhs.mChunkBase[0] ),\
                op_func( lhs, rhs.mChunkBase[1] ) );\
    }
#define DEFINE_R_SCALAR_DIVISION( leftClass, rightType, op, op_func )\
    inline ArrayVector2 operator op ( const leftClass &lhs, const rightType fScalar )\
    {\
        assert( fScalar != 0.0 );\
        Real fInv = 1.0f / fScalar;\
        ArrayReal rhs = vdupq_n_f32( fInv );\
        return ArrayVector2(\
                op_func( lhs.mChunkBase[0], rhs ),\
                op_func( lhs.mChunkBase[1], rhs ) );\
    }

#ifdef NDEBUG
    #define ASSERT_DIV_BY_ZERO( values ) ((void)0)
#else
    #define ASSERT_DIV_BY_ZERO( values ) {\
                assert( vmovemaskq_u32( vceqq_f32( values, vdupq_n_f32(0.0f) ) ) == 0 &&\
                "One of the 4 floats is a zero. Can't divide by zero" ); }
#endif

#define DEFINE_L_DIVISION( leftType, rightClass, op, op_func )\
    inline ArrayVector2 operator op ( const leftType lhs, const rightClass &rhs )\
    {\
        return ArrayVector2(\
                op_func( lhs, rhs.mChunkBase[0] ),\
                op_func( lhs, rhs.mChunkBase[1] ) );\
    }
#define DEFINE_R_DIVISION( leftClass, rightType, op, op_func )\
    inline ArrayVector2 operator op ( const leftClass &lhs, const rightType r )\
    {\
        ASSERT_DIV_BY_ZERO( r );\
        ArrayReal rhs = MathlibNEON::Inv4( r );\
        return ArrayVector2(\
                op_func( lhs.mChunkBase[0], rhs ),\
                op_func( lhs.mChunkBase[1], rhs ) );\
    }

    // Update operations
#define DEFINE_UPDATE_OPERATION( leftClass, op, op_func )\
    inline void ArrayVector2::operator op ( const leftClass &a )\
    {\
        ArrayReal * RESTRICT_ALIAS chunkBase = mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS aChunkBase = a.mChunkBase;\
        chunkBase[0] = op_func( chunkBase[0], aChunkBase[0] );\
        chunkBase[1] = op_func( chunkBase[1], aChunkBase[1] );\
    }
#define DEFINE_UPDATE_R_SCALAR_OPERATION( rightType, op, op_func )\
    inline void ArrayVector2::operator op ( const rightType fScalar )\
    {\
        ArrayReal a = vdupq_n_f32( fScalar );\
        mChunkBase[0] = op_func( mChunkBase[0], a );\
        mChunkBase[1] = op_func( mChunkBase[1], a );\
    }
#define DEFINE_UPDATE_R_OPERATION( rightType, op, op_func )\
    inline void ArrayVector2::operator op ( const rightType a )\
    {\
        mChunkBase[0] = op_func( mChunkBase[0], a );\
        mChunkBase[1] = op_func( mChunkBase[1], a );\
    }
#define DEFINE_UPDATE_DIVISION( leftClass, op, op_func )\
    inline void ArrayVector2::operator op ( const leftClass &a )\
    {\
        ArrayReal * RESTRICT_ALIAS chunkBase = mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS aChunkBase = a.mChunkBase;\
        chunkBase[0] = op_func( chunkBase[0], aChunkBase[0] );\
        chunkBase[1] = op_func( chunkBase[1], aChunkBase[1] );\
    }
#define DEFINE_UPDATE_R_SCALAR_DIVISION( rightType, op, op_func )\
    inline void ArrayVector2::operator op ( const rightType fScalar )\
    {\
        assert( fScalar != 0.0 );\
        Real fInv = 1.0f / fScalar;\
        ArrayReal a = vdupq_n_f32( fInv );\
        mChunkBase[0] = op_func( mChunkBase[0], a );\
        mChunkBase[1] = op_func( mChunkBase[1], a );\
    }
#define DEFINE_UPDATE_R_DIVISION( rightType, op, op_func )\
    inline void ArrayVector2::operator op ( const rightType _a )\
    {\
        ASSERT_DIV_BY_ZERO( _a );\
        ArrayReal a = MathlibNEON::Inv4( _a );\
        mChunkBase[0] = op_func( mChunkBase[0], a );\
        mChunkBase[1] = op_func( mChunkBase[1], a );\
    }

    inline const ArrayVector2& ArrayVector2::operator + () const
    {
        return *this;
    };
    //-----------------------------------------------------------------------------------
    inline ArrayVector2 ArrayVector2::operator - () const
    {
        return ArrayVector2(
            veorq_f32( mChunkBase[0], MathlibNEON::SIGN_MASK ),   // -x
            veorq_f32( mChunkBase[1], MathlibNEON::SIGN_MASK ) ); // -y
    }
    //-----------------------------------------------------------------------------------

    // + Addition
    DEFINE_OPERATION( ArrayVector2, ArrayVector2, +, vaddq_f32 );
    DEFINE_L_SCALAR_OPERATION( Real, ArrayVector2, +, vaddq_f32 );
    DEFINE_R_SCALAR_OPERATION( ArrayVector2, Real, +, vaddq_f32 );

    DEFINE_L_OPERATION( ArrayReal, ArrayVector2, +, vaddq_f32 );
    DEFINE_R_OPERATION( ArrayVector2, ArrayReal, +, vaddq_f32 );

    // - Subtraction
    DEFINE_OPERATION( ArrayVector2, ArrayVector2, -, vsubq_f32 );
    DEFINE_L_SCALAR_OPERATION( Real, ArrayVector2, -, vsubq_f32 );
    DEFINE_R_SCALAR_OPERATION( ArrayVector2, Real, -, vsubq_f32 );

    DEFINE_L_OPERATION( ArrayReal, ArrayVector2, -, vsubq_f32 );
    DEFINE_R_OPERATION( ArrayVector2, ArrayReal, -, vsubq_f32 );

    // * Multiplication
    DEFINE_OPERATION( ArrayVector2, ArrayVector2, *, vmulq_f32 );
    DEFINE_L_SCALAR_OPERATION( Real, ArrayVector2, *, vmulq_f32 );
    DEFINE_R_SCALAR_OPERATION( ArrayVector2, Real, *, vmulq_f32 );

    DEFINE_L_OPERATION( ArrayReal, ArrayVector2, *, vmulq_f32 );
    DEFINE_R_OPERATION( ArrayVector2, ArrayReal, *, vmulq_f32 );

    // / Division (scalar versions use mul instead of div, because they mul against the reciprocal)
    DEFINE_OPERATION( ArrayVector2, ArrayVector2, /, vdivq_f32 );
    DEFINE_L_SCALAR_DIVISION( Real, ArrayVector2, /, vdivq_f32 );
    DEFINE_R_SCALAR_DIVISION( ArrayVector2, Real, /, vmulq_f32 );

    DEFINE_L_DIVISION( ArrayReal, ArrayVector2, /, vdivq_f32 );
    DEFINE_R_DIVISION( ArrayVector2, ArrayReal, /, vmulq_f32 );

    inline ArrayVector2 ArrayVector2::Cmov4( const ArrayVector2 &arg1, const ArrayVector2 &arg2, ArrayMaskR mask )
    {
        return ArrayVector2(
                MathlibNEON::Cmov4( arg1.mChunkBase[0], arg2.mChunkBase[0], mask ),
                MathlibNEON::Cmov4( arg1.mChunkBase[1], arg2.mChunkBase[1], mask ) );
    }

    // Update operations
    // +=
    DEFINE_UPDATE_OPERATION(            ArrayVector2,       +=, vaddq_f32 );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               +=, vaddq_f32 );
    DEFINE_UPDATE_R_OPERATION(          ArrayReal,          +=, vaddq_f32 );

    // -=
    DEFINE_UPDATE_OPERATION(            ArrayVector2,       -=, vsubq_f32 );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               -=, vsubq_f32 );
    DEFINE_UPDATE_R_OPERATION(          ArrayReal,          -=, vsubq_f32 );

    // *=
    DEFINE_UPDATE_OPERATION(            ArrayVector2,       *=, vmulq_f32 );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               *=, vmulq_f32 );
    DEFINE_UPDATE_R_OPERATION(          ArrayReal,          *=, vmulq_f32 );

    // /=
    DEFINE_UPDATE_DIVISION(             ArrayVector2,       /=, vdivq_f32 );
    DEFINE_UPDATE_R_SCALAR_DIVISION(    Real,               /=, vmulq_f32 );
    DEFINE_UPDATE_R_DIVISION(           ArrayReal,          /=, vmulq_f32 );
    // clang-format on

    // Functions
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::length() const
    {
        return MathlibNEON::Sqrt( vaddq_f32(                // sqrt(
            vmulq_f32( mChunkBase[0], mChunkBase[0] ),      // (x * x +
            vmulq_f32( mChunkBase[1], mChunkBase[1] ) ) );  // y * y) +
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::squaredLength() const
    {
        return vaddq_f32( vmulq_f32( mChunkBase[0], mChunkBase[0] ),    //(x * x +
                          vmulq_f32( mChunkBase[1], mChunkBase[1] ) );  // y * y)
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::distance( const ArrayVector2 &rhs ) const
    {
        return ( *this - rhs ).length();
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::squaredDistance( const ArrayVector2 &rhs ) const
    {
        return ( *this - rhs ).squaredLength();
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::dotProduct( const ArrayVector2 &vec ) const
    {
        return vaddq_f32( vmulq_f32( mChunkBase[0], vec.mChunkBase[0] ),    //( x * vec.x   +
                          vmulq_f32( mChunkBase[1], vec.mChunkBase[1] ) );  //  y * vec.y )
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::absDotProduct( const ArrayVector2 &vec ) const
    {
        return vaddq_f32(
            MathlibNEON::Abs4( vmulq_f32( mChunkBase[0], vec.mChunkBase[0] ) ),  //( abs( x * vec.x )   +
            MathlibNEON::Abs4( vmulq_f32( mChunkBase[1], vec.mChunkBase[1] ) ) );  //  abs( y * vec.y ) )
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::normalise()
    {
        ArrayReal sqLength = vaddq_f32( vmulq_f32( mChunkBase[0], mChunkBase[0] ),    //(x * x +
                                        vmulq_f32( mChunkBase[1], mChunkBase[1] ) );  // y * y)

        // Convert sqLength's 0s into 1, so that zero vectors remain as zero
        // Denormals are treated as 0 during the check.
        // Note: We could create a mask now and nuke nans after InvSqrt, however
        // generating the nans could impact performance in some architectures
        sqLength = MathlibNEON::Cmov4( sqLength, MathlibNEON::ONE,
                                       vcgtq_f32( sqLength, MathlibNEON::FLOAT_MIN ) );
        ArrayReal invLength = MathlibNEON::InvSqrtNonZero4( sqLength );
        mChunkBase[0] = vmulq_f32( mChunkBase[0], invLength );  // x * invLength
        mChunkBase[1] = vmulq_f32( mChunkBase[1], invLength );  // y * invLength
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::crossProduct( const ArrayVector2 &rkVec ) const
    {
        return vsubq_f32(
            vmulq_f32( mChunkBase[0], rkVec.mChunkBase[1] ),
            vmulq_f32( mChunkBase[1], rkVec.mChunkBase[0] ) );  // x * rkVec.y - y * rkVec.x
    }
    //-----------------------------------------------------------------------------------
    inline ArrayVector2 ArrayVector2::midPoint( const ArrayVector2 &rkVec ) const
    {
        return ArrayVector2(
            vmulq_f32( vaddq_f32( mChunkBase[0], rkVec.mChunkBase[0] ), MathlibNEON::HALF ),
            vmulq_f32( vaddq_f32( mChunkBase[1], rkVec.mChunkBase[1] ), MathlibNEON::HALF ) );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::makeFloor( const ArrayVector2 &cmp )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = cmp.mChunkBase;
        aChunkBase[0] = vminq_f32( aChunkBase[0], bChunkBase[0] );
        aChunkBase[1] = vminq_f32( aChunkBase[1], bChunkBase[1] );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::makeCeil( const ArrayVector2 &cmp )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = cmp.mChunkBase;
        aChunkBase[0] = vmaxq_f32( aChunkBase[0], bChunkBase[0] );
        aChunkBase[1] = vmaxq_f32( aChunkBase[1], bChunkBase[1] );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::getMinComponent() const
    {
        return vminq_f32( mChunkBase[0], mChunkBase[1] );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::getMaxComponent() const
    {
        return vmaxq_f32( mChunkBase[0], mChunkBase[1] );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::setToSign()
    {
        // x = 1.0f | (x & 0x80000000)
        ArrayReal signMask = vdupq_n_f32( -0.0f );
        mChunkBase[0] = vorrq_f32( MathlibNEON::ONE, vandq_f32( signMask, mChunkBase[0] ) );
        mChunkBase[1] = vorrq_f32( MathlibNEON::ONE, vandq_f32( signMask, mChunkBase[1] ) );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayVector2 ArrayVector2::perpendicular() const
    {
        return ArrayVector2( veorq_f32( mChunkBase[1], MathlibNEON::SIGN_MASK ), mChunkBase[0] );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayVector2 ArrayVector2::normalisedCopy() const
    {
        ArrayReal sqLength = vaddq_f32( vmulq_f32( mChunkBase[0], mChunkBase[0] ),    //(x * x +
                                        vmulq_f32( mChunkBase[1], mChunkBase[1] ) );  // y * y)

        // Convert sqLength's 0s into 1, so that zero vectors remain as zero
        // Denormals are treated as 0 during the check.
        // Note: We could create a mask now and nuke nans after InvSqrt, however
        // generating the nans could impact performance in some architectures
        sqLength = MathlibNEON::Cmov4( sqLength, MathlibNEON::ONE,
                                       vcgtq_f32( sqLength, MathlibNEON::FLOAT_MIN ) );
        ArrayReal invLength = MathlibNEON::InvSqrtNonZero4( sqLength );

        return ArrayVector2( vmulq_f32( mChunkBase[0], invLength ),    // x * invLength
                             vmulq_f32( mChunkBase[1], invLength ) );  // y * invLength
    }
    //-----------------------------------------------------------------------------------
    inline ArrayVector2 ArrayVector2::reflect( const ArrayVector2 &normal ) const
    {
        const ArrayReal twoPointZero = vdupq_n_f32( 2.0f );
        return ( *this - ( vmulq_f32( twoPointZero, this->dotProduct( normal ) ) * normal ) );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::inverseLeaveZeroes()
    {
        // Use InvNonZero, we're gonna nuke the NaNs anyway.
        mChunkBase[0] =
            MathlibNEON::CmovRobust( mChunkBase[0], MathlibNEON::InvNonZero4( mChunkBase[0] ),
                                     vceqq_f32( mChunkBase[0], vdupq_n_f32( 0.0f ) ) );
        mChunkBase[1] =
            MathlibNEON::CmovRobust( mChunkBase[1], MathlibNEON::InvNonZero4( mChunkBase[1] ),
                                     vceqq_f32( mChunkBase[1], vdupq_n_f32( 0.0f ) ) );
    }
    //-----------------------------------------------------------------------------------
    inline int ArrayVector2::isNaN() const
    {
        ArrayMaskR mask = vandq_u32( vceqq_f32( mChunkBase[0], mChunkBase[0] ),
                                     vceqq_f32( mChunkBase[1], mChunkBase[1] ) );

        return int( vmovemaskq_u32( mask ) ^ 0x0000000f );
    }
    //-----------------------------------------------------------------------------------
    inline Vector2 ArrayVector2::collapseMin() const
    {
        Real min0 = MathlibNEON::CollapseMin( mChunkBase[0] );
        Real min1 = MathlibNEON::CollapseMin( mChunkBase[1] );
        return Vector2( min0, min1 );
    }
    //-----------------------------------------------------------------------------------
    inline Vector2 ArrayVector2::collapseMax() const
    {
        Real max0 = MathlibNEON::CollapseMax( mChunkBase[0] );
        Real max1 = MathlibNEON::CollapseMax( mChunkBase[1] );
        return Vector2( max0, max1 );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::Cmov4( ArrayMaskR mask, const ArrayVector2 &replacement )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = replacement.mChunkBase;
        aChunkBase[0] = MathlibNEON::Cmov4( aChunkBase[0], bChunkBase[0], mask );
        aChunkBase[1] = MathlibNEON::Cmov4( aChunkBase[1], bChunkBase[1], mask );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::CmovRobust( ArrayMaskR mask, const ArrayVector2 &replacement )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = mChunkBase;
        const ArrayReal *RESTRICT_ALIAS bChunkBase = replacement.mChunkBase;
        aChunkBase[0] = MathlibNEON::CmovRobust( aChunkBase[0], bChunkBase[0], mask );
        aChunkBase[1] = MathlibNEON::CmovRobust( aChunkBase[1], bChunkBase[1], mask );
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
