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
#include "OgreStableHeaders.h"

#include "OgrePlane.h"

#include "OgreAxisAlignedBox.h"
#include "OgreMatrix3.h"

namespace Ogre
{
    //-----------------------------------------------------------------------
    Plane::Side Plane::getSide( const AxisAlignedBox &box ) const
    {
        if( box.isNull() )
            return NO_SIDE;
        if( box.isInfinite() )
            return BOTH_SIDE;

        return getSide( box.getCenter(), box.getHalfSize() );
    }
    //-----------------------------------------------------------------------
    Plane::Side Plane::getSide( const Vector3 &centre, const Vector3 &halfSize ) const
    {
        // Calculate the distance between box centre and the plane
        Real dist = getDistance( centre );

        // Calculate the maximise allows absolute distance for
        // the distance between box centre and plane
        Real maxAbsDist = normal.absDotProduct( halfSize );

        if( dist < -maxAbsDist )
            return Plane::NEGATIVE_SIDE;

        if( dist > +maxAbsDist )
            return Plane::POSITIVE_SIDE;

        return Plane::BOTH_SIDE;
    }
    //-----------------------------------------------------------------------
    Vector3 Plane::projectVector( const Vector3 &p ) const
    {
        // We know plane normal is unit length, so use simple method
        Matrix3 xform;
        xform[0][0] = 1.0f - normal.x * normal.x;
        xform[0][1] = -normal.x * normal.y;
        xform[0][2] = -normal.x * normal.z;
        xform[1][0] = -normal.y * normal.x;
        xform[1][1] = 1.0f - normal.y * normal.y;
        xform[1][2] = -normal.y * normal.z;
        xform[2][0] = -normal.z * normal.x;
        xform[2][1] = -normal.z * normal.y;
        xform[2][2] = 1.0f - normal.z * normal.z;
        return xform * p;
    }
    //-----------------------------------------------------------------------
    Real Plane::normalise()
    {
        Real fLength = normal.length();

        // Will also work for zero-sized vectors, but will change nothing
        // We're not using epsilons because we don't need to.
        // Read http://www.ogre3d.org/forums/viewtopic.php?f=4&t=61259
        if( fLength > Real( 0.0f ) )
        {
            Real fInvLength = 1.0f / fLength;
            normal *= fInvLength;
            d *= fInvLength;
        }

        return fLength;
    }
}  // namespace Ogre
