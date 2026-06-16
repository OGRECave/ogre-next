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
#include "OgreDirectionRandomiserAffector2.h"

#include "OgreStringConverter.h"

using namespace Ogre;

// Instantiate statics
DirectionRandomiserAffector2::CmdRandomness DirectionRandomiserAffector2::msRandomnessCmd;
DirectionRandomiserAffector2::CmdScope DirectionRandomiserAffector2::msScopeCmd;
DirectionRandomiserAffector2::CmdKeepVelocity DirectionRandomiserAffector2::msKeepVelocityCmd;

//-----------------------------------------------------------------------------
DirectionRandomiserAffector2::DirectionRandomiserAffector2() :
    mRandomness( 1.0f ),
    mScope( 1.0f ),
    mKeepVelocity( false )
{
    // Set up parameters
    if( createParamDictionary( "DirectionRandomiserAffector2" ) )
    {
        // Add extra parameters
        ParamDictionary *dict = getParamDictionary();
        dict->addParameter(
            ParameterDef( "randomness",
                          "The amount of randomness (chaos) to apply to the particle movement.",
                          PT_REAL ),
            &msRandomnessCmd );
        dict->addParameter(
            ParameterDef( "scope", "The percentage of particles which is affected.", PT_REAL ),
            &msScopeCmd );
        dict->addParameter(
            ParameterDef( "keep_velocity",
                          "Determines whether the velocity of the particles is changed.", PT_BOOL ),
            &msKeepVelocityCmd );
    }
}
//-----------------------------------------------------------------------------
void DirectionRandomiserAffector2::run( ParticleCpuData cpuData, const size_t numParticles,
                                        const ArrayReal timeSinceLast ) const
{
    const Real randomness = mRandomness;
    const ArrayReal scope = Mathlib::SetAll( mScope );

    // NOTE:
    // This affector is not very SIMD-friendly as it needs to calculate a
    // lot of Math::RangeRandom in non-SIMD fashion.
    for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
    {
        ArrayReal scopeRandValues;
        {
            Real *aliasedScopeRandValues = reinterpret_cast<Real *>( &scopeRandValues );

            for( size_t j = 0u; j < ARRAY_PACKED_REALS; ++j )
                aliasedScopeRandValues[j] = Math::UnitRandom();
        }

        // alterDirection = mScope > Math::UnitRandom()
        const ArrayMaskR alterDirection = Mathlib::CompareGreater( scope, scopeRandValues );
        const ArrayMaskR isNonZeroLength =
            Mathlib::CompareGreater( cpuData.mDirection->squaredLength(), ARRAY_MASK_ZERO );

        ArrayReal length;
        if( mKeepVelocity )
            length = cpuData.mDirection->length();

        // Call Math::RangeRandom N x 3 times.
        ArrayReal randX, randY, randZ;
        {
            Real *aliasedRandX = reinterpret_cast<Real *>( &randX );
            Real *aliasedRandY = reinterpret_cast<Real *>( &randY );
            Real *aliasedRandZ = reinterpret_cast<Real *>( &randZ );
            for( size_t j = 0u; j < ARRAY_PACKED_REALS; ++j )
            {
                aliasedRandX[j] = Math::RangeRandom( -randomness, randomness );
                aliasedRandY[j] = Math::RangeRandom( -randomness, randomness );
                aliasedRandZ[j] = Math::RangeRandom( -randomness, randomness );
            }
        }

        ArrayVector3 calculatedDir =
            *cpuData.mDirection + ArrayVector3( randX, randY, randZ ) * timeSinceLast;
        if( mKeepVelocity )
            calculatedDir *= length / cpuData.mDirection->length();

        /*
            if( alterDirection && isNonZeroLength )
                cpuData.mDirection = calculatedDir;

            We must use CmovRobust because if mKeepVelocity == true and it is zero length,
            we may end up with unwanted NaNs.
        */
        cpuData.mDirection->CmovRobust( Mathlib::And( alterDirection, isNonZeroLength ), calculatedDir );
        cpuData.advancePack();
    }
}
//-----------------------------------------------------------------------------
void DirectionRandomiserAffector2::setRandomness( Real force ) { mRandomness = force; }
//-----------------------------------------------------------------------------
void DirectionRandomiserAffector2::setScope( Real scope ) { mScope = scope; }
//-----------------------------------------------------------------------------
void DirectionRandomiserAffector2::setKeepVelocity( bool keepVelocity ) { mKeepVelocity = keepVelocity; }
//-----------------------------------------------------------------------------
Real DirectionRandomiserAffector2::getRandomness() const { return mRandomness; }
//-----------------------------------------------------------------------------
Real DirectionRandomiserAffector2::getScope() const { return mScope; }
//-----------------------------------------------------------------------------
bool DirectionRandomiserAffector2::getKeepVelocity() const { return mKeepVelocity; }
//-----------------------------------------------------------------------------
String DirectionRandomiserAffector2::getType() const { return "DirectionRandomiser"; }
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
String DirectionRandomiserAffector2::CmdRandomness::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const DirectionRandomiserAffector2 *>( target )->getRandomness() );
}
void DirectionRandomiserAffector2::CmdRandomness::doSet( void *target, const String &val )
{
    static_cast<DirectionRandomiserAffector2 *>( target )->setRandomness(
        StringConverter::parseReal( val ) );
}

String DirectionRandomiserAffector2::CmdScope::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const DirectionRandomiserAffector2 *>( target )->getScope() );
}
void DirectionRandomiserAffector2::CmdScope::doSet( void *target, const String &val )
{
    static_cast<DirectionRandomiserAffector2 *>( target )->setScope( StringConverter::parseReal( val ) );
}
String DirectionRandomiserAffector2::CmdKeepVelocity::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const DirectionRandomiserAffector2 *>( target )->getKeepVelocity() );
}
void DirectionRandomiserAffector2::CmdKeepVelocity::doSet( void *target, const String &val )
{
    static_cast<DirectionRandomiserAffector2 *>( target )->setKeepVelocity(
        StringConverter::parseBool( val ) );
}
