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
#include "OgreDeflectorPlaneAffector2.h"

#include "Math/Array/OgreBooleanMask.h"
#include "OgreStringConverter.h"

using namespace Ogre;

// Instantiate statics
DeflectorPlaneAffector2::CmdPlanePoint DeflectorPlaneAffector2::msPlanePointCmd;
DeflectorPlaneAffector2::CmdPlaneNormal DeflectorPlaneAffector2::msPlaneNormalCmd;
DeflectorPlaneAffector2::CmdBounce DeflectorPlaneAffector2::msBounceCmd;
//-----------------------------------------------------------------------------
DeflectorPlaneAffector2::DeflectorPlaneAffector2() :
    mPlanePoint( Vector3::ZERO ),
    mPlaneNormal( Vector3::UNIT_Y ),
    mBounce( 1.0 )
{
    // Set up parameters
    if( createParamDictionary( "DeflectorPlaneAffector2" ) )
    {
        // Add extra parameters
        ParamDictionary *dict = getParamDictionary();
        dict->addParameter( ParameterDef( "plane_point",
                                          "A point on the deflector plane. Together with the normal "
                                          "vector it defines the plane.",
                                          PT_VECTOR3 ),
                            &msPlanePointCmd );
        dict->addParameter( ParameterDef( "plane_normal",
                                          "The normal vector of the deflector plane. Together with "
                                          "the point it defines the plane.",
                                          PT_VECTOR3 ),
                            &msPlaneNormalCmd );
        dict->addParameter(
            ParameterDef( "bounce",
                          "The amount of bouncing when a particle is deflected. 0 means no "
                          "deflection and 1 stands for 100 percent reflection.",
                          PT_REAL ),
            &msBounceCmd );
    }
}
//-----------------------------------------------------------------------------
void DeflectorPlaneAffector2::run( ParticleCpuData cpuData, const size_t numParticles,
                                   const ArrayReal timeSinceLast ) const
{
    // precalculate distance of plane from origin
    const ArrayReal planeDistance =
        Mathlib::SetAll( -mPlaneNormal.dotProduct( mPlanePoint ) /
                         Math::Sqrt( mPlaneNormal.dotProduct( mPlaneNormal ) ) );

    const ArrayReal bounce = Mathlib::SetAll( mBounce );

    ArrayVector3 planeNormal;
    planeNormal.setAll( mPlaneNormal );

    for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
    {
        ArrayVector3 direction( *cpuData.mDirection * timeSinceLast );

        // goesIntoPlane =
        //      planeNormal.dotProduct( *cpuData.mPosition + direction ) + planeDistance <= 0.0
        const ArrayMaskR goesIntoPlane = Mathlib::CompareLessEqual(
            planeNormal.dotProduct( *cpuData.mPosition + direction ) + planeDistance, ARRAY_REAL_ZERO );

        // If none of the particles go through the plane, skip calculations.
        if( BooleanMask4::getScalarMask( goesIntoPlane ) != 0u )
        {
            const ArrayReal a = planeNormal.dotProduct( *cpuData.mPosition ) + planeDistance;

            // isAbovePlane = a > 0.0
            const ArrayMaskR isAbovePlane = Mathlib::CompareGreater( a, ARRAY_REAL_ZERO );

            // for intersection point
            const ArrayVector3 directionPart = direction * ( -a / direction.dotProduct( planeNormal ) );

            const ArrayVector3 newPos =
                ( *cpuData.mPosition + directionPart ) + ( directionPart - direction ) * bounce;
            // reflect direction vector
            const ArrayVector3 newDir =
                ( *cpuData.mDirection -
                  ( ( 2.0f * cpuData.mDirection->dotProduct( planeNormal ) ) * planeNormal ) ) *
                bounce;

            // mustUpdate = goesIntoPlane && isAbovePlane
            const ArrayMaskR mustUpdate = Mathlib::And( goesIntoPlane, isAbovePlane );
            /*
                if( goesIntoPlane && isAbovePlane )
                {
                    cpuData.mPosition = newPos;
                    cpuData.mDirection = newDir;
                }
            */
            *cpuData.mPosition = ArrayVector3::Cmov4( newPos, *cpuData.mPosition, mustUpdate );
            *cpuData.mDirection = ArrayVector3::Cmov4( newDir, *cpuData.mDirection, mustUpdate );
        }

        cpuData.advancePack();
    }
}
//-----------------------------------------------------------------------------
void DeflectorPlaneAffector2::setPlanePoint( const Vector3 &pos )
{
    mPlanePoint = pos;
}
//-----------------------------------------------------------------------------
void DeflectorPlaneAffector2::setPlaneNormal( const Vector3 &normal )
{
    mPlaneNormal = normal;
}
//-----------------------------------------------------------------------------
void DeflectorPlaneAffector2::setBounce( Real bounce )
{
    mBounce = bounce;
}
//-----------------------------------------------------------------------------
Vector3 DeflectorPlaneAffector2::getPlanePoint() const
{
    return mPlanePoint;
}
//-----------------------------------------------------------------------------
Vector3 DeflectorPlaneAffector2::getPlaneNormal() const
{
    return mPlaneNormal;
}
//-----------------------------------------------------------------------------
Real DeflectorPlaneAffector2::getBounce() const
{
    return mBounce;
}
//-----------------------------------------------------------------------------
String DeflectorPlaneAffector2::getType() const
{
    return "DeflectorPlane";
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
String DeflectorPlaneAffector2::CmdPlanePoint::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const DeflectorPlaneAffector2 *>( target )->getPlanePoint() );
}
void DeflectorPlaneAffector2::CmdPlanePoint::doSet( void *target, const String &val )
{
    static_cast<DeflectorPlaneAffector2 *>( target )->setPlanePoint(
        StringConverter::parseVector3( val ) );
}
//-----------------------------------------------------------------------------
String DeflectorPlaneAffector2::CmdPlaneNormal::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const DeflectorPlaneAffector2 *>( target )->getPlaneNormal() );
}
void DeflectorPlaneAffector2::CmdPlaneNormal::doSet( void *target, const String &val )
{
    static_cast<DeflectorPlaneAffector2 *>( target )->setPlaneNormal(
        StringConverter::parseVector3( val ) );
}
//-----------------------------------------------------------------------------
String DeflectorPlaneAffector2::CmdBounce::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const DeflectorPlaneAffector2 *>( target )->getBounce() );
}
void DeflectorPlaneAffector2::CmdBounce::doSet( void *target, const String &val )
{
    static_cast<DeflectorPlaneAffector2 *>( target )->setBounce( StringConverter::parseReal( val ) );
}
