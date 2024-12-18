/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre4d.org/

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
#ifndef Ogre_SSE2_ArrayVector4_H
#define Ogre_SSE2_ArrayVector4_H

#ifndef OgreArrayVector4_H
#    error "Don't include this file directly. include Math/Array/OgreArrayVector4.h"
#endif

#include "OgreVector4.h"

#include "Math/Array/OgreMathlib.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Math
     *  @{
     */
    /** Cache-friendly array of 4-dimensional represented as a SoA array.
        @remarks
            ArrayVector4 is a SIMD & cache-friendly version of Vector4.
            An operation on an ArrayVector4 is done on 4 vectors at a
            time (the actual amount is defined by ARRAY_PACKED_REALS)
            Assuming ARRAY_PACKED_REALS == 4, the memory layout will
            be as following:
                 mChunkBase				mChunkBase + 4
            XXXX YYYY ZZZZ WWWW      XXXX YYYY ZZZZ WWWW
            Extracting one vector (XYZW) needs 64 bytes, which is within
            the 64 byte size of common cache lines.
            Architectures where the cache line == 64 bytes may want to
            set ARRAY_PACKED_REALS = 2 depending on their needs
    */

    class _OgreExport ArrayVector4
    {
    public:
        ArrayReal mChunkBase[4];

        ArrayVector4() {}
        ArrayVector4( ArrayReal chunkX, ArrayReal chunkY, ArrayReal chunkZ, ArrayReal chunkW )
        {
            mChunkBase[0] = chunkX;
            mChunkBase[1] = chunkY;
            mChunkBase[2] = chunkZ;
            mChunkBase[3] = chunkW;
        }

        void getAsVector4( Vector4 &out, size_t index ) const
        {
            // Be careful of not writing to these regions or else strict aliasing rule gets broken!!!
            const Real *aliasedReal = reinterpret_cast<const Real *>( mChunkBase );
            out.x = aliasedReal[ARRAY_PACKED_REALS * 0 + index];  // X
            out.y = aliasedReal[ARRAY_PACKED_REALS * 1 + index];  // Y
            out.z = aliasedReal[ARRAY_PACKED_REALS * 2 + index];  // Z
            out.w = aliasedReal[ARRAY_PACKED_REALS * 3 + index];  // W
        }

        /// Prefer using @see getAsVector4() because this function may have more
        /// overhead (the other one is faster)
        Vector4 getAsVector4( size_t index ) const
        {
            // Be careful of not writing to these regions or else strict aliasing rule gets broken!!!
            const Real *aliasedReal = reinterpret_cast<const Real *>( mChunkBase );
            return Vector4( aliasedReal[ARRAY_PACKED_REALS * 0 + index],    // X
                            aliasedReal[ARRAY_PACKED_REALS * 1 + index],    // Y
                            aliasedReal[ARRAY_PACKED_REALS * 2 + index],    // Z
                            aliasedReal[ARRAY_PACKED_REALS * 3 + index] );  // W
        }

        void setFromVector4( const Vector4 &v, size_t index )
        {
            Real *aliasedReal = reinterpret_cast<Real *>( mChunkBase );
            aliasedReal[ARRAY_PACKED_REALS * 0 + index] = v.x;
            aliasedReal[ARRAY_PACKED_REALS * 1 + index] = v.y;
            aliasedReal[ARRAY_PACKED_REALS * 2 + index] = v.z;
            aliasedReal[ARRAY_PACKED_REALS * 3 + index] = v.w;
        }

        /// Sets all packed vectors to the same value as the scalar input vector
        void setAll( const Vector4 &v )
        {
            mChunkBase[0] = _mm_set_ps1( v.x );
            mChunkBase[1] = _mm_set_ps1( v.y );
            mChunkBase[2] = _mm_set_ps1( v.z );
            mChunkBase[3] = _mm_set_ps1( v.w );
        }

        inline ArrayVector4 &operator=( const Real fScalar )
        {
            // set1_ps is a composite instrinsic using shuffling instructions.
            // Store the actual result in a tmp variable and copy. We don't
            // do mChunkBase[1] = mChunkBase[0]; because of a potential LHS
            // depending on how smart the compiler was
            ArrayReal tmp = _mm_set1_ps( fScalar );
            mChunkBase[0] = tmp;
            mChunkBase[1] = tmp;
            mChunkBase[2] = tmp;
            mChunkBase[3] = tmp;

            return *this;
        }

        // Arithmetic operations
        inline const ArrayVector4 &operator+() const;
        inline ArrayVector4        operator-() const;

        inline friend ArrayVector4 operator+( const ArrayVector4 &lhs, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator+( Real fScalar, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator+( const ArrayVector4 &lhs, Real fScalar );

        inline friend ArrayVector4 operator+( ArrayReal fScalar, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator+( const ArrayVector4 &lhs, ArrayReal fScalar );

        inline friend ArrayVector4 operator-( const ArrayVector4 &lhs, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator-( Real fScalar, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator-( const ArrayVector4 &lhs, Real fScalar );

        inline friend ArrayVector4 operator-( ArrayReal fScalar, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator-( const ArrayVector4 &lhs, ArrayReal fScalar );

        inline friend ArrayVector4 operator*( const ArrayVector4 &lhs, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator*( Real fScalar, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator*( const ArrayVector4 &lhs, Real fScalar );

        inline friend ArrayVector4 operator*( ArrayReal fScalar, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator*( const ArrayVector4 &lhs, ArrayReal fScalar );

        inline friend ArrayVector4 operator/( const ArrayVector4 &lhs, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator/( Real fScalar, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator/( const ArrayVector4 &lhs, Real fScalar );

        inline friend ArrayVector4 operator/( ArrayReal fScalar, const ArrayVector4 &rhs );
        inline friend ArrayVector4 operator/( const ArrayVector4 &lhs, ArrayReal fScalar );

        inline void operator+=( const ArrayVector4 &a );
        inline void operator+=( const Real fScalar );
        inline void operator+=( const ArrayReal fScalar );

        inline void operator-=( const ArrayVector4 &a );
        inline void operator-=( const Real fScalar );
        inline void operator-=( const ArrayReal fScalar );

        inline void operator*=( const ArrayVector4 &a );
        inline void operator*=( const Real fScalar );
        inline void operator*=( const ArrayReal fScalar );

        inline void operator/=( const ArrayVector4 &a );
        inline void operator/=( const Real fScalar );
        inline void operator/=( const ArrayReal fScalar );

        /** Performs the following operation:
                this->xyz = (this->xyz + a) * m
                this->w = (this->w + a2) * m2
        @param m
            Scalar to multiply to the xyz components.
        @param a
            Scalar to add to the xyz components.
        @param m2
            Scalar to multiply to the w component.
        @param a2
            Scalar to add to the w component.
        */
        inline void fma3x1( ArrayReal m, ArrayReal a, ArrayReal m2, ArrayReal a2 );

        /// Does the same as Vector3::makeFloor (including the .w component).
        inline void makeFloor( const ArrayVector4 &cmp );

        /// Does the same as Vector3::makeCeil (including the .w component).
        inline void makeCeil( const ArrayVector4 &cmp );

        /** Calculates the inverse of the vectors: 1.0f / v;
            But if original is zero, the zero is left (0 / 0 = 0).
            Example:
            Bfore inverseLeaveZero:
                x = 0; y = 2; z = 4;
            After inverseLeaveZero
                x = 0; y = 0.5; z = 0.4444;
        */
        inline void inverseLeaveZeroes();

        /// @see Vector4::isNaN()
        /// @return
        ///     Return value differs from Vector4's counterpart. We return an int
        ///     bits 0-4 are set for each NaN of each vector inside.
        ///     if the int is non-zero, there is a NaN.
        inline int isNaN() const;

        /** Conditional move update.
            Changes each of the four vectors contained in 'this' with
            the replacement provided:

            this[i] = mask[i] != 0 ? this[i] : replacement[i]
            @see MathlibSSE2::Cmov4
            @remarks
                If mask param contains anything other than 0's or 0xffffffff's
                the result is undefined.
                Use this version if you want to decide whether to keep current
                result or overwrite with a replacement (performance optimization).
                i.e. a = Cmov4( a, b )
                If this vector hasn't been assigned yet any value and want to
                decide between two ArrayVector4s, i.e. a = Cmov4( b, c ) then
                see Cmov4( const ArrayVector4 &arg1, const ArrayVector4 &arg2, ArrayMaskR mask );
                instead.
            @param replacement
                Vectors to be used as replacement if the mask is zero.
            @param mask
                mask filled with either 0's or 0xFFFFFFFF
        */
        inline void Cmov4( ArrayMaskR mask, const ArrayVector4 &replacement );

        /** Conditional move update.
            Changes each of the four vectors contained in 'this' with
            the replacement provided:

            this[i] = mask[i] != 0 ? this[i] : replacement[i]
            @see MathlibSSE2::CmovRobust
            @remarks
                If mask param contains anything other than 0's or 0xffffffff's
                the result is undefined.
                Use this version if you want to decide whether to keep current
                result or overwrite with a replacement (performance optimization).
                i.e. a = CmovRobust( a, b )
                If this vector hasn't been assigned yet any value and want to
                decide between two ArrayVector4s, i.e. a = Cmov4( b, c ) then
                see Cmov4( const ArrayVector4 &arg1, const ArrayVector4 &arg2, ArrayMaskR mask );
                instead.
            @param replacement
                Vectors to be used as replacement if the mask is zero.
            @param mask
                mask filled with either 0's or 0xFFFFFFFF
        */
        inline void CmovRobust( ArrayMaskR mask, const ArrayVector4 &replacement );

        /** Conditional move.
            Selects between arg1 & arg2 according to mask:

            this[i] = mask[i] != 0 ? arg1[i] : arg2[i]
            @see MathlibSSE2::Cmov4
            @remarks
                If mask param contains anything other than 0's or 0xffffffff's
                the result is undefined.
                If you wanted to do a = cmov4( a, b ), then consider using the update version
                see Cmov4( ArrayMaskR mask, const ArrayVector4 &replacement );
                instead.
            @param arg1
                First array of Vectors
            @param arg2
                Second array of Vectors
            @param mask
                mask filled with either 0's or 0xFFFFFFFF
        */
        inline static ArrayVector4 Cmov4( const ArrayVector4 &arg1, const ArrayVector4 &arg2,
                                          ArrayMaskR mask );

        static const ArrayVector4 ZERO;
    };

    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreArrayVector4SSE2.inl"

#endif
