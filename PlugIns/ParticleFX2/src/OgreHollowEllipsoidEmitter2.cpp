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
// Original author: Tels <http://bloodgate.com>, released as public domain
#include "OgreHollowEllipsoidEmitter2.h"

#include "OgreMath.h"
#include "OgreStringConverter.h"

/* Implements an Emitter whose emitting points all lie inside an ellipsoid.
   See <http://mathworld.wolfram.com/Ellipsoid.html> for mathematical details.

  If the lengths of two axes of an ellipsoid are the same, the figure is
  called a 'spheroid' (depending on whether c < a or c > a, an 'oblate
  spheroid' or 'prolate spheroid', respectively), and if all three are the
  same, it is a 'sphere' (ball).
*/

using namespace Ogre;

HollowEllipsoidEmitter2::CmdInnerX HollowEllipsoidEmitter2::msCmdInnerX;
HollowEllipsoidEmitter2::CmdInnerY HollowEllipsoidEmitter2::msCmdInnerY;
HollowEllipsoidEmitter2::CmdInnerZ HollowEllipsoidEmitter2::msCmdInnerZ;
//-----------------------------------------------------------------------------
HollowEllipsoidEmitter2::HollowEllipsoidEmitter2()
{
    if( initDefaults() )
    {
        // Add custom parameters
        ParamDictionary *pDict = getParamDictionary();

        pDict->addParameter( ParameterDef( "inner_width",
                                           "Parametric value describing the proportion of the "
                                           "shape which is hollow.",
                                           PT_REAL ),
                             &msCmdInnerX );
        pDict->addParameter( ParameterDef( "inner_height",
                                           "Parametric value describing the proportion of the "
                                           "shape which is hollow.",
                                           PT_REAL ),
                             &msCmdInnerY );
        pDict->addParameter( ParameterDef( "inner_depth",
                                           "Parametric value describing the proportion of the "
                                           "shape which is hollow.",
                                           PT_REAL ),
                             &msCmdInnerZ );
    }
    // default is half empty
    setInnerSize( 0.5, 0.5, 0.5 );
}
//-----------------------------------------------------------------------------
void HollowEllipsoidEmitter2::initEmittedParticles( ParticleCpuData cpuData,
                                                    const EmittedParticle *newHandles,
                                                    const size_t numParticles )
{
    const Vector3 position = mPosition;
    const Vector2 dimensions = mDimensions;
    const Vector3 xRange = mXRange;
    const Vector3 yRange = mYRange;
    const Vector3 zRange = mZRange;

    for( size_t i = 0u; i < numParticles; ++i )
    {
        const size_t h = newHandles[i].handle;
        const size_t j = h / ARRAY_PACKED_REALS;
        const size_t idx = h % ARRAY_PACKED_REALS;

        // create two random angles alpha and beta
        // with these two angles, we are able to select any point on an
        // ellipsoid's surface
        Radian alpha( Math::RangeRandom( 0, Math::TWO_PI ) );
        Radian beta( Math::RangeRandom( 0, Math::PI ) );

        Real a, b, c, x, y, z;
        // create three random radius values that are bigger than the inner
        // size, but smaller/equal than/to the outer size 1.0 (inner size is
        // between 0 and 1)
        a = Math::RangeRandom( mInnerSize.x, 1.0 );
        b = Math::RangeRandom( mInnerSize.y, 1.0 );
        c = Math::RangeRandom( mInnerSize.z, 1.0 );

        // with a,b,c we have defined a random ellipsoid between the inner
        // ellipsoid and the outer sphere (radius 1.0)
        // with alpha and beta we select on point on this random ellipsoid
        // and calculate the 3D coordinates of this point
        Real sinbeta( Math::Sin( beta ) );
        x = a * Math::Cos( alpha ) * sinbeta;
        y = b * Math::Sin( alpha ) * sinbeta;
        z = c * Math::Cos( beta );

        // scale the found point to the ellipsoid's size and move it
        // relatively to the center of the emitter point
        const Vector3 localPos = position + x * xRange + y * yRange + z * zRange;
        cpuData.mPosition[j].setFromVector3( newHandles[i].pos + newHandles[i].rot * localPos, idx );
        reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mRotation )[h] = 0.0f;

        Vector3 direction;
        this->genEmissionDirection( localPos, direction );
        this->genEmissionVelocity( direction );
        direction = newHandles[i].rot * direction;
        cpuData.mDirection[j].setFromVector3( direction, idx );

        cpuData.mDimensions[j].setFromVector2( dimensions, idx );

        Ogre::ColourValue colour;
        this->genEmissionColour( colour );
        cpuData.mColour[j].setFromVector4( Vector4( colour.ptr() ), idx );

        reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mTimeToLive )[h] =
            reinterpret_cast<Real * RESTRICT_ALIAS>( cpuData.mTotalTimeToLive )[h] =
                this->genEmissionTTL();
    }
}
//-----------------------------------------------------------------------------
void HollowEllipsoidEmitter2::setInnerSize( Real x, Real y, Real z )
{
    OGRE_ASSERT_LOW( ( x > 0 ) && ( x < 1.0 ) && ( y > 0 ) && ( y < 1.0 ) && ( z > 0 ) && ( z < 1.0 ) );

    mInnerSize.x = x;
    mInnerSize.y = y;
    mInnerSize.z = z;
}
//-----------------------------------------------------------------------------
void HollowEllipsoidEmitter2::setInnerSizeX( Real x )
{
    OGRE_ASSERT_LOW( x > 0 && x < 1.0 );

    mInnerSize.x = x;
}
//-----------------------------------------------------------------------------
void HollowEllipsoidEmitter2::setInnerSizeY( Real y )
{
    OGRE_ASSERT_LOW( y > 0 && y < 1.0 );

    mInnerSize.y = y;
}
//-----------------------------------------------------------------------------
void HollowEllipsoidEmitter2::setInnerSizeZ( Real z )
{
    OGRE_ASSERT_LOW( z > 0 && z < 1.0 );

    mInnerSize.z = z;
}
//-----------------------------------------------------------------------------
Real HollowEllipsoidEmitter2::getInnerSizeX() const { return mInnerSize.x; }
//-----------------------------------------------------------------------------
Real HollowEllipsoidEmitter2::getInnerSizeY() const { return mInnerSize.y; }
//-----------------------------------------------------------------------------
Real HollowEllipsoidEmitter2::getInnerSizeZ() const { return mInnerSize.z; }
//-----------------------------------------------------------------------------------
static const String kHollowEllipsoidEmitterFactoryName = "HollowEllipsoid";
const String &HollowEllipsoidEmitter2::getType() const { return kHollowEllipsoidEmitterFactoryName; }
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
String HollowEllipsoidEmitter2::CmdInnerX::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const HollowEllipsoidEmitter2 *>( target )->getInnerSizeX() );
}
void HollowEllipsoidEmitter2::CmdInnerX::doSet( void *target, const String &val )
{
    static_cast<HollowEllipsoidEmitter2 *>( target )->setInnerSizeX( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String HollowEllipsoidEmitter2::CmdInnerY::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const HollowEllipsoidEmitter2 *>( target )->getInnerSizeY() );
}
void HollowEllipsoidEmitter2::CmdInnerY::doSet( void *target, const String &val )
{
    static_cast<HollowEllipsoidEmitter2 *>( target )->setInnerSizeY( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String HollowEllipsoidEmitter2::CmdInnerZ::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const HollowEllipsoidEmitter2 *>( target )->getInnerSizeZ() );
}
void HollowEllipsoidEmitter2::CmdInnerZ::doSet( void *target, const String &val )
{
    static_cast<HollowEllipsoidEmitter2 *>( target )->setInnerSizeZ( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
const String &HollowEllipsoidEmitterFactory2::getName() const
{
    return kHollowEllipsoidEmitterFactoryName;
}
//-----------------------------------------------------------------------------------------
EmitterDefData *HollowEllipsoidEmitterFactory2::createEmitter() { return new EllipsoidEmitter2(); }
