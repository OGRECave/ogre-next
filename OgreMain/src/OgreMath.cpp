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

#include "OgreMath.h"

#include "OgreAxisAlignedBox.h"
#include "OgreColourValue.h"
#include "OgreDualQuaternion.h"
#include "OgrePlane.h"
#include "OgreRay.h"
#include "OgreSphere.h"
#include "OgreVector2.h"
#include "OgreVector3.h"
#include "OgreVector4.h"

#include "ogrestd/list.h"

#include <sstream>

// #define _OGRE_USE_OLD_RAY_TRIANGLE_INTERSECTION // new Moller-Trumbore implementation is 3x faster but
//  less mature

namespace Ogre
{
    const Real Math::POS_INFINITY = std::numeric_limits<Real>::infinity();
    const Real Math::NEG_INFINITY = -std::numeric_limits<Real>::infinity();
    const Real Math::PI = Real( 3.14159265358979323846264338327950288 );  // enough even for float128
    const Real Math::TWO_PI = Real( 2.0 * PI );
    const Real Math::HALF_PI = Real( 0.5 * PI );
    const Real Math::fDeg2Rad = PI / Real( 180.0 );
    const Real Math::fRad2Deg = Real( 180.0 ) / PI;
    const Real Math::LOG2 = std::log( Real( 2.0 ) );

    int Math::mTrigTableSize;
    Math::AngleUnit Math::msAngleUnit;

    Real Math::mTrigTableFactor;
    Real *Math::mSinTable = NULL;
    Real *Math::mTanTable = NULL;

    Math::RandomValueProvider *Math::mRandProvider = NULL;

    //-----------------------------------------------------------------------
    Math::Math( unsigned int trigTableSize )
    {
        msAngleUnit = AU_DEGREE;
        mTrigTableSize = static_cast<int>( trigTableSize );
        mTrigTableFactor = Real( mTrigTableSize ) / Math::TWO_PI;

        mSinTable = OGRE_ALLOC_T( Real, trigTableSize, MEMCATEGORY_GENERAL );
        mTanTable = OGRE_ALLOC_T( Real, trigTableSize, MEMCATEGORY_GENERAL );

        buildTrigTables();
    }

    //-----------------------------------------------------------------------
    Math::~Math()
    {
        OGRE_FREE( mSinTable, MEMCATEGORY_GENERAL );
        OGRE_FREE( mTanTable, MEMCATEGORY_GENERAL );
    }

    //-----------------------------------------------------------------------
    void Math::buildTrigTables()
    {
        // Build trig lookup tables
        // Could get away with building only PI sized Sin table but simpler this
        // way. Who cares, it'll ony use an extra 8k of memory anyway and I like
        // simplicity.
        Real angle;
        for( int i = 0; i < mTrigTableSize; ++i )
        {
            angle = Math::TWO_PI * Real( i ) / Real( mTrigTableSize );
            mSinTable[i] = std::sin( angle );
            mTanTable[i] = std::tan( angle );
        }
    }
    //-----------------------------------------------------------------------
    Real Math::SinTable( Real fValue )
    {
        // Convert range to index values, wrap if required
        int idx;
        if( fValue >= 0 )
        {
            idx = int( fValue * mTrigTableFactor ) % mTrigTableSize;
        }
        else
        {
            idx = mTrigTableSize - ( int( -fValue * mTrigTableFactor ) % mTrigTableSize ) - 1;
        }

        return mSinTable[idx];
    }
    //-----------------------------------------------------------------------
    Real Math::TanTable( Real fValue )
    {
        // Convert range to index values, wrap if required
        int idx = int( fValue *= mTrigTableFactor ) % mTrigTableSize;
        return mTanTable[idx];
    }
    //-----------------------------------------------------------------------
    int Math::ISign( int iValue ) { return ( iValue > 0 ? +1 : ( iValue < 0 ? -1 : 0 ) ); }
    //-----------------------------------------------------------------------
    Radian Math::ACos( Real fValue )
    {
        if( -1.0 < fValue )
        {
            if( fValue < 1.0 )
                return Radian( std::acos( fValue ) );
            else
                return Radian( 0.0 );
        }
        else
        {
            OGRE_ASSERT_HIGH( PI != 0.0f &&
                              "Ogre library static variables are initialized out of order" );
            return Radian( PI );
        }
    }
    //-----------------------------------------------------------------------
    Radian Math::ASin( Real fValue )
    {
        OGRE_ASSERT_HIGH( HALF_PI != 0.0f &&
                          "Ogre library static variables are initialized out of order" );

        if( -1.0 < fValue )
        {
            if( fValue < 1.0 )
                return Radian( std::asin( fValue ) );
            else
                return Radian( HALF_PI );
        }
        else
        {
            return Radian( -HALF_PI );
        }
    }
    //-----------------------------------------------------------------------
    Real Math::Sign( Real fValue )
    {
        if( fValue > 0.0 )
            return 1.0;

        if( fValue < 0.0 )
            return -1.0;

        return 0.0;
    }
    //-----------------------------------------------------------------------
    /// Whether we're compiled as a static or shared lib, mCurrentRand must always be thread_local
    static thread_local uint32 gCurrentRand = 7525534u;
    static constexpr uint32 kLCRNG_Max = 0x3FFFFFFF;
    /// Returns a random number between 0 and 0x3FFFFFFF
    /// See https://en.wikipedia.org/wiki/Linear_congruential_generator
    static uint32_t LCRNG()
    {
        const uint32 newValue = 214013u * gCurrentRand + 2531011u;
        OGRE_ASSERT_MEDIUM( gCurrentRand != newValue );
        gCurrentRand = newValue;
        return newValue >> 2u;
    }
    Real Math::UnitRandom()
    {
        if( mRandProvider )
            return mRandProvider->getRandomUnit();
        else
            return Real( LCRNG() ) / Real( kLCRNG_Max );
    }
    //-----------------------------------------------------------------------
    Real Math::RangeRandom( Real fLow, Real fHigh ) { return ( fHigh - fLow ) * UnitRandom() + fLow; }

    //-----------------------------------------------------------------------
    Real Math::SymmetricRandom() { return 2.0f * UnitRandom() - 1.0f; }

    //-----------------------------------------------------------------------
    void Math::SetRandomValueProvider( RandomValueProvider *provider ) { mRandProvider = provider; }

    //-----------------------------------------------------------------------
    void Math::setAngleUnit( Math::AngleUnit unit ) { msAngleUnit = unit; }
    //-----------------------------------------------------------------------
    Math::AngleUnit Math::getAngleUnit() { return msAngleUnit; }
    //-----------------------------------------------------------------------
    Real Math::AngleUnitsToRadians( Real angleunits )
    {
        if( msAngleUnit == AU_DEGREE )
        {
            OGRE_ASSERT_HIGH( fDeg2Rad != 0.0f &&
                              "Ogre library static variables are initialized out of order" );
            return angleunits * fDeg2Rad;
        }
        else
            return angleunits;
    }

    //-----------------------------------------------------------------------
    Real Math::RadiansToAngleUnits( Real radians )
    {
        if( msAngleUnit == AU_DEGREE )
        {
            OGRE_ASSERT_HIGH( fRad2Deg != 0.0f &&
                              "Ogre library static variables are initialized out of order" );
            return radians * fRad2Deg;
        }
        else
            return radians;
    }

    //-----------------------------------------------------------------------
    Real Math::AngleUnitsToDegrees( Real angleunits )
    {
        if( msAngleUnit == AU_RADIAN )
        {
            OGRE_ASSERT_HIGH( fRad2Deg != 0.0f &&
                              "Ogre library static variables are initialized out of order" );
            return angleunits * fRad2Deg;
        }
        else
            return angleunits;
    }

    //-----------------------------------------------------------------------
    Real Math::DegreesToAngleUnits( Real degrees )
    {
        if( msAngleUnit == AU_RADIAN )
        {
            OGRE_ASSERT_HIGH( fDeg2Rad != 0.0f &&
                              "Ogre library static variables are initialized out of order" );
            return degrees * fDeg2Rad;
        }
        else
            return degrees;
    }
    //-----------------------------------------------------------------------
    Vector2 Math::octahedronMappingWrap( Vector2 v )
    {
        Vector2 retVal;
        retVal.x = ( Real( 1.0f ) - Abs( v.y ) ) * ( v.x >= 0 ? Real( 1.0f ) : -Real( 1.0f ) );
        retVal.y = ( Real( 1.0f ) - Abs( v.x ) ) * ( v.y >= 0 ? Real( 1.0f ) : -Real( 1.0f ) );
        return retVal;
    }
    //-----------------------------------------------------------------------
    Vector2 Math::octahedronMappingEncode( Vector3 n )
    {
        n /= ( abs( n.x ) + abs( n.y ) + abs( n.z ) );
        Vector2 nxy = n.z >= 0 ? n.xy() : octahedronMappingWrap( n.xy() );
        nxy = nxy * Real( 0.5f ) + Real( 0.5f );
        return nxy;
    }
    //-----------------------------------------------------------------------
    Vector3 Math::octahedronMappingDecode( Vector2 f )
    {
        f = f * Real( 2.0f ) - Real( 1.0f );

        // https://twitter.com/Stubbesaurus/status/937994790553227264
        Vector3 n = Vector3( f.x, f.y, Real( 1.0f ) - Abs( f.x ) - Abs( f.y ) );
        float t = saturate( -n.z );
        n.x += n.x >= 0 ? -t : t;
        n.y += n.y >= 0 ? -t : t;
        return n.normalisedCopy();
    }
    //-----------------------------------------------------------------------
    bool Math::pointInTri2D( const Vector2 &p, const Vector2 &a, const Vector2 &b, const Vector2 &c )
    {
        // Winding must be consistent from all edges for point to be inside
        Vector2 v1, v2;
        Real dot[3];
        bool zeroDot[3];

        v1 = b - a;
        v2 = p - a;

        // Note we don't care about normalisation here since sign is all we need
        // It means we don't have to worry about magnitude of cross products either
        dot[0] = v1.crossProduct( v2 );
        zeroDot[0] = Math::RealEqual( dot[0], 0.0f, Real( 1e-3 ) );

        v1 = c - b;
        v2 = p - b;

        dot[1] = v1.crossProduct( v2 );
        zeroDot[1] = Math::RealEqual( dot[1], 0.0f, Real( 1e-3 ) );

        // Compare signs (ignore colinear / coincident points)
        if( !zeroDot[0] && !zeroDot[1] && Math::Sign( dot[0] ) != Math::Sign( dot[1] ) )
        {
            return false;
        }

        v1 = a - c;
        v2 = p - c;

        dot[2] = v1.crossProduct( v2 );
        zeroDot[2] = Math::RealEqual( dot[2], 0.0f, Real( 1e-3 ) );
        // Compare signs (ignore colinear / coincident points)
        if( ( !zeroDot[0] && !zeroDot[2] && Math::Sign( dot[0] ) != Math::Sign( dot[2] ) ) ||
            ( !zeroDot[1] && !zeroDot[2] && Math::Sign( dot[1] ) != Math::Sign( dot[2] ) ) )
        {
            return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------
    bool Math::pointInTri3D( const Vector3 &p, const Vector3 &a, const Vector3 &b, const Vector3 &c,
                             const Vector3 &normal )
    {
        // Winding must be consistent from all edges for point to be inside
        Vector3 v1, v2;
        Real dot[3];
        bool zeroDot[3];

        v1 = b - a;
        v2 = p - a;

        // Note we don't care about normalisation here since sign is all we need
        // It means we don't have to worry about magnitude of cross products either
        dot[0] = v1.crossProduct( v2 ).dotProduct( normal );
        zeroDot[0] = Math::RealEqual( dot[0], 0.0f, Real( 1e-3 ) );

        v1 = c - b;
        v2 = p - b;

        dot[1] = v1.crossProduct( v2 ).dotProduct( normal );
        zeroDot[1] = Math::RealEqual( dot[1], 0.0f, Real( 1e-3 ) );

        // Compare signs (ignore colinear / coincident points)
        if( !zeroDot[0] && !zeroDot[1] && Math::Sign( dot[0] ) != Math::Sign( dot[1] ) )
        {
            return false;
        }

        v1 = a - c;
        v2 = p - c;

        dot[2] = v1.crossProduct( v2 ).dotProduct( normal );
        zeroDot[2] = Math::RealEqual( dot[2], 0.0f, Real( 1e-3 ) );
        // Compare signs (ignore colinear / coincident points)
        if( ( !zeroDot[0] && !zeroDot[2] && Math::Sign( dot[0] ) != Math::Sign( dot[2] ) ) ||
            ( !zeroDot[1] && !zeroDot[2] && Math::Sign( dot[1] ) != Math::Sign( dot[2] ) ) )
        {
            return false;
        }

        return true;
    }
    //-----------------------------------------------------------------------
    std::pair<bool, Real> Math::intersects( const Ray &ray, const Plane &plane )
    {
        Real denom = plane.normal.dotProduct( ray.getDirection() );
        if( Math::Abs( denom ) < std::numeric_limits<Real>::epsilon() )
        {
            // Parallel
            return std::pair<bool, Real>( false, (Real)0 );
        }
        else
        {
            Real nom = plane.normal.dotProduct( ray.getOrigin() ) + plane.d;
            Real t = -( nom / denom );
            return std::pair<bool, Real>( t >= 0, (Real)t );
        }
    }
    //-----------------------------------------------------------------------
    std::pair<bool, Real> Math::intersects( const Ray &ray, const StdVector<Plane> &planes,
                                            bool normalIsOutside )
    {
        StdList<Plane> planesList;
        for( StdVector<Plane>::const_iterator i = planes.begin(); i != planes.end(); ++i )
        {
            planesList.push_back( *i );
        }
        return intersects( ray, planesList, normalIsOutside );
    }
    //-----------------------------------------------------------------------
    std::pair<bool, Real> Math::intersects( const Ray &ray, const StdList<Plane> &planes,
                                            bool normalIsOutside )
    {
        list<Plane>::type::const_iterator planeit, planeitend;
        planeitend = planes.end();
        bool allInside = true;
        std::pair<bool, Real> ret;
        std::pair<bool, Real> end;
        ret.first = false;
        ret.second = 0.0f;
        end.first = false;
        end.second = 0;

        // derive side
        // NB we don't pass directly since that would require Plane::Side in
        // interface, which results in recursive includes since Math is so fundamental
        Plane::Side outside = normalIsOutside ? Plane::POSITIVE_SIDE : Plane::NEGATIVE_SIDE;

        for( planeit = planes.begin(); planeit != planeitend; ++planeit )
        {
            const Plane &plane = *planeit;
            // is origin outside?
            if( plane.getSide( ray.getOrigin() ) == outside )
            {
                allInside = false;
                // Test single plane
                std::pair<bool, Real> planeRes = ray.intersects( plane );
                if( planeRes.first )
                {
                    // Ok, we intersected
                    ret.first = true;
                    // Use the most distant result since convex volume
                    ret.second = std::max( ret.second, planeRes.second );
                }
                else
                {
                    ret.first = false;
                    ret.second = 0.0f;
                    return ret;
                }
            }
            else
            {
                std::pair<bool, Real> planeRes = ray.intersects( plane );
                if( planeRes.first )
                {
                    if( !end.first )
                    {
                        end.first = true;
                        end.second = planeRes.second;
                    }
                    else
                    {
                        end.second = std::min( planeRes.second, end.second );
                    }
                }
            }
        }

        if( allInside )
        {
            // Intersecting at 0 distance since inside the volume!
            ret.first = true;
            ret.second = 0.0f;
            return ret;
        }

        if( end.first )
        {
            if( end.second < ret.second )
            {
                ret.first = false;
                return ret;
            }
        }
        return ret;
    }
    //-----------------------------------------------------------------------
    std::pair<bool, Real> Math::intersects( const Ray &ray, const Sphere &sphere, bool discardInside )
    {
        const Vector3 &raydir = ray.getDirection();
        // Adjust ray origin relative to sphere center
        const Vector3 &rayorig = ray.getOrigin() - sphere.getCenter();
        Real radius = sphere.getRadius();

        // Check origin inside first
        if( rayorig.squaredLength() <= radius * radius && discardInside )
        {
            return std::pair<bool, Real>( true, (Real)0 );
        }

        // Mmm, quadratics
        // Build coeffs which can be used with std quadratic solver
        // ie t = (-b +/- sqrt(b*b + 4ac)) / 2a
        Real a = raydir.dotProduct( raydir );
        Real b = 2 * rayorig.dotProduct( raydir );
        Real c = rayorig.dotProduct( rayorig ) - radius * radius;

        // Calc determinant
        Real d = ( b * b ) - ( 4 * a * c );
        if( d < 0 )
        {
            // No intersection
            return std::pair<bool, Real>( false, (Real)0 );
        }
        else
        {
            // BTW, if d=0 there is one intersection, if d > 0 there are 2
            // But we only want the closest one, so that's ok, just use the
            // '-' version of the solver
            Real t = ( -b - Math::Sqrt( d ) ) / ( 2 * a );
            if( t < 0 )
                t = ( -b + Math::Sqrt( d ) ) / ( 2 * a );
            return std::pair<bool, Real>( true, (Real)t );
        }
    }
    //-----------------------------------------------------------------------
    std::pair<bool, Real> Math::intersects( const Ray &ray, const AxisAlignedBox &box )
    {
        if( box.isNull() )
            return std::pair<bool, Real>( false, (Real)0 );
        if( box.isInfinite() )
            return std::pair<bool, Real>( true, (Real)0 );

        Real lowt = 0.0f;
        Real t;
        bool hit = false;
        Vector3 hitpoint;
        const Vector3 &min = box.getMinimum();
        const Vector3 &max = box.getMaximum();
        const Vector3 &rayorig = ray.getOrigin();
        const Vector3 &raydir = ray.getDirection();

        // Check origin inside first
        if( rayorig > min && rayorig < max )
        {
            return std::pair<bool, Real>( true, (Real)0 );
        }

        // Check each face in turn, only check closest 3
        // Min x
        if( rayorig.x <= min.x && raydir.x > 0 )
        {
            t = ( min.x - rayorig.x ) / raydir.x;
            if( t >= 0 )
            {
                // Substitute t back into ray and check bounds and dist
                hitpoint = rayorig + raydir * t;
                if( hitpoint.y >= min.y && hitpoint.y <= max.y && hitpoint.z >= min.z &&
                    hitpoint.z <= max.z && ( !hit || t < lowt ) )
                {
                    hit = true;
                    lowt = t;
                }
            }
        }
        // Max x
        if( rayorig.x >= max.x && raydir.x < 0 )
        {
            t = ( max.x - rayorig.x ) / raydir.x;

            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if( hitpoint.y >= min.y && hitpoint.y <= max.y && hitpoint.z >= min.z &&
                hitpoint.z <= max.z && ( !hit || t < lowt ) )
            {
                hit = true;
                lowt = t;
            }
        }
        // Min y
        if( rayorig.y <= min.y && raydir.y > 0 )
        {
            t = ( min.y - rayorig.y ) / raydir.y;

            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if( hitpoint.x >= min.x && hitpoint.x <= max.x && hitpoint.z >= min.z &&
                hitpoint.z <= max.z && ( !hit || t < lowt ) )
            {
                hit = true;
                lowt = t;
            }
        }
        // Max y
        if( rayorig.y >= max.y && raydir.y < 0 )
        {
            t = ( max.y - rayorig.y ) / raydir.y;

            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if( hitpoint.x >= min.x && hitpoint.x <= max.x && hitpoint.z >= min.z &&
                hitpoint.z <= max.z && ( !hit || t < lowt ) )
            {
                hit = true;
                lowt = t;
            }
        }
        // Min z
        if( rayorig.z <= min.z && raydir.z > 0 )
        {
            t = ( min.z - rayorig.z ) / raydir.z;

            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if( hitpoint.x >= min.x && hitpoint.x <= max.x && hitpoint.y >= min.y &&
                hitpoint.y <= max.y && ( !hit || t < lowt ) )
            {
                hit = true;
                lowt = t;
            }
        }
        // Max z
        if( rayorig.z >= max.z && raydir.z < 0 )
        {
            t = ( max.z - rayorig.z ) / raydir.z;

            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if( hitpoint.x >= min.x && hitpoint.x <= max.x && hitpoint.y >= min.y &&
                hitpoint.y <= max.y && ( !hit || t < lowt ) )
            {
                hit = true;
                lowt = t;
            }
        }

        return std::pair<bool, Real>( hit, (Real)lowt );
    }
    //-----------------------------------------------------------------------
    bool Math::intersects( const Ray &ray, const AxisAlignedBox &box, Real *d1, Real *d2 )
    {
        if( box.isNull() )
            return false;

        if( box.isInfinite() )
        {
            if( d1 )
                *d1 = 0;
            if( d2 )
                *d2 = Math::POS_INFINITY;
            return true;
        }

        const Vector3 &min = box.getMinimum();
        const Vector3 &max = box.getMaximum();
        const Vector3 &rayorig = ray.getOrigin();
        const Vector3 &raydir = ray.getDirection();

        Vector3 absDir;
        absDir[0] = Math::Abs( raydir[0] );
        absDir[1] = Math::Abs( raydir[1] );
        absDir[2] = Math::Abs( raydir[2] );

        // Sort the axis, ensure check minimise floating error axis first
        size_t imax = 0u, imid = 1u, imin = 2u;
        if( absDir[0] < absDir[2] )
        {
            imax = 2;
            imin = 0;
        }
        if( absDir[1] < absDir[imin] )
        {
            imid = imin;
            imin = 1;
        }
        else if( absDir[1] > absDir[imax] )
        {
            imid = imax;
            imax = 1;
        }

        Real start = 0, end = Math::POS_INFINITY;

#define _CALC_AXIS( i ) \
    do \
    { \
        Real denom = 1 / raydir[i]; \
        Real newstart = ( min[i] - rayorig[i] ) * denom; \
        Real newend = ( max[i] - rayorig[i] ) * denom; \
        if( newstart > newend ) \
            std::swap( newstart, newend ); \
        if( newstart > end || newend < start ) \
            return false; \
        if( newstart > start ) \
            start = newstart; \
        if( newend < end ) \
            end = newend; \
    } while( 0 )

        // Check each axis in turn

        _CALC_AXIS( imax );

        if( absDir[imid] < std::numeric_limits<Real>::epsilon() )
        {
            // Parallel with middle and minimise axis, check bounds only
            if( rayorig[imid] < min[imid] || rayorig[imid] > max[imid] || rayorig[imin] < min[imin] ||
                rayorig[imin] > max[imin] )
                return false;
        }
        else
        {
            _CALC_AXIS( imid );

            if( absDir[imin] < std::numeric_limits<Real>::epsilon() )
            {
                // Parallel with minimise axis, check bounds only
                if( rayorig[imin] < min[imin] || rayorig[imin] > max[imin] )
                    return false;
            }
            else
            {
                _CALC_AXIS( imin );
            }
        }
#undef _CALC_AXIS

        if( d1 )
            *d1 = start;
        if( d2 )
            *d2 = end;

        return true;
    }
    //-----------------------------------------------------------------------
    std::pair<bool, Real> Math::intersects( const Ray &ray, const Vector3 &a, const Vector3 &b,
                                            const Vector3 &c, const Vector3 &normal, bool positiveSide,
                                            bool negativeSide )
    {
#ifdef _OGRE_USE_OLD_RAY_TRIANGLE_INTERSECTION
        //
        // Calculate intersection with plane.
        //
        Real t;
        {
            Real denom = normal.dotProduct( ray.getDirection() );

            // Check intersect side
            if( denom > +std::numeric_limits<Real>::epsilon() )
            {
                if( !negativeSide )
                    return std::pair<bool, Real>( false, (Real)0 );
            }
            else if( denom < -std::numeric_limits<Real>::epsilon() )
            {
                if( !positiveSide )
                    return std::pair<bool, Real>( false, (Real)0 );
            }
            else
            {
                // Parallel or triangle area is close to zero when
                // the plane normal not normalised.
                return std::pair<bool, Real>( false, (Real)0 );
            }

            t = normal.dotProduct( a - ray.getOrigin() ) / denom;

            if( t < 0 )
            {
                // Intersection is behind origin
                return std::pair<bool, Real>( false, (Real)0 );
            }
        }

        //
        // Calculate the largest area projection plane in X, Y or Z.
        //
        size_t i0, i1;
        {
            Real n0 = Math::Abs( normal[0] );
            Real n1 = Math::Abs( normal[1] );
            Real n2 = Math::Abs( normal[2] );

            i0 = 1;
            i1 = 2;
            if( n1 > n2 )
            {
                if( n1 > n0 )
                    i0 = 0;
            }
            else
            {
                if( n2 > n0 )
                    i1 = 0;
            }
        }

        //
        // Check the intersection point is inside the triangle.
        //
        {
            Real u1 = b[i0] - a[i0];
            Real v1 = b[i1] - a[i1];
            Real u2 = c[i0] - a[i0];
            Real v2 = c[i1] - a[i1];
            Real u0 = t * ray.getDirection()[i0] + ray.getOrigin()[i0] - a[i0];
            Real v0 = t * ray.getDirection()[i1] + ray.getOrigin()[i1] - a[i1];

            Real alpha = u0 * v2 - u2 * v0;
            Real beta = u1 * v0 - u0 * v1;
            Real area = u1 * v2 - u2 * v1;

            // epsilon to avoid float precision error
            const Real EPSILON = 1e-6f;

            Real tolerance = -EPSILON * area;

            if( area > 0 )
            {
                if( alpha < tolerance || beta < tolerance || alpha + beta > area - tolerance )
                    return std::pair<bool, Real>( false, (Real)0 );
            }
            else
            {
                if( alpha > tolerance || beta > tolerance || alpha + beta < area - tolerance )
                    return std::pair<bool, Real>( false, (Real)0 );
            }
        }

        return std::pair<bool, Real>( true, (Real)t );
#else
        // it is faster to use new implementation ignoring precalculated normal
        return intersects( ray, a, b, c, positiveSide, negativeSide );
#endif
    }
    //-----------------------------------------------------------------------
    std::pair<bool, Real> Math::intersects( const Ray &ray, const Vector3 &a, const Vector3 &b,
                                            const Vector3 &c, bool positiveSide, bool negativeSide )
    {
#ifdef _OGRE_USE_OLD_RAY_TRIANGLE_INTERSECTION
        Vector3 normal = calculateBasicFaceNormalWithoutNormalize( a, b, c );
        return intersects( ray, a, b, c, normal, positiveSide, negativeSide );
#else
        // Moller–Trumbore intersection algorithm, http://www.graphics.cornell.edu/pubs/1997/MT97.pdf
        const Real EPSILON = 1e-6f;
        Vector3 E1 = b - a;
        Vector3 E2 = c - a;
        Vector3 P = ray.getDirection().crossProduct( E2 );
        Real det = E1.dotProduct( P );

        // if determinant is near zero, ray lies in plane of triangle
        if( !( ( positiveSide && det > EPSILON ) || ( negativeSide && det < -EPSILON ) ) )
            return std::make_pair( false, (Real)0 );
        Real inv_det = 1.0f / det;

        // calculate u parameter and test bounds
        Vector3 T = ray.getOrigin() - a;
        Real u = T.dotProduct( P ) * inv_det;
        if( u < 0.0f || u > 1.0f )
            return std::make_pair( false, (Real)0 );

        // calculate v parameter and test bounds
        Vector3 Q = T.crossProduct( E1 );
        Real v = ray.getDirection().dotProduct( Q ) * inv_det;
        if( v < 0.0f || u + v > 1.0f )
            return std::make_pair( false, (Real)0 );

        // calculate t, ray intersects triangle
        Real t = E2.dotProduct( Q ) * inv_det;
        if( t < 0.0f )
            return std::make_pair( false, (Real)0 );

        return std::make_pair( true, t );
#endif
    }
    //-----------------------------------------------------------------------
    bool Math::intersects( const Sphere &sphere, const AxisAlignedBox &box )
    {
        if( box.isNull() )
            return false;
        if( box.isInfinite() )
            return true;

        // Use splitting planes
        const Vector3 &center = sphere.getCenter();
        Real radius = sphere.getRadius();
        const Vector3 &min = box.getMinimum();
        const Vector3 &max = box.getMaximum();

        // Arvo's algorithm
        Real s, d = 0;
        for( int i = 0; i < 3; ++i )
        {
            if( center.ptr()[i] < min.ptr()[i] )
            {
                s = center.ptr()[i] - min.ptr()[i];
                d += s * s;
            }
            else if( center.ptr()[i] > max.ptr()[i] )
            {
                s = center.ptr()[i] - max.ptr()[i];
                d += s * s;
            }
        }
        return d <= radius * radius;
    }
    //-----------------------------------------------------------------------
    bool Math::intersects( const Plane &plane, const AxisAlignedBox &box )
    {
        return ( plane.getSide( box ) == Plane::BOTH_SIDE );
    }
    //-----------------------------------------------------------------------
    bool Math::intersects( const Sphere &sphere, const Plane &plane )
    {
        return ( Math::Abs( plane.getDistance( sphere.getCenter() ) ) <= sphere.getRadius() );
    }
    //-----------------------------------------------------------------------
    Vector3 Math::calculateTangentSpaceVector( const Vector3 &position1, const Vector3 &position2,
                                               const Vector3 &position3, Real u1, Real v1, Real u2,
                                               Real v2, Real u3, Real v3 )
    {
        // side0 is the vector along one side of the triangle of vertices passed in,
        // and side1 is the vector along another side. Taking the cross product of these returns the
        // normal.
        Vector3 side0 = position1 - position2;
        Vector3 side1 = position3 - position1;
        // Calculate face normal
        Vector3 normal = side1.crossProduct( side0 );
        normal.normalise();
        // Now we use a formula to calculate the tangent.
        Real deltaV0 = v1 - v2;
        Real deltaV1 = v3 - v1;
        Vector3 tangent = deltaV1 * side0 - deltaV0 * side1;
        tangent.normalise();
        // Calculate binormal
        Real deltaU0 = u1 - u2;
        Real deltaU1 = u3 - u1;
        Vector3 binormal = deltaU1 * side0 - deltaU0 * side1;
        binormal.normalise();
        // Now, we take the cross product of the tangents to get a vector which
        // should point in the same direction as our normal calculated above.
        // If it points in the opposite direction (the dot product between the normals is less than
        // zero), then we need to reverse the s and t tangents. This is because the triangle has been
        // mirrored when going from tangent space to object space. reverse tangents if necessary
        Vector3 tangentCross = tangent.crossProduct( binormal );
        if( tangentCross.dotProduct( normal ) < 0.0f )
        {
            tangent = -tangent;
            binormal = -binormal;
        }

        return tangent;
    }
    //-----------------------------------------------------------------------
    Matrix4 Math::buildReflectionMatrix( const Plane &p )
    {
        return Matrix4( -2 * p.normal.x * p.normal.x + 1, -2 * p.normal.x * p.normal.y,
                        -2 * p.normal.x * p.normal.z, -2 * p.normal.x * p.d,

                        -2 * p.normal.y * p.normal.x, -2 * p.normal.y * p.normal.y + 1,
                        -2 * p.normal.y * p.normal.z, -2 * p.normal.y * p.d,

                        -2 * p.normal.z * p.normal.x, -2 * p.normal.z * p.normal.y,
                        -2 * p.normal.z * p.normal.z + 1, -2 * p.normal.z * p.d,

                        0, 0, 0, 1 );
    }
    //-----------------------------------------------------------------------
    Vector4 Math::calculateFaceNormal( const Vector3 &v1, const Vector3 &v2, const Vector3 &v3 )
    {
        Vector3 normal = calculateBasicFaceNormal( v1, v2, v3 );
        // Now set up the w (distance of tri from origin
        return Vector4( normal.x, normal.y, normal.z, -( normal.dotProduct( v1 ) ) );
    }
    //-----------------------------------------------------------------------
    Vector3 Math::calculateBasicFaceNormal( const Vector3 &v1, const Vector3 &v2, const Vector3 &v3 )
    {
        Vector3 normal = ( v2 - v1 ).crossProduct( v3 - v1 );
        normal.normalise();
        return normal;
    }
    //-----------------------------------------------------------------------
    Vector4 Math::calculateFaceNormalWithoutNormalize( const Vector3 &v1, const Vector3 &v2,
                                                       const Vector3 &v3 )
    {
        Vector3 normal = calculateBasicFaceNormalWithoutNormalize( v1, v2, v3 );
        // Now set up the w (distance of tri from origin)
        return Vector4( normal.x, normal.y, normal.z, -( normal.dotProduct( v1 ) ) );
    }
    //-----------------------------------------------------------------------
    Vector3 Math::calculateBasicFaceNormalWithoutNormalize( const Vector3 &v1, const Vector3 &v2,
                                                            const Vector3 &v3 )
    {
        Vector3 normal = ( v2 - v1 ).crossProduct( v3 - v1 );
        return normal;
    }
    //-----------------------------------------------------------------------
    Real Math::gaussianDistribution( Real x, Real offset, Real scale )
    {
        OGRE_ASSERT_HIGH( PI != 0.0f && "Ogre library static variables are initialized out of order" );

        Real nom = Math::Exp( -Math::Sqr( x - offset ) / ( 2 * Math::Sqr( scale ) ) );
        Real denom = scale * Math::Sqrt( 2 * Math::PI );

        return nom / denom;
    }
    //---------------------------------------------------------------------
    Matrix4 Math::makeViewMatrix( const Vector3 &position, const Quaternion &orientation,
                                  const Matrix4 *reflectMatrix )
    {
        Matrix4 viewMatrix;

        // View matrix is:
        //
        //  [ Lx  Uy  Dz  Tx  ]
        //  [ Lx  Uy  Dz  Ty  ]
        //  [ Lx  Uy  Dz  Tz  ]
        //  [ 0   0   0   1   ]
        //
        // Where T = -(Transposed(Rot) * Pos)

        // This is most efficiently done using 3x3 Matrices
        Matrix3 rot;
        orientation.ToRotationMatrix( rot );

        // Make the translation relative to new axes
        Matrix3 rotT = rot.Transpose();
        Vector3 trans = -rotT * position;

        // Make final matrix
        viewMatrix = Matrix4::IDENTITY;
        viewMatrix = rotT;  // fills upper 3x3
        viewMatrix[0][3] = trans.x;
        viewMatrix[1][3] = trans.y;
        viewMatrix[2][3] = trans.z;

        // Deal with reflections
        if( reflectMatrix )
        {
            viewMatrix = viewMatrix * ( *reflectMatrix );
        }

        return viewMatrix;
    }
    //---------------------------------------------------------------------
    Real Math::boundingRadiusFromAABB( const AxisAlignedBox &aabb )
    {
        Vector3 max = aabb.getMaximum();
        Vector3 min = aabb.getMinimum();

        Vector3 magnitude = max;
        magnitude.makeCeil( -max );
        magnitude.makeCeil( min );
        magnitude.makeCeil( -min );

        return magnitude.length();
    }
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    //---------------------------------------------------------------------
    Math::RandomValueProvider::~RandomValueProvider() {}
    //---------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const Radian &v )
    {
        o << "Radian(" << v.valueRadians() << ")";
        return o;
    }
    //---------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const Degree &v )
    {
        o << "Degree(" << v.valueDegrees() << ")";
        return o;
    }
    //-----------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const Vector2 &v )
    {
        o << "Vector2(" << v.x << ", " << v.y << ")";
        return o;
    }
    //-----------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const Vector3 &v )
    {
        o << "Vector3(" << v.x << ", " << v.y << ", " << v.z << ")";
        return o;
    }
    //-----------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const Vector4 &v )
    {
        o << "Vector4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
        return o;
    }
    //-----------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const Quaternion &q )
    {
        o << "Quaternion(" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ")";
        return o;
    }
    //-----------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const DualQuaternion &q )
    {
        o << "DualQuaternion(" << q.w << ", " << q.x << ", " << q.y << ", " << q.z << ", " << q.dw
          << ", " << q.dx << ", " << q.dy << ", " << q.dz << ")";
        return o;
    }
    //---------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const ColourValue &c )
    {
        o << "ColourValue(" << c.r << ", " << c.g << ", " << c.b << ", " << c.a << ")";
        return o;
    }
    //-----------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const Matrix3 &mat )
    {
        // clang-format off
        o << "Matrix3(" << mat[0][0] << ", " << mat[0][1] << ", " << mat[0][2] << ", "
                        << mat[1][0] << ", " << mat[1][1] << ", " << mat[1][2] << ", "
                        << mat[2][0] << ", " << mat[2][1] << ", " << mat[2][2] << ")";
        // clang-format on
        return o;
    }
    //-----------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const Matrix4 &mat )
    {
        o << "Matrix4(";
        for( size_t i = 0; i < 4; ++i )
        {
            o << " row" << (unsigned)i << "{";
            for( size_t j = 0; j < 4; ++j )
            {
                o << mat[i][j] << " ";
            }
            o << "}";
        }
        o << ")";
        return o;
    }
    //---------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const AxisAlignedBox &aab )
    {
        switch( aab.mExtent )
        {
        case AxisAlignedBox::EXTENT_NULL:
            o << "AxisAlignedBox(null)";
            return o;

        case AxisAlignedBox::EXTENT_FINITE:
            o << "AxisAlignedBox(min=" << aab.mMinimum << ", max=" << aab.mMaximum << ")";
            return o;

        case AxisAlignedBox::EXTENT_INFINITE:
            o << "AxisAlignedBox(infinite)";
            return o;

        default:  // shut up compiler
            assert( false && "Never reached" );
            return o;
        }
    }

    //-----------------------------------------------------------------------
    std::ostream &operator<<( std::ostream &o, const Plane &p )
    {
        o << "Plane(normal=" << p.normal << ", d=" << p.d << ")";
        return o;
    }
}  // namespace Ogre
