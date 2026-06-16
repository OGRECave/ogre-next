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
#include "OgreColourFaderAffector2FX2.h"

#include "OgreStringConverter.h"

using namespace Ogre;

// init statics
// Phase 1
ColourFaderAffector2FX2::CmdRedAdjust1 ColourFaderAffector2FX2::msRedCmd1;
ColourFaderAffector2FX2::CmdGreenAdjust1 ColourFaderAffector2FX2::msGreenCmd1;
ColourFaderAffector2FX2::CmdBlueAdjust1 ColourFaderAffector2FX2::msBlueCmd1;
ColourFaderAffector2FX2::CmdAlphaAdjust1 ColourFaderAffector2FX2::msAlphaCmd1;

// Phase 2
ColourFaderAffector2FX2::CmdRedAdjust2 ColourFaderAffector2FX2::msRedCmd2;
ColourFaderAffector2FX2::CmdGreenAdjust2 ColourFaderAffector2FX2::msGreenCmd2;
ColourFaderAffector2FX2::CmdBlueAdjust2 ColourFaderAffector2FX2::msBlueCmd2;
ColourFaderAffector2FX2::CmdAlphaAdjust2 ColourFaderAffector2FX2::msAlphaCmd2;

ColourFaderAffector2FX2::CmdStateChange ColourFaderAffector2FX2::msStateCmd;

ColourFaderAffector2FX2::CmdMinColour ColourFaderAffector2FX2::msMinColourCmd;
ColourFaderAffector2FX2::CmdMaxColour ColourFaderAffector2FX2::msMaxColourCmd;

//-----------------------------------------------------------------------------
ColourFaderAffector2FX2::ColourFaderAffector2FX2() :
    mColourAdj1( Vector4::ZERO ),
    mColourAdj2( Vector4::ZERO ),
    mMinColour( Vector4::ZERO ),
    mMaxColour( Vector4( Real( 1.0f ) ) ),
    mStateChangeVal( 1.0f )  // Switch when there is 1 second left on the TTL
{
    // Init parameters
    if( createParamDictionary( "ColourFaderAffector2FX2" ) )
    {
        ParamDictionary *dict = getParamDictionary();

        // Phase 1
        dict->addParameter(
            ParameterDef( "red1",
                          "The amount by which to adjust the red component of particles per second.",
                          PT_REAL ),
            &msRedCmd1 );
        dict->addParameter(
            ParameterDef( "green1",
                          "The amount by which to adjust the green component of particles per second.",
                          PT_REAL ),
            &msGreenCmd1 );
        dict->addParameter(
            ParameterDef( "blue1",
                          "The amount by which to adjust the blue component of particles per second.",
                          PT_REAL ),
            &msBlueCmd1 );
        dict->addParameter(
            ParameterDef( "alpha1",
                          "The amount by which to adjust the alpha component of particles per second.",
                          PT_REAL ),
            &msAlphaCmd1 );

        // Phase 2
        dict->addParameter(
            ParameterDef( "red2",
                          "The amount by which to adjust the red component of particles per second.",
                          PT_REAL ),
            &msRedCmd2 );
        dict->addParameter(
            ParameterDef( "green2",
                          "The amount by which to adjust the green component of particles per second.",
                          PT_REAL ),
            &msGreenCmd2 );
        dict->addParameter(
            ParameterDef( "blue2",
                          "The amount by which to adjust the blue component of particles per second.",
                          PT_REAL ),
            &msBlueCmd2 );
        dict->addParameter(
            ParameterDef( "alpha2",
                          "The amount by which to adjust the alpha component of particles per second.",
                          PT_REAL ),
            &msAlphaCmd2 );

        // State Change Value
        dict->addParameter(
            ParameterDef(
                "state_change",
                "When the particle has this much time to live left, it will switch to state 2.",
                PT_REAL ),
            &msStateCmd );

        dict->addParameter(
            ParameterDef( "min_colour", "The minimum colour to clamp against.", PT_COLOURVALUE ),
            &msMinColourCmd );
        dict->addParameter(
            ParameterDef( "max_colour", "The maximum colour to clamp against.", PT_COLOURVALUE ),
            &msMaxColourCmd );
    }
}
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::run( ParticleCpuData cpuData, const size_t numParticles,
                                   const ArrayReal timeSinceLast ) const
{
    ArrayVector4 minColour, maxColour, colourAdj1, colourAdj2;
    minColour.setAll( mMinColour );
    maxColour.setAll( mMaxColour );
    colourAdj1.setAll( mColourAdj1 );
    colourAdj2.setAll( mColourAdj2 );

    const ArrayReal stateChangeVal = Mathlib::SetAll( mStateChangeVal );

    // Scale adjustments by time
    colourAdj1 *= timeSinceLast;
    colourAdj2 *= timeSinceLast;

    for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
    {
        // if( cpuData.mTimeToLive > stateChangeVal )
        //  colourAdj = colourAdj1;
        // else
        //  colourAdj = colourAdj2;
        const ArrayVector4 colourAdj = ArrayVector4::Cmov4(
            colourAdj1, colourAdj2, Mathlib::CompareGreater( *cpuData.mTimeToLive, stateChangeVal ) );

        *cpuData.mColour += colourAdj;
        cpuData.mColour->makeCeil( minColour );
        cpuData.mColour->makeFloor( maxColour );
        cpuData.advancePack();
    }
}
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setMaxColour( const Vector4 &rgba ) { mMaxColour = rgba; }
//-----------------------------------------------------------------------------
const Vector4 &ColourFaderAffector2FX2::getMaxColour() const { return mMaxColour; }
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setMinColour( const Vector4 &rgba ) { mMinColour = rgba; }
//-----------------------------------------------------------------------------
const Vector4 &ColourFaderAffector2FX2::getMinColour() const { return mMinColour; }
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setAdjust1( float red, float green, float blue, float alpha )
{
    mColourAdj1.x = red;
    mColourAdj1.y = green;
    mColourAdj1.z = blue;
    mColourAdj1.w = alpha;
}
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setAdjust2( float red, float green, float blue, float alpha )
{
    mColourAdj2.x = red;
    mColourAdj2.y = green;
    mColourAdj2.z = blue;
    mColourAdj2.w = alpha;
}
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setRedAdjust1( float red ) { mColourAdj1.x = red; }
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setRedAdjust2( float red ) { mColourAdj2.x = red; }
//-----------------------------------------------------------------------------
float ColourFaderAffector2FX2::getRedAdjust1() const { return mColourAdj1.x; }
//-----------------------------------------------------------------------------
float ColourFaderAffector2FX2::getRedAdjust2() const { return mColourAdj2.x; }
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setGreenAdjust1( float green ) { mColourAdj1.y = green; }
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setGreenAdjust2( float green ) { mColourAdj2.y = green; }
//-----------------------------------------------------------------------------
float ColourFaderAffector2FX2::getGreenAdjust1() const { return mColourAdj1.y; }
//-----------------------------------------------------------------------------
float ColourFaderAffector2FX2::getGreenAdjust2() const { return mColourAdj2.y; }
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setBlueAdjust1( float blue ) { mColourAdj1.z = blue; }
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setBlueAdjust2( float blue ) { mColourAdj2.z = blue; }
//-----------------------------------------------------------------------------
float ColourFaderAffector2FX2::getBlueAdjust1() const { return mColourAdj1.z; }
//-----------------------------------------------------------------------------
float ColourFaderAffector2FX2::getBlueAdjust2() const { return mColourAdj2.z; }
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setAlphaAdjust1( float alpha ) { mColourAdj1.w = alpha; }
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setAlphaAdjust2( float alpha ) { mColourAdj2.w = alpha; }
//-----------------------------------------------------------------------------
float ColourFaderAffector2FX2::getAlphaAdjust1() const { return mColourAdj1.w; }
//-----------------------------------------------------------------------------
float ColourFaderAffector2FX2::getAlphaAdjust2() const { return mColourAdj2.w; }
//-----------------------------------------------------------------------------
void ColourFaderAffector2FX2::setStateChange( Real NewValue ) { mStateChangeVal = NewValue; }
//-----------------------------------------------------------------------------
Real ColourFaderAffector2FX2::getStateChange() const { return mStateChangeVal; }
//-----------------------------------------------------------------------------------
String ColourFaderAffector2FX2::getType() const { return "ColourFader2"; }
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
String ColourFaderAffector2FX2::CmdRedAdjust1::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffector2FX2 *>( target )->getRedAdjust1() );
}
void ColourFaderAffector2FX2::CmdRedAdjust1::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffector2FX2 *>( target )->setRedAdjust1( StringConverter::parseReal( val ) );
}
String ColourFaderAffector2FX2::CmdRedAdjust2::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffector2FX2 *>( target )->getRedAdjust2() );
}
void ColourFaderAffector2FX2::CmdRedAdjust2::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffector2FX2 *>( target )->setRedAdjust2( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String ColourFaderAffector2FX2::CmdGreenAdjust1::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffector2FX2 *>( target )->getGreenAdjust1() );
}
void ColourFaderAffector2FX2::CmdGreenAdjust1::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffector2FX2 *>( target )->setGreenAdjust1(
        StringConverter::parseReal( val ) );
}
String ColourFaderAffector2FX2::CmdGreenAdjust2::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffector2FX2 *>( target )->getGreenAdjust2() );
}
void ColourFaderAffector2FX2::CmdGreenAdjust2::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffector2FX2 *>( target )->setGreenAdjust2(
        StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String ColourFaderAffector2FX2::CmdBlueAdjust1::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffector2FX2 *>( target )->getBlueAdjust1() );
}
void ColourFaderAffector2FX2::CmdBlueAdjust1::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffector2FX2 *>( target )->setBlueAdjust1(
        StringConverter::parseReal( val ) );
}
String ColourFaderAffector2FX2::CmdBlueAdjust2::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffector2FX2 *>( target )->getBlueAdjust2() );
}
void ColourFaderAffector2FX2::CmdBlueAdjust2::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffector2FX2 *>( target )->setBlueAdjust2(
        StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String ColourFaderAffector2FX2::CmdAlphaAdjust1::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffector2FX2 *>( target )->getAlphaAdjust1() );
}
void ColourFaderAffector2FX2::CmdAlphaAdjust1::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffector2FX2 *>( target )->setAlphaAdjust1(
        StringConverter::parseReal( val ) );
}
String ColourFaderAffector2FX2::CmdAlphaAdjust2::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffector2FX2 *>( target )->getAlphaAdjust2() );
}
void ColourFaderAffector2FX2::CmdAlphaAdjust2::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffector2FX2 *>( target )->setAlphaAdjust2(
        StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String ColourFaderAffector2FX2::CmdStateChange::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffector2FX2 *>( target )->getStateChange() );
}
void ColourFaderAffector2FX2::CmdStateChange::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffector2FX2 *>( target )->setStateChange(
        StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String ColourFaderAffector2FX2::CmdMinColour::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffector2FX2 *>( target )->getMinColour() );
}
void ColourFaderAffector2FX2::CmdMinColour::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffector2FX2 *>( target )->setMinColour(
        Vector4( StringConverter::parseColourValue( val ).ptr() ) );
}
//-----------------------------------------------------------------------------
String ColourFaderAffector2FX2::CmdMaxColour::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffector2FX2 *>( target )->getMaxColour() );
}
void ColourFaderAffector2FX2::CmdMaxColour::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffector2FX2 *>( target )->setMaxColour(
        Vector4( StringConverter::parseColourValue( val ).ptr() ) );
}
