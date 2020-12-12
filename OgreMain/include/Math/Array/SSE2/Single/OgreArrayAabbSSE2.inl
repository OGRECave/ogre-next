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

namespace Ogre
{
    //-----------------------------------------------------------------------------------
    inline ArrayVector3 ArrayAabb::getMinimum() const
    {
        return mCenter - mHalfSize;
    }
    //-----------------------------------------------------------------------------------
    inline ArrayVector3 ArrayAabb::getMaximum() const
    {
        return mCenter + mHalfSize;
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayAabb::merge( const ArrayAabb& rhs )
    {
        ArrayVector3 max( mCenter + mHalfSize );
        max.makeCeil( rhs.mCenter + rhs.mHalfSize );

        ArrayVector3 min( mCenter - mHalfSize );
        min.makeFloor( rhs.mCenter - rhs.mHalfSize );

        mCenter = ( max + min ) * Mathlib::HALF;
        mHalfSize   = ( max - min ) * Mathlib::HALF;
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayAabb::merge( const ArrayVector3& points )
    {
        ArrayVector3 max( mCenter + mHalfSize );
        max.makeCeil( points );

        ArrayVector3 min( mCenter - mHalfSize );
        min.makeFloor( points );

        mCenter = ( max + min ) * Mathlib::HALF;
        mHalfSize   = ( max - min ) * Mathlib::HALF;
    }
    //-----------------------------------------------------------------------------------
    inline ArrayMaskR ArrayAabb::intersects( const ArrayAabb& b2 ) const
    {
        ArrayVector3 dist( mCenter - b2.mCenter );
        ArrayVector3 sumHalfSizes( mHalfSize + b2.mHalfSize );

        // ( abs( center.x - center2.x ) <= halfSize.x + halfSize2.x &&
        //   abs( center.y - center2.y ) <= halfSize.y + halfSize2.y &&
        //   abs( center.z - center2.z ) <= halfSize.z + halfSize2.z )
        ArrayReal maskX = _mm_cmple_ps( MathlibSSE2::Abs4( dist.mChunkBase[0] ),
                                        sumHalfSizes.mChunkBase[0] );
        ArrayReal maskY = _mm_cmple_ps( MathlibSSE2::Abs4( dist.mChunkBase[1] ),
                                        sumHalfSizes.mChunkBase[1] );
        ArrayReal maskZ = _mm_cmple_ps( MathlibSSE2::Abs4( dist.mChunkBase[2] ),
                                        sumHalfSizes.mChunkBase[2] );
        
        return _mm_and_ps( _mm_and_ps( maskX, maskY ), maskZ );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayAabb::volume(void) const
    {
        ArrayReal w = _mm_add_ps( mHalfSize.mChunkBase[0], mHalfSize.mChunkBase[0] ); // x * 2
        ArrayReal h = _mm_add_ps( mHalfSize.mChunkBase[1], mHalfSize.mChunkBase[1] ); // y * 2
        ArrayReal d = _mm_add_ps( mHalfSize.mChunkBase[2], mHalfSize.mChunkBase[2] ); // z * 2

        return _mm_mul_ps( _mm_mul_ps( w, h ), d ); // w * h * d
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayAabb::contains( const ArrayAabb &other ) const
    {
        ArrayVector3 dist( mCenter - other.mCenter );

        // In theory, "abs( dist.x ) < mHalfSize - other.mHalfSize" should be more pipeline-
        // friendly because the processor can do the subtraction while the abs4() is being performed,
        // however that variation won't handle the case where both boxes are infinite (will produce
        // nan instead and return false, when it should return true)

        // ( abs( dist.x ) + other.mHalfSize.x <= mHalfSize.x &&
        //   abs( dist.y ) + other.mHalfSize.y <= mHalfSize.y &&
        //   abs( dist.z ) + other.mHalfSize.z <= mHalfSize.z )
        ArrayReal maskX = _mm_cmple_ps( _mm_add_ps( MathlibSSE2::Abs4( dist.mChunkBase[0] ),
                                        other.mHalfSize.mChunkBase[0] ), mHalfSize.mChunkBase[0] );
        ArrayReal maskY = _mm_cmple_ps( _mm_add_ps( MathlibSSE2::Abs4( dist.mChunkBase[1] ),
                                        other.mHalfSize.mChunkBase[1] ), mHalfSize.mChunkBase[1] );
        ArrayReal maskZ = _mm_cmple_ps( _mm_add_ps( MathlibSSE2::Abs4( dist.mChunkBase[2] ),
                                        other.mHalfSize.mChunkBase[2] ), mHalfSize.mChunkBase[2] );

        return _mm_and_ps( _mm_and_ps( maskX, maskY ), maskZ );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayAabb::contains( const ArrayVector3 &v ) const
    {
        ArrayVector3 dist( mCenter - v );

        // ( abs( dist.x ) <= mHalfSize.x &&
        //   abs( dist.y ) <= mHalfSize.y &&
        //   abs( dist.z ) <= mHalfSize.z )
        ArrayReal maskX = _mm_cmple_ps( MathlibSSE2::Abs4( dist.mChunkBase[0] ),
                                        mHalfSize.mChunkBase[0] );
        ArrayReal maskY = _mm_cmple_ps( MathlibSSE2::Abs4( dist.mChunkBase[1] ),
                                        mHalfSize.mChunkBase[1] );
        ArrayReal maskZ = _mm_cmple_ps( MathlibSSE2::Abs4( dist.mChunkBase[2] ),
                                        mHalfSize.mChunkBase[2] );

        return _mm_and_ps( _mm_and_ps( maskX, maskY ), maskZ );
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayAabb::squaredDistance( const ArrayVector3 &v ) const
    {
        ArrayVector3 dist( mCenter - v );

        // x = max( abs( dist.x ) - halfSize.x, 0 )
        // y = max( abs( dist.y ) - halfSize.y, 0 )
        // z = max( abs( dist.z ) - halfSize.z, 0 )
        // return squaredLength( x, y, z );
        dist.mChunkBase[0] =
            _mm_sub_ps( MathlibSSE2::Abs4( dist.mChunkBase[0] ), mHalfSize.mChunkBase[0] );
        dist.mChunkBase[1] =
            _mm_sub_ps( MathlibSSE2::Abs4( dist.mChunkBase[1] ), mHalfSize.mChunkBase[1] );
        dist.mChunkBase[2] =
            _mm_sub_ps( MathlibSSE2::Abs4( dist.mChunkBase[2] ), mHalfSize.mChunkBase[2] );

        dist.mChunkBase[0] = _mm_max_ps( dist.mChunkBase[0], _mm_setzero_ps() );
        dist.mChunkBase[1] = _mm_max_ps( dist.mChunkBase[1], _mm_setzero_ps() );
        dist.mChunkBase[2] = _mm_max_ps( dist.mChunkBase[2], _mm_setzero_ps() );

        return dist.squaredLength();
    }
    //-----------------------------------------------------------------------------------
    inline ArrayReal ArrayAabb::distance( const ArrayVector3 &v ) const
    {
        return _mm_sqrt_ps( squaredDistance( v ) );
    }
    //-----------------------------------------------------------------------------------
    inline void ArrayAabb::transformAffine( const ArrayMatrix4 &m )
    {
        assert( m.isAffine() );

        mCenter = m * mCenter;

        ArrayReal x = _mm_mul_ps( Mathlib::Abs4( m.mChunkBase[2] ), mHalfSize.mChunkBase[2] );  // abs( m02 ) * z +
        x = _mm_madd_ps( Mathlib::Abs4( m.mChunkBase[1] ), mHalfSize.mChunkBase[1], x );            // abs( m01 ) * y +
        x = _mm_madd_ps( Mathlib::Abs4( m.mChunkBase[0] ), mHalfSize.mChunkBase[0], x );            // abs( m00 ) * x

        ArrayReal y = _mm_mul_ps( Mathlib::Abs4( m.mChunkBase[6] ), mHalfSize.mChunkBase[2] );  // abs( m12 ) * z +
        y = _mm_madd_ps( Mathlib::Abs4( m.mChunkBase[5] ), mHalfSize.mChunkBase[1], y );            // abs( m11 ) * y +
        y = _mm_madd_ps( Mathlib::Abs4( m.mChunkBase[4] ), mHalfSize.mChunkBase[0], y );            // abs( m10 ) * x

        ArrayReal z = _mm_mul_ps( Mathlib::Abs4( m.mChunkBase[10] ), mHalfSize.mChunkBase[2] ); // abs( m22 ) * z +
        z = _mm_madd_ps( Mathlib::Abs4( m.mChunkBase[9] ), mHalfSize.mChunkBase[1], z );            // abs( m21 ) * y +
        z = _mm_madd_ps( Mathlib::Abs4( m.mChunkBase[8] ), mHalfSize.mChunkBase[0], z );            // abs( m20 ) * x

        //Handle infinity & null boxes becoming NaN; leaving the original value instead.
        x = MathlibSSE2::CmovRobust( mHalfSize.mChunkBase[0], x,
                                     _mm_cmpeq_ps( Mathlib::Abs4(mHalfSize.mChunkBase[0]),
                                                   Mathlib::INFINITEA ) );
        y = MathlibSSE2::CmovRobust( mHalfSize.mChunkBase[1], y,
                                     _mm_cmpeq_ps( Mathlib::Abs4(mHalfSize.mChunkBase[1]),
                                                   Mathlib::INFINITEA ) );
        z = MathlibSSE2::CmovRobust( mHalfSize.mChunkBase[2], z,
                                     _mm_cmpeq_ps( Mathlib::Abs4(mHalfSize.mChunkBase[2]),
                                                   Mathlib::INFINITEA ) );

        mHalfSize = ArrayVector3( x, y, z );
    }
    //-----------------------------------------------------------------------------------
}
