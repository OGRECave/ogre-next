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
#include "OgreColourFaderAffectorFX2.h"

#include "OgreStringConverter.h"

using namespace Ogre;

// init statics
ColourFaderAffectorFX2::CmdRedAdjust ColourFaderAffectorFX2::msRedCmd;
ColourFaderAffectorFX2::CmdGreenAdjust ColourFaderAffectorFX2::msGreenCmd;
ColourFaderAffectorFX2::CmdBlueAdjust ColourFaderAffectorFX2::msBlueCmd;
ColourFaderAffectorFX2::CmdAlphaAdjust ColourFaderAffectorFX2::msAlphaCmd;
ColourFaderAffectorFX2::CmdMinColour ColourFaderAffectorFX2::msMinColourCmd;
ColourFaderAffectorFX2::CmdMaxColour ColourFaderAffectorFX2::msMaxColourCmd;
//-----------------------------------------------------------------------------
ColourFaderAffectorFX2::ColourFaderAffectorFX2() :
    mColourAdj( Vector4::ZERO ),
    mMinColour( Vector4::ZERO ),
    mMaxColour( Vector4( Real( 1.0f ) ) )
{
    // Init parameters
    if( createParamDictionary( "ColourFaderAffectorFX2" ) )
    {
        ParamDictionary *dict = getParamDictionary();

        dict->addParameter(
            ParameterDef( "red",
                          "The amount by which to adjust the red component of particles per second.",
                          PT_REAL ),
            &msRedCmd );
        dict->addParameter(
            ParameterDef( "green",
                          "The amount by which to adjust the green component of particles per second.",
                          PT_REAL ),
            &msGreenCmd );
        dict->addParameter(
            ParameterDef( "blue",
                          "The amount by which to adjust the blue component of particles per second.",
                          PT_REAL ),
            &msBlueCmd );
        dict->addParameter(
            ParameterDef( "alpha",
                          "The amount by which to adjust the alpha component of particles per second.",
                          PT_REAL ),
            &msAlphaCmd );

        dict->addParameter(
            ParameterDef( "min_colour", "The minimum colour to clamp against.", PT_COLOURVALUE ),
            &msMinColourCmd );
        dict->addParameter(
            ParameterDef( "max_colour", "The maximum colour to clamp against.", PT_COLOURVALUE ),
            &msMaxColourCmd );
    }
}
//-----------------------------------------------------------------------------
void ColourFaderAffectorFX2::run( ParticleCpuData cpuData, const size_t numParticles,
                                  const ArrayReal timeSinceLast ) const
{
    ArrayVector4 minColour, maxColour, colourAdj;
    minColour.setAll( mMinColour );
    maxColour.setAll( mMaxColour );
    colourAdj.setAll( mColourAdj );
    colourAdj *= timeSinceLast;

    for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
    {
        *cpuData.mColour += colourAdj;
        cpuData.mColour->makeCeil( minColour );
        cpuData.mColour->makeFloor( maxColour );
        cpuData.advancePack();
    }
}
//-----------------------------------------------------------------------------
void ColourFaderAffectorFX2::setMaxColour( const Vector4 &rgba ) { mMaxColour = rgba; }
//-----------------------------------------------------------------------------
const Vector4 &ColourFaderAffectorFX2::getMaxColour() const { return mMaxColour; }
//-----------------------------------------------------------------------------
void ColourFaderAffectorFX2::setMinColour( const Vector4 &rgba ) { mMinColour = rgba; }
//-----------------------------------------------------------------------------
const Vector4 &ColourFaderAffectorFX2::getMinColour() const { return mMinColour; }
//-----------------------------------------------------------------------------
void ColourFaderAffectorFX2::setAdjust( float red, float green, float blue, float alpha )
{
    mColourAdj.x = red;
    mColourAdj.y = green;
    mColourAdj.z = blue;
    mColourAdj.w = alpha;
}
//-----------------------------------------------------------------------------
void ColourFaderAffectorFX2::setRedAdjust( float red ) { mColourAdj.x = red; }
//-----------------------------------------------------------------------------
float ColourFaderAffectorFX2::getRedAdjust() const { return mColourAdj.x; }
//-----------------------------------------------------------------------------
void ColourFaderAffectorFX2::setGreenAdjust( float green ) { mColourAdj.y = green; }
//-----------------------------------------------------------------------------
float ColourFaderAffectorFX2::getGreenAdjust() const { return mColourAdj.y; }
//-----------------------------------------------------------------------------
void ColourFaderAffectorFX2::setBlueAdjust( float blue ) { mColourAdj.z = blue; }
//-----------------------------------------------------------------------------
float ColourFaderAffectorFX2::getBlueAdjust() const { return mColourAdj.z; }
//-----------------------------------------------------------------------------
void ColourFaderAffectorFX2::setAlphaAdjust( float alpha ) { mColourAdj.w = alpha; }
//-----------------------------------------------------------------------------
float ColourFaderAffectorFX2::getAlphaAdjust() const { return mColourAdj.w; }
//-----------------------------------------------------------------------------
String ColourFaderAffectorFX2::getType() const { return "ColourFader"; }
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
String ColourFaderAffectorFX2::CmdRedAdjust::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffectorFX2 *>( target )->getRedAdjust() );
}
void ColourFaderAffectorFX2::CmdRedAdjust::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffectorFX2 *>( target )->setRedAdjust( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String ColourFaderAffectorFX2::CmdGreenAdjust::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffectorFX2 *>( target )->getGreenAdjust() );
}
void ColourFaderAffectorFX2::CmdGreenAdjust::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffectorFX2 *>( target )->setGreenAdjust( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String ColourFaderAffectorFX2::CmdBlueAdjust::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffectorFX2 *>( target )->getBlueAdjust() );
}
void ColourFaderAffectorFX2::CmdBlueAdjust::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffectorFX2 *>( target )->setBlueAdjust( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String ColourFaderAffectorFX2::CmdAlphaAdjust::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffectorFX2 *>( target )->getAlphaAdjust() );
}
void ColourFaderAffectorFX2::CmdAlphaAdjust::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffectorFX2 *>( target )->setAlphaAdjust( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String ColourFaderAffectorFX2::CmdMinColour::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffectorFX2 *>( target )->getMinColour() );
}
void ColourFaderAffectorFX2::CmdMinColour::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffectorFX2 *>( target )->setMinColour(
        Vector4( StringConverter::parseColourValue( val ).ptr() ) );
}
//-----------------------------------------------------------------------------
String ColourFaderAffectorFX2::CmdMaxColour::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourFaderAffectorFX2 *>( target )->getMaxColour() );
}
void ColourFaderAffectorFX2::CmdMaxColour::doSet( void *target, const String &val )
{
    static_cast<ColourFaderAffectorFX2 *>( target )->setMaxColour(
        Vector4( StringConverter::parseColourValue( val ).ptr() ) );
}
