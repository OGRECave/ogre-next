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
#include "OgreRotationAffector2.h"

#include "OgreStringConverter.h"

using namespace Ogre;

// init statics
RotationAffector2::CmdRotationSpeedRangeStart RotationAffector2::msRotationSpeedRangeStartCmd;
RotationAffector2::CmdRotationSpeedRangeEnd RotationAffector2::msRotationSpeedRangeEndCmd;
RotationAffector2::CmdRotationRangeStart RotationAffector2::msRotationRangeStartCmd;
RotationAffector2::CmdRotationRangeEnd RotationAffector2::msRotationRangeEndCmd;
//-----------------------------------------------------------------------------
RotationAffector2::RotationAffector2() :
    mRotationSpeedRangeStart( 0 ),
    mRotationSpeedRangeEnd( 0 ),
    mRotationRangeStart( 0 ),
    mRotationRangeEnd( 0 )
{
    // Init parameters
    if( createParamDictionary( "RotationAffector2" ) )
    {
        ParamDictionary *dict = getParamDictionary();

        dict->addParameter(
            ParameterDef( "rotation_speed_range_start",
                          "The start of a range of rotation speed to be assigned to emitted particles.",
                          PT_REAL ),
            &msRotationSpeedRangeStartCmd );

        dict->addParameter(
            ParameterDef( "rotation_speed_range_end",
                          "The end of a range of rotation speed to be assigned to emitted particles.",
                          PT_REAL ),
            &msRotationSpeedRangeEndCmd );

        dict->addParameter(
            ParameterDef( "rotation_range_start",
                          "The start of a range of rotation angles to be assigned to emitted particles.",
                          PT_REAL ),
            &msRotationRangeStartCmd );

        dict->addParameter(
            ParameterDef( "rotation_range_end",
                          "The end of a range of rotation angles to be assigned to emitted particles.",
                          PT_REAL ),
            &msRotationRangeEndCmd );
    }
}
//-----------------------------------------------------------------------------
bool RotationAffector2::needsInitialization() const { return true; }
//-----------------------------------------------------------------------------
bool RotationAffector2::wantsRotation() const { return true; }
//-----------------------------------------------------------------------------
void RotationAffector2::initEmittedParticles( ParticleCpuData cpuData, const EmittedParticle *newHandles,
                                              size_t numParticles ) const
{
    for( size_t i = 0u; i < numParticles; ++i )
    {
        const size_t h = newHandles[i].handle;
        reinterpret_cast<Radian * RESTRICT_ALIAS>( cpuData.mRotation )[h] =
            mRotationRangeStart + Math::UnitRandom() * ( mRotationRangeEnd - mRotationRangeStart );
        reinterpret_cast<Radian * RESTRICT_ALIAS>( cpuData.mRotationSpeed )[h] =
            mRotationSpeedRangeStart +
            Math::UnitRandom() * ( mRotationSpeedRangeEnd - mRotationSpeedRangeStart );
    }
}
//-----------------------------------------------------------------------------
void RotationAffector2::run( ParticleCpuData cpuData, const size_t numParticles,
                             const ArrayReal timeSinceLast ) const
{
    for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
    {
        *cpuData.mRotation = *cpuData.mRotation + *cpuData.mRotationSpeed * timeSinceLast;
        cpuData.mRotation->wrapToRangeNPI_PI();
        cpuData.advancePack();
    }
}
//-----------------------------------------------------------------------------
const Radian &RotationAffector2::getRotationSpeedRangeStart() const { return mRotationSpeedRangeStart; }
//-----------------------------------------------------------------------------
const Radian &RotationAffector2::getRotationSpeedRangeEnd() const { return mRotationSpeedRangeEnd; }
//-----------------------------------------------------------------------------
void RotationAffector2::setRotationSpeedRangeStart( const Radian &val )
{
    mRotationSpeedRangeStart = val;
}
//-----------------------------------------------------------------------------
void RotationAffector2::setRotationSpeedRangeEnd( const Radian &val ) { mRotationSpeedRangeEnd = val; }
//-----------------------------------------------------------------------------
const Radian &RotationAffector2::getRotationRangeStart() const { return mRotationRangeStart; }
//-----------------------------------------------------------------------------
const Radian &RotationAffector2::getRotationRangeEnd() const { return mRotationRangeEnd; }
//-----------------------------------------------------------------------------
void RotationAffector2::setRotationRangeStart( const Radian &val ) { mRotationRangeStart = val; }
//-----------------------------------------------------------------------------
void RotationAffector2::setRotationRangeEnd( const Radian &val ) { mRotationRangeEnd = val; }
//-----------------------------------------------------------------------------
String RotationAffector2::getType() const { return "Rotator"; }
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
String RotationAffector2::CmdRotationSpeedRangeEnd::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const RotationAffector2 *>( target )->getRotationSpeedRangeEnd() );
}
void RotationAffector2::CmdRotationSpeedRangeEnd::doSet( void *target, const String &val )
{
    static_cast<RotationAffector2 *>( target )->setRotationSpeedRangeEnd(
        StringConverter::parseAngle( val ) );
}
//-----------------------------------------------------------------------------
String RotationAffector2::CmdRotationSpeedRangeStart::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const RotationAffector2 *>( target )->getRotationSpeedRangeStart() );
}
void RotationAffector2::CmdRotationSpeedRangeStart::doSet( void *target, const String &val )
{
    static_cast<RotationAffector2 *>( target )->setRotationSpeedRangeStart(
        StringConverter::parseAngle( val ) );
}
//-----------------------------------------------------------------------------
String RotationAffector2::CmdRotationRangeEnd::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const RotationAffector2 *>( target )->getRotationRangeEnd() );
}
void RotationAffector2::CmdRotationRangeEnd::doSet( void *target, const String &val )
{
    static_cast<RotationAffector2 *>( target )->setRotationRangeEnd(
        StringConverter::parseAngle( val ) );
}
//-----------------------------------------------------------------------------
String RotationAffector2::CmdRotationRangeStart::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const RotationAffector2 *>( target )->getRotationRangeStart() );
}
void RotationAffector2::CmdRotationRangeStart::doSet( void *target, const String &val )
{
    static_cast<RotationAffector2 *>( target )->setRotationRangeStart(
        StringConverter::parseAngle( val ) );
}
