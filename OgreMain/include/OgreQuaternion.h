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
// This file is based on material originally from:
// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt

#ifndef __Quaternion_H__
#define __Quaternion_H__

#include "OgrePrerequisites.h"

#include "OgreMath.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Math
     *  @{
     */
    /** Implementation of a Quaternion, i.e. a rotation around an axis.
        For more information about Quaternions and the theory behind it, we recommend reading:
        http://www.ogre3d.org/tikiwiki/Quaternion+and+Rotation+Primer and
        http://www.cprogramming.com/tutorial/3d/quaternions.html and
        http://www.gamedev.net/page/resources/_/reference/programming/math-and-physics/quaternions/quaternion-powers-r1095
    */
    class _OgreExport Quaternion
    {
    public:
        /// Default constructor, initializes to identity rotation (aka 0°)
        inline Quaternion() : w( 1 ), x( 0 ), y( 0 ), z( 0 ) {}
        /// Construct from an explicit list of values
        inline Quaternion( Real fW, Real fX, Real fY, Real fZ ) : w( fW ), x( fX ), y( fY ), z( fZ ) {}
        /// Construct a quaternion from a rotation matrix
        inline Quaternion( const Matrix3 &rot ) { this->FromRotationMatrix( rot ); }
        /// Construct a quaternion from an angle/axis
        inline Quaternion( const Radian &rfAngle, const Vector3 &rkAxis )
        {
            this->FromAngleAxis( rfAngle, rkAxis );
        }
        /// Construct a quaternion from 3 orthonormal local axes
        inline Quaternion( const Vector3 &xaxis, const Vector3 &yaxis, const Vector3 &zaxis )
        {
            this->FromAxes( xaxis, yaxis, zaxis );
        }
        /// Construct a quaternion from 3 orthonormal local axes
        inline Quaternion( const Vector3 *akAxis ) { this->FromAxes( akAxis ); }
        /// Construct a quaternion from 4 manual w/x/y/z values
        inline Quaternion( Real *valptr ) { memcpy( &w, valptr, sizeof( Real ) * 4 ); }

        /** Exchange the contents of this quaternion with another.
         */
        inline void swap( Quaternion &other )
        {
            std::swap( w, other.w );
            std::swap( x, other.x );
            std::swap( y, other.y );
            std::swap( z, other.z );
        }

        /// Array accessor operator
        inline Real operator[]( const size_t i ) const
        {
            assert( i < 4 );

            return *( &w + i );
        }

        /// Array accessor operator
        inline Real &operator[]( const size_t i )
        {
            assert( i < 4 );

            return *( &w + i );
        }

        /// Pointer accessor for direct copying
        inline Real *ptr() { return &w; }

        /// Pointer accessor for direct copying
        inline const Real *ptr() const { return &w; }

        void FromRotationMatrix( const Matrix3 &kRot );
        void ToRotationMatrix( Matrix3 &kRot ) const;
        /** Setups the quaternion using the supplied vector, and "roll" around
            that vector by the specified radians.
        */
        void        FromAngleAxis( const Radian &rfAngle, const Vector3 &rkAxis );
        void        ToAngleAxis( Radian &rfAngle, Vector3 &rkAxis ) const;
        inline void ToAngleAxis( Degree &dAngle, Vector3 &rkAxis ) const
        {
            Radian rAngle;
            ToAngleAxis( rAngle, rkAxis );
            dAngle = rAngle;
        }
        /** Constructs the quaternion using 3 axes, the axes are assumed to be orthonormal
            @see FromAxes
        */
        void FromAxes( const Vector3 *akAxis );
        void FromAxes( const Vector3 &xAxis, const Vector3 &yAxis, const Vector3 &zAxis );
        /** Gets the 3 orthonormal axes defining the quaternion. @see FromAxes */
        void ToAxes( Vector3 *akAxis ) const;
        void ToAxes( Vector3 &xAxis, Vector3 &yAxis, Vector3 &zAxis ) const;

        /** Returns the X orthonormal axis defining the quaternion. Same as doing
            xAxis = Vector3::UNIT_X * this. Also called the local X-axis
        */
        Vector3 xAxis() const;

        /** Returns the Y orthonormal axis defining the quaternion. Same as doing
            yAxis = Vector3::UNIT_Y * this. Also called the local Y-axis
        */
        Vector3 yAxis() const;

        /** Returns the Z orthonormal axis defining the quaternion. Same as doing
            zAxis = Vector3::UNIT_Z * this. Also called the local Z-axis
        */
        Vector3 zAxis() const;

        Quaternion                    operator+( const Quaternion &rkQ ) const;
        Quaternion                    operator-( const Quaternion &rkQ ) const;
        Quaternion                    operator*( const Quaternion &rkQ ) const;
        Quaternion                    operator*( Real fScalar ) const;
        _OgreExport friend Quaternion operator*( Real fScalar, const Quaternion &rkQ );
        Quaternion                    operator-() const;
        inline bool                   operator==( const Quaternion &rhs ) const
        {
            return ( rhs.x == x ) && ( rhs.y == y ) && ( rhs.z == z ) && ( rhs.w == w );
        }
        inline bool operator!=( const Quaternion &rhs ) const { return !operator==( rhs ); }
        // functions of a quaternion
        /// Returns the dot product of the quaternion
        Real Dot( const Quaternion &rkQ ) const;
        /* Returns the normal length of this quaternion.
            @note This does <b>not</b> alter any values.
        */
        Real Norm() const;
        /// Normalises this quaternion, and returns the previous length
        Real       normalise();
        Quaternion Inverse() const;      /// Apply to non-zero quaternion
        Quaternion UnitInverse() const;  /// Apply to unit-length quaternion
        Quaternion Exp() const;
        Quaternion Log() const;

        /// Rotation of a vector by a quaternion
        Vector3 operator*( const Vector3 &rkVector ) const;

        /** Calculate the local roll element of this quaternion.
        @param reprojectAxis By default the method returns the 'intuitive' result
            that is, if you projected the local Y of the quaternion onto the X and
            Y axes, the angle between them is returned. If set to false though, the
            result is the actual yaw that will be used to implement the quaternion,
            which is the shortest possible path to get to the same orientation and
             may involve less axial rotation.  The co-domain of the returned value is
             from -180 to 180 degrees.
        */
        Radian getRoll( bool reprojectAxis = true ) const;
        /** Calculate the local pitch element of this quaternion
        @param reprojectAxis By default the method returns the 'intuitive' result
            that is, if you projected the local Z of the quaternion onto the X and
            Y axes, the angle between them is returned. If set to true though, the
            result is the actual yaw that will be used to implement the quaternion,
            which is the shortest possible path to get to the same orientation and
            may involve less axial rotation.  The co-domain of the returned value is
            from -180 to 180 degrees.
        */
        Radian getPitch( bool reprojectAxis = true ) const;
        /** Calculate the local yaw element of this quaternion
        @param reprojectAxis By default the method returns the 'intuitive' result
            that is, if you projected the local Y of the quaternion onto the X and
            Z axes, the angle between them is returned. If set to true though, the
            result is the actual yaw that will be used to implement the quaternion,
            which is the shortest possible path to get to the same orientation and
            may involve less axial rotation. The co-domain of the returned value is
            from -180 to 180 degrees.
        */
        Radian getYaw( bool reprojectAxis = true ) const;

        /** Equality with tolerance (tolerance is max angle difference)
        @remark Both equals() and orientationEquals() measure the exact same thing.
                One measures the difference by angle, the other by a different, non-linear metric.
        */
        bool equals( const Quaternion &rhs, const Radian &tolerance ) const;

        /** Compare two quaternions which are assumed to be used as orientations.
        @remark Both equals() and orientationEquals() measure the exact same thing.
                One measures the difference by angle, the other by a different, non-linear metric.
        @return true if the two orientations are the same or very close, relative to the given tolerance.
            Slerp ( 0.75f, A, B ) != Slerp ( 0.25f, B, A );
            therefore be careful if your code relies in the order of the operands.
            This is specially important in IK animation.
        */
        inline bool orientationEquals( const Quaternion &other, Real tolerance = Real( 1e-3 ) ) const
        {
            Real d = this->Dot( other );
            return 1 - d * d < tolerance;
        }

        /** Performs Spherical linear interpolation between two quaternions, and returns the result.
            Slerp ( 0.0f, A, B ) = A
            Slerp ( 1.0f, A, B ) = B
            @return Interpolated quaternion
            @remarks
            Slerp has the proprieties of performing the interpolation at constant
            velocity, and being torque-minimal (unless shortestPath=false).
            However, it's NOT commutative, which means
            Slerp ( 0.75f, A, B ) != Slerp ( 0.25f, B, A );
            therefore be careful if your code relies in the order of the operands.
            This is specially important in IK animation.
        */
        static Quaternion Slerp( Real fT, const Quaternion &rkP, const Quaternion &rkQ,
                                 bool shortestPath = false );

        /** @see Slerp. It adds extra "spins" (i.e. rotates several times) specified
            by parameter 'iExtraSpins' while interpolating before arriving to the
            final values
        */
        static Quaternion SlerpExtraSpins( Real fT, const Quaternion &rkP, const Quaternion &rkQ,
                                           int iExtraSpins );

        /// Setup for spherical quadratic interpolation
        static void Intermediate( const Quaternion &rkQ0, const Quaternion &rkQ1, const Quaternion &rkQ2,
                                  Quaternion &rka, Quaternion &rkB );

        /// Spherical quadratic interpolation
        static Quaternion Squad( Real fT, const Quaternion &rkP, const Quaternion &rkA,
                                 const Quaternion &rkB, const Quaternion &rkQ,
                                 bool shortestPath = false );

        /** Performs Normalised linear interpolation between two quaternions, and returns the result.
            nlerp ( 0.0f, A, B ) = A
            nlerp ( 1.0f, A, B ) = B
            @remarks
            Nlerp is faster than Slerp.
            Nlerp has the proprieties of being commutative (@see Slerp;
            commutativity is desired in certain places, like IK animation), and
            being torque-minimal (unless shortestPath=false). However, it's performing
            the interpolation at non-constant velocity; sometimes this is desired,
            sometimes it is not. Having a non-constant velocity can produce a more
            natural rotation feeling without the need of tweaking the weights; however
            if your scene relies on the timing of the rotation or assumes it will point
            at a specific angle at a specific weight value, Slerp is a better choice.
        */
        static Quaternion nlerp( Real fT, const Quaternion &rkP, const Quaternion &rkQ,
                                 bool shortestPath = false );

        /// Cutoff for sine near zero
        static const Real msEpsilon;

        // special values
        static const Quaternion ZERO;
        static const Quaternion IDENTITY;

        Real w, x, y, z;

        /// Check whether this quaternion contains valid values
        inline bool isNaN() const
        {
            return Math::isNaN( x ) || Math::isNaN( y ) || Math::isNaN( z ) || Math::isNaN( w );
        }

        /** Function for writing to a stream. Outputs "Quaternion(w, x, y, z)" with w,x,y,z
            being the member values of the quaternion.
        */
        _OgreExport friend std::ostream &operator<<( std::ostream &o, const Quaternion &q );
    };
    /** @} */
    /** @} */

}  // namespace Ogre

#endif
