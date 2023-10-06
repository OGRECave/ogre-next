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
#ifndef Ogre_C_ArrayVector2_H
#define Ogre_C_ArrayVector2_H

#ifndef OgreArrayVector2_H
#    error "Don't include this file directly. include Math/Array/OgreArrayVector2.h"
#endif

#include "OgreVector2.h"

#include "Math/Array/OgreMathlib.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Math
     *  @{
     */
    /** Cache-friendly array of 2-dimensional represented as a SoA array.
        @remarks
            ArrayVector2 is a SIMD & cache-friendly version of Vector2.
            An operation on an ArrayVector2 is done on 4 vectors at a
            time (the actual amount is defined by ARRAY_PACKED_REALS)
            Assuming ARRAY_PACKED_REALS == 4, the memory layout will
            be as following:
             mChunkBase     mChunkBase + 2
            XXXX YYYY       XXXX YYYY
            Extracting one vector (XY) needs 32 bytes, which is within
            the 64 byte size of common cache lines.
            Architectures where the cache line == 32 bytes may want to
            set ARRAY_PACKED_REALS = 2 depending on their needs
    */

    class _OgreExport ArrayVector2
    {
    public:
        ArrayReal mChunkBase[2];

        ArrayVector2() {}
        ArrayVector2( ArrayReal chunkX, ArrayReal chunkY )
        {
            mChunkBase[0] = chunkX;
            mChunkBase[1] = chunkY;
        }

        void getAsVector2( Vector2 &out, size_t index ) const
        {
            // Be careful of not writing to these regions or else strict aliasing rule gets broken!!!
            const Real *aliasedReal = reinterpret_cast<const Real *>( mChunkBase );
            out.x = aliasedReal[ARRAY_PACKED_REALS * 0 + index];  // X
            out.y = aliasedReal[ARRAY_PACKED_REALS * 1 + index];  // Y
        }

        /// Prefer using @see getAsVector2() because this function may have more
        /// overhead (the other one is faster)
        Vector2 getAsVector2( size_t index ) const
        {
            // Be careful of not writing to these regions or else strict aliasing rule gets broken!!!
            const Real *aliasedReal = reinterpret_cast<const Real *>( mChunkBase );
            return Vector2( aliasedReal[ARRAY_PACKED_REALS * 0 + index],    // X
                            aliasedReal[ARRAY_PACKED_REALS * 1 + index] );  // Y
        }

        void setFromVector2( const Vector2 &v, size_t index )
        {
            Real *aliasedReal = reinterpret_cast<Real *>( mChunkBase );
            aliasedReal[ARRAY_PACKED_REALS * 0 + index] = v.x;
            aliasedReal[ARRAY_PACKED_REALS * 1 + index] = v.y;
        }

        /// Sets all packed vectors to the same value as the scalar input vector
        void setAll( const Vector2 &v )
        {
            mChunkBase[0] = v.x;
            mChunkBase[1] = v.y;
        }

        inline ArrayVector2 &operator=( const Real fScalar )
        {
            mChunkBase[0] = fScalar;
            mChunkBase[1] = fScalar;

            return *this;
        }

        // Arithmetic operations
        inline const ArrayVector2 &operator+() const;
        inline ArrayVector2        operator-() const;

        inline friend ArrayVector2 operator+( const ArrayVector2 &lhs, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator+( Real fScalar, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator+( const ArrayVector2 &lhs, Real fScalar );

        inline friend ArrayVector2 operator+( ArrayReal fScalar, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator+( const ArrayVector2 &lhs, ArrayReal fScalar );

        inline friend ArrayVector2 operator-( const ArrayVector2 &lhs, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator-( Real fScalar, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator-( const ArrayVector2 &lhs, Real fScalar );

        inline friend ArrayVector2 operator-( ArrayReal fScalar, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator-( const ArrayVector2 &lhs, ArrayReal fScalar );

        inline friend ArrayVector2 operator*( const ArrayVector2 &lhs, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator*( Real fScalar, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator*( const ArrayVector2 &lhs, Real fScalar );

        inline friend ArrayVector2 operator*( ArrayReal fScalar, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator*( const ArrayVector2 &lhs, ArrayReal fScalar );

        inline friend ArrayVector2 operator/( const ArrayVector2 &lhs, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator/( Real fScalar, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator/( const ArrayVector2 &lhs, Real fScalar );

        inline friend ArrayVector2 operator/( ArrayReal fScalar, const ArrayVector2 &rhs );
        inline friend ArrayVector2 operator/( const ArrayVector2 &lhs, ArrayReal fScalar );

        inline void operator+=( const ArrayVector2 &a );
        inline void operator+=( const ArrayReal fScalar );

        inline void operator-=( const ArrayVector2 &a );
        inline void operator-=( const ArrayReal fScalar );

        inline void operator*=( const ArrayVector2 &a );
        inline void operator*=( const ArrayReal fScalar );

        inline void operator/=( const ArrayVector2 &a );
        inline void operator/=( const ArrayReal fScalar );

        /// @copydoc Vector2::length()
        inline ArrayReal length() const;

        /// @copydoc Vector2::squaredLength()
        inline ArrayReal squaredLength() const;

        /// @copydoc Vector2::distance()
        inline ArrayReal distance( const ArrayVector2 &rhs ) const;

        /// @copydoc Vector2::squaredDistance()
        inline ArrayReal squaredDistance( const ArrayVector2 &rhs ) const;

        /// @copydoc Vector2::dotProduct()
        inline ArrayReal dotProduct( const ArrayVector2 &vec ) const;

        /// Returns the absolute of the dotProduct().
        inline ArrayReal absDotProduct( const ArrayVector2 &vec ) const;

        /// Unlike Vector2::normalise(), this function does not return the length of the vector
        /// because such value was not cached and was never available @see Vector2::normalise()
        inline void normalise();

        /// @copydoc Vector2::crossProduct()
        inline ArrayReal crossProduct( const ArrayVector2 &rkVector ) const;

        /// @copydoc Vector2::midPoint()
        inline ArrayVector2 midPoint( const ArrayVector2 &vec ) const;

        /// @copydoc Vector2::makeFloor()
        inline void makeFloor( const ArrayVector2 &cmp );

        /// @copydoc Vector2::makeCeil()
        inline void makeCeil( const ArrayVector2 &cmp );

        /// Returns the smallest value between x, y; min( x, y )
        inline ArrayReal getMinComponent() const;

        /// Returns the biggest value between x, y; max( x, y )
        inline ArrayReal getMaxComponent() const;

        /** Converts the vector to (sign(x), sign(y), sign(z))
        @remarks
            For reference, sign( x ) = x >= 0 ? 1.0 : -1.0
            sign( -0.0f ) may return 1 or -1 depending on implementation
            @par
            SSE2 implementation: Does distinguish between -0 & 0
            C implementation: Does not distinguish between -0 & 0
        */
        inline void setToSign();

        /// @copydoc Vector2::perpendicular()
        inline ArrayVector2 perpendicular() const;

        /// @copydoc Vector2::normalisedCopy()
        inline ArrayVector2 normalisedCopy() const;

        /// @copydoc Vector2::reflect()
        inline ArrayVector2 reflect( const ArrayVector2 &normal ) const;

        /** Calculates the inverse of the vectors: 1.0f / v;
            But if original is zero, the zero is left (0 / 0 = 0).
            Example:
            Bfore inverseLeaveZero:
                x = 0; y = 2; z = 3;
            After inverseLeaveZero
                x = 0; y = 0.5; z = 0.3333;
        */
        inline void inverseLeaveZeroes();

        /// @see Vector2::isNaN()
        /// @return
        ///     Return value differs from Vector2's counterpart. We return an int
        ///     bits 0-4 are set for each NaN of each vector inside.
        ///     if the int is non-zero, there is a NaN.
        inline int isNaN() const;

        /** Takes each Vector and returns one returns a single vector
        @remarks
            This is useful when calculating bounding boxes, since it can be done independently
            in SIMD form, and once it is done, merge the results from the simd vectors into one
        @return
            Vector.x = min( vector[0].x, vector[1].x, vector[2].x, vector[3].x )
            Vector.y = min( vector[0].y, vector[1].y, vector[2].y, vector[3].y )
        */
        inline Vector2 collapseMin() const;

        /** Takes each Vector and returns one returns a single vector
        @remarks
            This is useful when calculating bounding boxes, since it can be done independently
            in SIMD form, and once it is done, merge the results from the simd vectors into one
        @return
            Vector.x = max( vector[0].x, vector[1].x, vector[2].x, vector[3].x )
            Vector.y = max( vector[0].y, vector[1].y, vector[2].y, vector[3].y )
        */
        inline Vector2 collapseMax() const;

        /** Conditional move update.
            Changes each of the four vectors contained in 'this' with
            the replacement provided:

            this[i] = mask[i] != 0 ? this[i] : replacement[i]
            @see MathlibC::Cmov4
            @remarks
                If mask param contains anything other than 0's or 0xffffffff's
                the result is undefined.
                Use this version if you want to decide whether to keep current
                result or overwrite with a replacement (performance optimization).
                i.e. a = Cmov4( a, b )
                If this vector hasn't been assigned yet any value and want to
                decide between two ArrayVector2s, i.e. a = Cmov4( b, c ) then
                see Cmov4( const ArrayVector2 &arg1, const ArrayVector2 &arg2, ArrayMaskR mask );
                instead.
            @param replacement
                Vectors to be used as replacement if the mask is zero.
            @param mask
                mask filled with either 0's or 0xFFFFFFFF
        */
        inline void Cmov4( ArrayMaskR mask, const ArrayVector2 &replacement );

        /** Conditional move update.
            Changes each of the four vectors contained in 'this' with
            the replacement provided:

            this[i] = mask[i] != 0 ? this[i] : replacement[i]
            @see MathlibC::CmovRobust
            @remarks
                If mask param contains anything other than 0's or 0xffffffff's
                the result is undefined.
                Use this version if you want to decide whether to keep current
                result or overwrite with a replacement (performance optimization).
                i.e. a = CmovRobust( a, b )
                If this vector hasn't been assigned yet any value and want to
                decide between two ArrayVector2s, i.e. a = Cmov4( b, c ) then
                see Cmov4( const ArrayVector2 &arg1, const ArrayVector2 &arg2, ArrayMaskR mask );
                instead.
            @param replacement
                Vectors to be used as replacement if the mask is zero.
            @param mask
                mask filled with either 0's or 0xFFFFFFFF
        */
        inline void CmovRobust( ArrayMaskR mask, const ArrayVector2 &replacement );

        /** Conditional move.
            Selects between arg1 & arg2 according to mask:

            this[i] = mask[i] != 0 ? arg1[i] : arg2[i]
            @see MathlibC::Cmov4
            @remarks
                If mask param contains anything other than 0's or 0xffffffff's
                the result is undefined.
                If you wanted to do a = cmov4( a, b ), then consider using the update version
                see Cmov4( ArrayMaskR mask, const ArrayVector2 &replacement );
                instead.
            @param arg1
                First array of Vectors
            @param arg2
                Second array of Vectors
            @param mask
                mask filled with either 0's or 0xFFFFFFFF
        */
        inline static ArrayVector2 Cmov4( const ArrayVector2 &arg1, const ArrayVector2 &arg2,
                                          ArrayMaskR mask );

        /** Converts 4 ARRAY_PACKED_REALS reals into this ArrayVector2
         @remarks
         'src' must be aligned and assumed to have enough memory for ARRAY_PACKED_REALS Vector2
         See Frustum::getCustomWorldSpaceCorners implementation for an actual, advanced use case.
         */
        inline void loadFromAoS( const Real *RESTRICT_ALIAS src );

        static const ArrayVector2 ZERO;
        static const ArrayVector2 UNIT_X;
        static const ArrayVector2 UNIT_Y;
        static const ArrayVector2 NEGATIVE_UNIT_X;
        static const ArrayVector2 NEGATIVE_UNIT_Y;
        static const ArrayVector2 UNIT_SCALE;
    };
    /** @} */
    /** @} */

}  // namespace Ogre

#include "OgreArrayVector2.inl"

#endif
