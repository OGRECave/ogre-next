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

#ifndef __Plane_H__
#define __Plane_H__

#include "OgrePrerequisites.h"

#include "OgreVector3.h"

namespace Ogre
{
    /** \addtogroup Core
     *  @{
     */
    /** \addtogroup Math
     *  @{
     */
    /** Defines a plane in 3D space.
        @remarks
            A plane is defined in 3D space by the equation
            Ax + By + Cz + D = 0
        @par
            This equates to a vector (the normal of the plane, whose x, y
            and z components equate to the coefficients A, B and C
            respectively), and a constant (D) which is the distance along
            the normal you have to go to move the plane back to the origin.
     */
    class _OgreExport Plane
    {
    public:
        Vector3 normal;
        Real    d;

    public:
        /** Default constructor - sets everything to 0.
         */
        Plane() : normal( Vector3::ZERO ), d( 0.0f ) {}
        /** Construct a plane through a normal, and a distance to move the plane along the normal.*/
        Plane( const Vector3 &rkNormal, Real fConstant ) : normal( rkNormal ), d( -fConstant ) {}
        /** Construct a plane using the 4 constants directly **/
        Plane( Real a, Real b, Real c, Real _d ) : normal( a, b, c ), d( _d ) {}
        Plane( const Vector3 &rkNormal, const Vector3 &rkPoint ) { redefine( rkNormal, rkPoint ); }
        Plane( const Vector3 &p0, const Vector3 &p1, const Vector3 &p2 ) { redefine( p0, p1, p2 ); }

        /** The "positive side" of the plane is the half space to which the
            plane normal points. The "negative side" is the other half
            space. The flag "no side" indicates the plane itself.
        */
        enum Side
        {
            NO_SIDE,
            POSITIVE_SIDE,
            NEGATIVE_SIDE,
            BOTH_SIDE
        };

        Side getSide( const Vector3 &rkPoint ) const
        {
            Real fDistance = getDistance( rkPoint );

            if( fDistance < 0.0 )
                return Plane::NEGATIVE_SIDE;

            if( fDistance > 0.0 )
                return Plane::POSITIVE_SIDE;

            return Plane::NO_SIDE;
        }

        /**
        Returns the side where the alignedBox is. The flag BOTH_SIDE indicates an intersecting box.
        One corner ON the plane is sufficient to consider the box and the plane intersecting.
        */
        Side getSide( const AxisAlignedBox &rkBox ) const;

        /** Returns which side of the plane that the given box lies on.
            The box is defined as centre/half-size pairs for effectively.
        @param centre The centre of the box.
        @param halfSize The half-size of the box.
        @return
            POSITIVE_SIDE if the box complete lies on the "positive side" of the plane,
            NEGATIVE_SIDE if the box complete lies on the "negative side" of the plane,
            and BOTH_SIDE if the box intersects the plane.
        */
        Side getSide( const Vector3 &centre, const Vector3 &halfSize ) const;

        /** This is a pseudodistance. The sign of the return value is
            positive if the point is on the positive side of the plane,
            negative if the point is on the negative side, and zero if the
            point is on the plane.
            @par
            The absolute value of the return value is the true distance only
            when the plane normal is a unit length vector.
        */
        Real getDistance( const Vector3 &rkPoint ) const { return normal.dotProduct( rkPoint ) + d; }

        /** Redefine this plane based on 3 points. */
        void redefine( const Vector3 &p0, const Vector3 &p1, const Vector3 &p2 )
        {
            normal = ( p1 - p0 ).crossProduct( p2 - p0 ).normalisedCopy();
            d = -normal.dotProduct( p0 );
        }

        /** Redefine this plane based on a normal and a point. */
        void redefine( const Vector3 &rkNormal, const Vector3 &rkPoint )
        {
            normal = rkNormal;
            d = -rkNormal.dotProduct( rkPoint );
        }

        /** Project a vector onto the plane.
        @remarks This gives you the element of the input vector that is perpendicular
            to the normal of the plane. You can get the element which is parallel
            to the normal of the plane by subtracting the result of this method
            from the original vector, since parallel + perpendicular = original.
        @param v The input vector
        */
        Vector3 projectVector( const Vector3 &v ) const;

        /** Normalises the plane.
            @remarks
                This method normalises the plane's normal and the length scale of d
                is as well.
            @note
                This function will not crash for zero-sized vectors, but there
                will be no changes made to their components.
            @return The previous length of the plane's normal.
        */
        Real normalise();

        /// Get flipped plane, with same location but reverted orientation
        Plane operator-() const
        {
            return Plane( -( normal.x ), -( normal.y ), -( normal.z ),
                          -d );  // not equal to Plane(-normal, -d)
        }

        /// Comparison operator
        bool operator==( const Plane &rhs ) const { return ( rhs.d == d && rhs.normal == normal ); }
        bool operator!=( const Plane &rhs ) const { return ( rhs.d != d || rhs.normal != normal ); }

        _OgreExport friend std::ostream &operator<<( std::ostream &o, const Plane &p );
    };

    typedef StdVector<Plane> PlaneList;
    /** @} */
    /** @} */

}  // namespace Ogre

#endif
