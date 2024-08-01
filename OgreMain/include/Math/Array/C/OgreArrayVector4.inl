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
#define DEFINE_OPERATION( leftClass, rightClass, op )\
    inline ArrayVector4 operator op ( const leftClass &lhs, const rightClass &rhs )\
    {\
        const ArrayReal * RESTRICT_ALIAS lhsChunkBase = lhs.mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS rhsChunkBase = rhs.mChunkBase;\
        return ArrayVector4(\
                lhsChunkBase[0] op rhsChunkBase[0],\
                lhsChunkBase[1] op rhsChunkBase[1],\
                lhsChunkBase[2] op rhsChunkBase[2],\
                lhsChunkBase[3] op rhsChunkBase[3] );\
       }
#define DEFINE_L_OPERATION( leftType, rightClass, op )\
    inline ArrayVector4 operator op ( const leftType fScalar, const rightClass &rhs )\
    {\
        return ArrayVector4(\
                fScalar op rhs.mChunkBase[0],\
                fScalar op rhs.mChunkBase[1],\
                fScalar op rhs.mChunkBase[2],\
                fScalar op rhs.mChunkBase[3] );\
    }
#define DEFINE_R_OPERATION( leftClass, rightType, op )\
    inline ArrayVector4 operator op ( const leftClass &lhs, const rightType fScalar )\
    {\
        return ArrayVector4(\
                lhs.mChunkBase[0] op fScalar,\
                lhs.mChunkBase[1] op fScalar,\
                lhs.mChunkBase[2] op fScalar,\
                lhs.mChunkBase[3] op fScalar );\
    }

#define DEFINE_L_SCALAR_DIVISION( leftType, rightClass, op )\
    inline ArrayVector4 operator op ( const leftType fScalar, const rightClass &rhs )\
    {\
        return ArrayVector4(\
                fScalar op rhs.mChunkBase[0],\
                fScalar op rhs.mChunkBase[1],\
                fScalar op rhs.mChunkBase[2],\
                fScalar op rhs.mChunkBase[3] );\
    }
#define DEFINE_R_SCALAR_DIVISION( leftClass, rightType, op, op_func )\
    inline ArrayVector4 operator op ( const leftClass &lhs, const rightType fScalar )\
    {\
        assert( fScalar != 0.0 );\
        Real fInv = 1.0f / fScalar;\
        return ArrayVector4(\
                lhs.mChunkBase[0] op_func fInv,\
                lhs.mChunkBase[1] op_func fInv,\
                lhs.mChunkBase[2] op_func fInv,\
                lhs.mChunkBase[3] op_func fInv );\
    }

#ifdef NDEBUG
    #define ASSERT_DIV_BY_ZERO( values ) ((void)0)
#else
    #define ASSERT_DIV_BY_ZERO( values ) {\
                assert( values != 0 && "One of the 4 floats is a zero. Can't divide by zero" ); }
#endif

    // Update operations
#define DEFINE_UPDATE_OPERATION( leftClass, op, op_func )\
    inline void ArrayVector4::operator op ( const leftClass &a )\
    {\
        ArrayReal * RESTRICT_ALIAS chunkBase = mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS aChunkBase = a.mChunkBase;\
        chunkBase[0] = chunkBase[0] op_func aChunkBase[0];\
        chunkBase[1] = chunkBase[1] op_func aChunkBase[1];\
        chunkBase[2] = chunkBase[2] op_func aChunkBase[2];\
        chunkBase[3] = chunkBase[3] op_func aChunkBase[3];\
    }
#define DEFINE_UPDATE_R_SCALAR_OPERATION( rightType, op, op_func )\
    inline void ArrayVector4::operator op ( const rightType fScalar )\
    {\
        mChunkBase[0] = mChunkBase[0] op_func fScalar;\
        mChunkBase[1] = mChunkBase[1] op_func fScalar;\
        mChunkBase[2] = mChunkBase[2] op_func fScalar;\
        mChunkBase[3] = mChunkBase[3] op_func fScalar;\
    }
#define DEFINE_UPDATE_DIVISION( leftClass, op, op_func )\
    inline void ArrayVector4::operator op ( const leftClass &a )\
    {\
        ArrayReal * RESTRICT_ALIAS chunkBase = mChunkBase;\
        const ArrayReal * RESTRICT_ALIAS aChunkBase = a.mChunkBase;\
        chunkBase[0] = chunkBase[0] op_func aChunkBase[0];\
        chunkBase[1] = chunkBase[1] op_func aChunkBase[1];\
        chunkBase[2] = chunkBase[2] op_func aChunkBase[2];\
        chunkBase[3] = chunkBase[3] op_func aChunkBase[3];\
    }
#define DEFINE_UPDATE_R_SCALAR_DIVISION( rightType, op, op_func )\
    inline void ArrayVector4::operator op ( const rightType fScalar )\
    {\
        assert( fScalar != 0.0 );\
        Real fInv = 1.0f / fScalar;\
        mChunkBase[0] = mChunkBase[0] op_func fInv;\
        mChunkBase[1] = mChunkBase[1] op_func fInv;\
        mChunkBase[2] = mChunkBase[2] op_func fInv;\
        mChunkBase[3] = mChunkBase[3] op_func fInv;\
    }

    inline const ArrayVector4& ArrayVector4::operator + () const
    {
        return *this;
    };
    //-----------------------------------------------------------------------------------
    inline ArrayVector4 ArrayVector4::operator - () const
    {
        return ArrayVector4( -mChunkBase[0], -mChunkBase[1], -mChunkBase[2], -mChunkBase[3] );
    }
    //-----------------------------------------------------------------------------------

    // + Addition
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, + );
    DEFINE_L_OPERATION( Real, ArrayVector4, + );
    DEFINE_R_OPERATION( ArrayVector4, Real, + );

    // - Subtraction
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, - );
    DEFINE_L_OPERATION( Real, ArrayVector4, - );
    DEFINE_R_OPERATION( ArrayVector4, Real, - );

    // * Multiplication
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, * );
    DEFINE_L_OPERATION( Real, ArrayVector4, * );
    DEFINE_R_OPERATION( ArrayVector4, Real, * );

    // / Division (scalar versions use mul instead of div, because they mul against the reciprocal)
    DEFINE_OPERATION( ArrayVector4, ArrayVector4, / );
    DEFINE_L_SCALAR_DIVISION( Real, ArrayVector4, / );
    DEFINE_R_SCALAR_DIVISION( ArrayVector4, Real, /, * );

    inline ArrayVector4 ArrayVector4::Cmov4( const ArrayVector4 &arg1, const ArrayVector4 &arg2, ArrayMaskR mask )
    {
        return ArrayVector4(
                MathlibC::Cmov4( arg1.mChunkBase[0], arg2.mChunkBase[0], mask ),
                MathlibC::Cmov4( arg1.mChunkBase[1], arg2.mChunkBase[1], mask ),
                MathlibC::Cmov4( arg1.mChunkBase[2], arg2.mChunkBase[2], mask ),
                MathlibC::Cmov4( arg1.mChunkBase[3], arg2.mChunkBase[3], mask ) );
    }

    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------

    // Update operations
    // +=
    DEFINE_UPDATE_OPERATION(            ArrayVector4,       +=, + );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               +=, + );

    // -=
    DEFINE_UPDATE_OPERATION(            ArrayVector4,       -=, - );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               -=, - );

    // *=
    DEFINE_UPDATE_OPERATION(            ArrayVector4,       *=, * );
    DEFINE_UPDATE_R_SCALAR_OPERATION(   Real,               *=, * );

    // /=
    DEFINE_UPDATE_DIVISION(             ArrayVector4,       /=, / );
    DEFINE_UPDATE_R_SCALAR_DIVISION(    Real,               /=, * );
    // clang-format on

    // Functions
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::fma3x1( const ArrayReal m, const ArrayReal a,  //
                                      const ArrayReal m2, const ArrayReal a2 )
    {
        mChunkBase[0] = mChunkBase[0] * m + a;
        mChunkBase[1] = mChunkBase[1] * m + a;
        mChunkBase[2] = mChunkBase[2] * m + a;

        mChunkBase[3] = mChunkBase[3] * m2 + a2;
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::makeFloor( const ArrayVector4 &cmp )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = &mChunkBase[0];
        const ArrayReal *RESTRICT_ALIAS bChunkBase = &cmp.mChunkBase[0];
        aChunkBase[0] = std::min( aChunkBase[0], bChunkBase[0] );
        aChunkBase[1] = std::min( aChunkBase[1], bChunkBase[1] );
        aChunkBase[2] = std::min( aChunkBase[2], bChunkBase[2] );
        aChunkBase[3] = std::min( aChunkBase[3], bChunkBase[3] );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::makeCeil( const ArrayVector4 &cmp )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = &mChunkBase[0];
        const ArrayReal *RESTRICT_ALIAS bChunkBase = &cmp.mChunkBase[0];
        aChunkBase[0] = std::max( aChunkBase[0], bChunkBase[0] );
        aChunkBase[1] = std::max( aChunkBase[1], bChunkBase[1] );
        aChunkBase[2] = std::max( aChunkBase[2], bChunkBase[2] );
        aChunkBase[3] = std::max( aChunkBase[3], bChunkBase[3] );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::inverseLeaveZeroes()
    {
        mChunkBase[0] = MathlibC::CmovRobust( mChunkBase[0], MathlibC::InvNonZero4( mChunkBase[0] ),
                                              mChunkBase[0] == 0.0f );
        mChunkBase[1] = MathlibC::CmovRobust( mChunkBase[1], MathlibC::InvNonZero4( mChunkBase[1] ),
                                              mChunkBase[1] == 0.0f );
        mChunkBase[2] = MathlibC::CmovRobust( mChunkBase[2], MathlibC::InvNonZero4( mChunkBase[2] ),
                                              mChunkBase[2] == 0.0f );
        mChunkBase[3] = MathlibC::CmovRobust( mChunkBase[3], MathlibC::InvNonZero4( mChunkBase[3] ),
                                              mChunkBase[3] == 0.0f );
    }
    //-----------------------------------------------------------------------------------
    inline int ArrayVector4::isNaN() const
    {
        return Math::isNaN( mChunkBase[0] ) | Math::isNaN( mChunkBase[1] ) |
               Math::isNaN( mChunkBase[2] ) | Math::isNaN( mChunkBase[3] );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::Cmov4( ArrayMaskR mask, const ArrayVector4 &replacement )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = &mChunkBase[0];
        const ArrayReal *RESTRICT_ALIAS bChunkBase = &replacement.mChunkBase[0];
        aChunkBase[0] = MathlibC::Cmov4( aChunkBase[0], bChunkBase[0], mask );
        aChunkBase[1] = MathlibC::Cmov4( aChunkBase[1], bChunkBase[1], mask );
        aChunkBase[2] = MathlibC::Cmov4( aChunkBase[2], bChunkBase[2], mask );
        aChunkBase[3] = MathlibC::Cmov4( aChunkBase[3], bChunkBase[3], mask );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayVector4::CmovRobust( ArrayMaskR mask, const ArrayVector4 &replacement )
    {
        ArrayReal *RESTRICT_ALIAS       aChunkBase = &mChunkBase[0];
        const ArrayReal *RESTRICT_ALIAS bChunkBase = &replacement.mChunkBase[0];
        aChunkBase[0] = MathlibC::CmovRobust( aChunkBase[0], bChunkBase[0], mask );
        aChunkBase[1] = MathlibC::CmovRobust( aChunkBase[1], bChunkBase[1], mask );
        aChunkBase[2] = MathlibC::CmovRobust( aChunkBase[2], bChunkBase[2], mask );
        aChunkBase[3] = MathlibC::CmovRobust( aChunkBase[3], bChunkBase[3], mask );
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
    // END CODE TO BE COPIED TO OgreArrayVector4.inl
    //-----------------------------------------------------------------------------------
}  // namespace Ogre
