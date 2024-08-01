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
        DEFINE_OPERATION( ArrayVector2, ArrayVector2, +, _mm_add_ps );
        Means: "define '+' operator that takes both arguments as ArrayVector2 and use
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
#define DEFINE_OPERATION( leftClass, rightClass, op )\
	inline ArrayVector2 operator op ( const leftClass &lhs, const rightClass &rhs )\
    {\
        const ArrayReal * RESTRICT_ALIAS lhsChunkBase = lhs.mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS rhsChunkBase = rhs.mChunkBase;\
		return ArrayVector2(\
                lhsChunkBase[0] op rhsChunkBase[0],\
				lhsChunkBase[1] op rhsChunkBase[1] );\
       }
#define DEFINE_L_OPERATION( leftType, rightClass, op )\
	inline ArrayVector2 operator op ( const leftType fScalar, const rightClass &rhs )\
    {\
		return ArrayVector2(\
                fScalar op rhs.mChunkBase[0],\
				fScalar op rhs.mChunkBase[1] );\
    }
#define DEFINE_R_OPERATION( leftClass, rightType, op )\
	inline ArrayVector2 operator op ( const leftClass &lhs, const rightType fScalar )\
    {\
		return ArrayVector2(\
                lhs.mChunkBase[0] op fScalar,\
				lhs.mChunkBase[1] op fScalar );\
    }

#define DEFINE_L_SCALAR_DIVISION( leftType, rightClass, op )\
	inline ArrayVector2 operator op ( const leftType fScalar, const rightClass &rhs )\
    {\
		return ArrayVector2(\
                fScalar op rhs.mChunkBase[0],\
				fScalar op rhs.mChunkBase[1] );\
    }
#define DEFINE_R_SCALAR_DIVISION( leftClass, rightType, op, op_func )\
	inline ArrayVector2 operator op ( const leftClass &lhs, const rightType fScalar )\
    {\
        assert( fScalar != 0.0 );\
        Real fInv = 1.0f / fScalar;\
		return ArrayVector2(\
                lhs.mChunkBase[0] op_func fInv,\
				lhs.mChunkBase[1] op_func fInv );\
    }

#ifdef NDEBUG
    #define ASSERT_DIV_BY_ZERO( values ) ((void)0)
#else
    #define ASSERT_DIV_BY_ZERO( values ) {\
                assert( values != 0 && "One of the 4 floats is a zero. Can't divide by zero" ); }
#endif

    // Update operations
#define DEFINE_UPDATE_OPERATION( leftClass, op, op_func )\
	inline void ArrayVector2::operator op ( const leftClass &a )\
    {\
        ArrayReal * RESTRICT_ALIAS chunkBase = mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS aChunkBase = a.mChunkBase;\
        chunkBase[0] = chunkBase[0] op_func aChunkBase[0];\
		chunkBase[1] = chunkBase[1] op_func aChunkBase[1];\
    }
#define DEFINE_UPDATE_R_SCALAR_OPERATION( rightType, op, op_func )\
	inline void ArrayVector2::operator op ( const rightType fScalar )\
    {\
        mChunkBase[0] = mChunkBase[0] op_func fScalar;\
		mChunkBase[1] = mChunkBase[1] op_func fScalar;\
    }
#define DEFINE_UPDATE_DIVISION( leftClass, op, op_func )\
	inline void ArrayVector2::operator op ( const leftClass &a )\
    {\
        ArrayReal * RESTRICT_ALIAS chunkBase = mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS aChunkBase = a.mChunkBase;\
        chunkBase[0] = chunkBase[0] op_func aChunkBase[0];\
		chunkBase[1] = chunkBase[1] op_func aChunkBase[1];\
    }
#define DEFINE_UPDATE_R_SCALAR_DIVISION( rightType, op, op_func )\
	inline void ArrayVector2::operator op ( const rightType fScalar )\
    {\
        assert( fScalar != 0.0 );\
        Real fInv = 1.0f / fScalar;\
        mChunkBase[0] = mChunkBase[0] op_func fInv;\
		mChunkBase[1] = mChunkBase[1] op_func fInv;\
    }

	inline const ArrayVector2& ArrayVector2::operator + () const
    {
        return *this;
    };
    //-----------------------------------------------------------------------------------
	inline ArrayVector2 ArrayVector2::operator - () const
    {
		return ArrayVector2( -mChunkBase[0], -mChunkBase[1] );
    }
    //-----------------------------------------------------------------------------------

    // + Addition
	DEFINE_OPERATION( ArrayVector2, ArrayVector2, + );
	DEFINE_L_OPERATION( Real, ArrayVector2, + );
	DEFINE_R_OPERATION( ArrayVector2, Real, + );

    // - Subtraction
	DEFINE_OPERATION( ArrayVector2, ArrayVector2, - );
	DEFINE_L_OPERATION( Real, ArrayVector2, - );
	DEFINE_R_OPERATION( ArrayVector2, Real, - );

    // * Multiplication
	DEFINE_OPERATION( ArrayVector2, ArrayVector2, * );
	DEFINE_L_OPERATION( Real, ArrayVector2, * );
	DEFINE_R_OPERATION( ArrayVector2, Real, * );

    // / Division (scalar versions use mul instead of div, because they mul against the reciprocal)
	DEFINE_OPERATION( ArrayVector2, ArrayVector2, / );
	DEFINE_L_SCALAR_DIVISION( Real, ArrayVector2, / );
	DEFINE_R_SCALAR_DIVISION( ArrayVector2, Real, /, * );

	inline ArrayVector2 ArrayVector2::Cmov4( const ArrayVector2 &arg1, const ArrayVector2 &arg2, ArrayMaskR mask )
    {
		return ArrayVector2(
                MathlibC::Cmov4( arg1.mChunkBase[0], arg2.mChunkBase[0], mask ),
				MathlibC::Cmov4( arg1.mChunkBase[1], arg2.mChunkBase[1], mask ) );
    }

    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------

    // Update operations
    // +=
	DEFINE_UPDATE_OPERATION(            ArrayVector2,       +=, + );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               +=, + );

    // -=
	DEFINE_UPDATE_OPERATION(            ArrayVector2,       -=, - );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               -=, - );

    // *=
	DEFINE_UPDATE_OPERATION(            ArrayVector2,       *=, * );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               *=, * );

    // /=
	DEFINE_UPDATE_DIVISION(             ArrayVector2,       /=, / );
    DEFINE_UPDATE_R_SCALAR_DIVISION(    Real,               /=, * );
    // clang-format on

    // Functions
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::length() const
    {
        return std::sqrt( ( mChunkBase[0] * mChunkBase[0] ) +  //
                          ( mChunkBase[1] * mChunkBase[1] ) );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::squaredLength() const
    {
        return ( mChunkBase[0] * mChunkBase[0] ) +  //
               ( mChunkBase[1] * mChunkBase[1] );
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
        return ( mChunkBase[0] * vec.mChunkBase[0] ) + ( mChunkBase[1] * vec.mChunkBase[1] );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::absDotProduct( const ArrayVector2 &vec ) const
    {
        return Math::Abs( ( mChunkBase[0] * vec.mChunkBase[0] ) ) +
               Math::Abs( ( mChunkBase[1] * vec.mChunkBase[1] ) );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::normalise()
    {
        ArrayReal sqLength = ( mChunkBase[0] * mChunkBase[0] ) +  //
                             ( mChunkBase[1] * mChunkBase[1] );

        // Convert sqLength's 0s into 1, so that zero vectors remain as zero
        // Denormals are treated as 0 during the check.
        // Note: We could create a mask now and nuke nans after InvSqrt, however
        // generating the nans could impact performance in some architectures
        sqLength = MathlibC::Cmov4( sqLength, 1.0f, sqLength > MathlibC::FLOAT_MIN );
        ArrayReal invLength = MathlibC::InvSqrtNonZero4( sqLength );
        mChunkBase[0] = mChunkBase[0] * invLength;  // x * invLength
        mChunkBase[1] = mChunkBase[1] * invLength;  // y * invLength
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::crossProduct( const ArrayVector2 &rkVec ) const
    {
        return ( mChunkBase[0] * rkVec.mChunkBase[1] ) -  //
               ( mChunkBase[1] * rkVec.mChunkBase[0] );   // x * rkVec.y - y * rkVec.x
    }
    //-----------------------------------------------------------------------------------
    inline ArrayVector2 ArrayVector2::midPoint( const ArrayVector2 &rkVec ) const
    {
        return ArrayVector2( ( mChunkBase[0] + rkVec.mChunkBase[0] ) * 0.5f,
                             ( mChunkBase[1] + rkVec.mChunkBase[1] ) * 0.5f );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::makeFloor( const ArrayVector2 &cmp )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = &mChunkBase[0];
        const ArrayReal *RESTRICT_ALIAS bChunkBase = &cmp.mChunkBase[0];
        aChunkBase[0] = std::min( aChunkBase[0], bChunkBase[0] );
        aChunkBase[1] = std::min( aChunkBase[1], bChunkBase[1] );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::makeCeil( const ArrayVector2 &cmp )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = &mChunkBase[0];
        const ArrayReal *RESTRICT_ALIAS bChunkBase = &cmp.mChunkBase[0];
        aChunkBase[0] = std::max( aChunkBase[0], bChunkBase[0] );
        aChunkBase[1] = std::max( aChunkBase[1], bChunkBase[1] );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::getMinComponent() const
    {
        return std::min( mChunkBase[0], mChunkBase[1] );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayVector2::getMaxComponent() const
    {
        return std::max( mChunkBase[0], mChunkBase[1] );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::setToSign()
    {
        mChunkBase[0] = mChunkBase[0] >= 0 ? 1.0f : -1.0f;
        mChunkBase[1] = mChunkBase[1] >= 0 ? 1.0f : -1.0f;
    }
    //-----------------------------------------------------------------------------------
    inline ArrayVector2 ArrayVector2::perpendicular() const
    {
        return ArrayVector2( -mChunkBase[1], mChunkBase[0] );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayVector2 ArrayVector2::normalisedCopy() const
    {
        ArrayReal sqLength = ( mChunkBase[0] * mChunkBase[0] ) +  //
                             ( mChunkBase[1] * mChunkBase[1] );

        // Convert sqLength's 0s into 1, so that zero vectors remain as zero
        // Denormals are treated as 0 during the check.
        // Note: We could create a mask now and nuke nans after InvSqrt, however
        // generating the nans could impact performance in some architectures
        sqLength = MathlibC::Cmov4( sqLength, MathlibC::ONE, sqLength > MathlibC::FLOAT_MIN );
        ArrayReal invLength = MathlibC::InvSqrtNonZero4( sqLength );

        return ArrayVector2( mChunkBase[0] * invLength,    // x * invLength
                             mChunkBase[1] * invLength );  // y * invLength
    }
    //-----------------------------------------------------------------------------------
    inline ArrayVector2 ArrayVector2::reflect( const ArrayVector2 &normal ) const
    {
        return ( *this - ( 2.0f * this->dotProduct( normal ) ) * normal );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::inverseLeaveZeroes()
    {
        mChunkBase[0] = MathlibC::CmovRobust( mChunkBase[0], MathlibC::InvNonZero4( mChunkBase[0] ),
                                              mChunkBase[0] == 0.0f );
        mChunkBase[1] = MathlibC::CmovRobust( mChunkBase[1], MathlibC::InvNonZero4( mChunkBase[1] ),
                                              mChunkBase[1] == 0.0f );
    }
    //-----------------------------------------------------------------------------------
    inline int ArrayVector2::isNaN() const
    {
        return Math::isNaN( mChunkBase[0] ) | Math::isNaN( mChunkBase[1] );
    }
    //-----------------------------------------------------------------------------------
    inline Vector2 ArrayVector2::collapseMin() const { return Vector2( mChunkBase[0], mChunkBase[1] ); }
    //-----------------------------------------------------------------------------------
    inline Vector2 ArrayVector2::collapseMax() const { return Vector2( mChunkBase[0], mChunkBase[1] ); }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::Cmov4( ArrayMaskR mask, const ArrayVector2 &replacement )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = &mChunkBase[0];
        const ArrayReal *RESTRICT_ALIAS bChunkBase = &replacement.mChunkBase[0];
        aChunkBase[0] = MathlibC::Cmov4( aChunkBase[0], bChunkBase[0], mask );
        aChunkBase[1] = MathlibC::Cmov4( aChunkBase[1], bChunkBase[1], mask );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector2::CmovRobust( ArrayMaskR mask, const ArrayVector2 &replacement )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = &mChunkBase[0];
        const ArrayReal *RESTRICT_ALIAS bChunkBase = &replacement.mChunkBase[0];
        aChunkBase[0] = MathlibC::CmovRobust( aChunkBase[0], bChunkBase[0], mask );
        aChunkBase[1] = MathlibC::CmovRobust( aChunkBase[1], bChunkBase[1], mask );
    }
    //-----------------------------------------------------------------------------------

#undef DEFINE_OPERATION
#undef DEFINE_L_OPERATION
#undef DEFINE_R_OPERATION
#undef DEFINE_L_SCALAR_DIVISION
#undef DEFINE_R_SCALAR_DIVISION

#undef DEFINE_UPDATE_OPERATION
#undef DEFINE_UPDATE_R_SCALAR_OPERATION
#undef DEFINE_UPDATE_DIVISION
#undef DEFINE_UPDATE_R_SCALAR_DIVISION

    //-----------------------------------------------------------------------------------
    // END CODE TO BE COPIED TO OgreArrayVector2.inl
    //-----------------------------------------------------------------------------------
}  // namespace Ogre
