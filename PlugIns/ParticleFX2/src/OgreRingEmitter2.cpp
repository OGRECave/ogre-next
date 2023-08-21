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
// Original author: Tels <http://bloodgate.com>, released as public domain
#include "OgreRingEmitter2.h"

#include "OgreStringConverter.h"

/* Implements an Emitter whose emitting points all lie inside a ring.
 */

using namespace Ogre;

RingEmitter2::CmdInnerX RingEmitter2::msCmdInnerX;
RingEmitter2::CmdInnerY RingEmitter2::msCmdInnerY;
//-----------------------------------------------------------------------------
RingEmitter2::RingEmitter2()
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
    }
    // default is half empty
    setInnerSize( 0.5, 0.5 );
}
//-----------------------------------------------------------------------------
void RingEmitter2::initEmittedParticles( ParticleCpuData cpuData, const EmittedParticle *newHandles,
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

        Real a, b, x, y, z;
        // create a random angle from 0 .. PI*2
        Radian alpha( Math::RangeRandom( 0, Math::TWO_PI ) );

        // create two random radius values that are bigger than the inner size
        a = Math::RangeRandom( mInnerSizex, 1.0 );
        b = Math::RangeRandom( mInnerSizey, 1.0 );

        // with a and b we have defined a random ellipse inside the inner
        // ellipse and the outer circle (radius 1.0)
        // with alpha, and a and b we select a random point on this ellipse
        // and calculate it's coordinates
        x = a * Math::Sin( alpha );
        y = b * Math::Cos( alpha );
        // the height is simple -1 to 1
        z = Math::SymmetricRandom();

        // scale the found point to the ring's size and move it
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
void RingEmitter2::setInnerSize( Real x, Real y )
{
    // TODO: should really throw some exception
    if( ( x > 0 ) && ( x < 1.0 ) && ( y > 0 ) && ( y < 1.0 ) )
    {
        mInnerSizex = x;
        mInnerSizey = y;
    }
}
//-----------------------------------------------------------------------------
void RingEmitter2::setInnerSizeX( Real x )
{
    OGRE_ASSERT_LOW( x > 0 && x < 1.0 );
    mInnerSizex = x;
}
//-----------------------------------------------------------------------------
void RingEmitter2::setInnerSizeY( Real y )
{
    OGRE_ASSERT_LOW( y > 0 && y < 1.0 );
    mInnerSizey = y;
}
//-----------------------------------------------------------------------------
Real RingEmitter2::getInnerSizeX() const
{
    return mInnerSizex;
}
//-----------------------------------------------------------------------------
Real RingEmitter2::getInnerSizeY() const
{
    return mInnerSizey;
}
//-----------------------------------------------------------------------------
static const String kRingEmitterFactoryName = "Ring";
const String &RingEmitter2::getType() const
{
    return kRingEmitterFactoryName;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
String RingEmitter2::CmdInnerX::doGet( const void *target ) const
{
    return StringConverter::toString( static_cast<const RingEmitter2 *>( target )->getInnerSizeX() );
}
void RingEmitter2::CmdInnerX::doSet( void *target, const String &val )
{
    static_cast<RingEmitter2 *>( target )->setInnerSizeX( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String RingEmitter2::CmdInnerY::doGet( const void *target ) const
{
    return StringConverter::toString( static_cast<const RingEmitter2 *>( target )->getInnerSizeY() );
}
void RingEmitter2::CmdInnerY::doSet( void *target, const String &val )
{
    static_cast<RingEmitter2 *>( target )->setInnerSizeY( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
const String &RingEmitterFactory2::getName() const
{
    return kRingEmitterFactoryName;
}
//-----------------------------------------------------------------------------------------
EmitterDefData *RingEmitterFactory2::createEmitter()
{
    return new RingEmitter2();
}
