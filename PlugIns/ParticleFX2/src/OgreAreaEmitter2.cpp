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
#include "OgreAreaEmitter2.h"

#include "OgreStringConverter.h"

using namespace Ogre;

// Instatiate statics
AreaEmitter2::CmdWidth AreaEmitter2::msWidthCmd;
AreaEmitter2::CmdHeight AreaEmitter2::msHeightCmd;
AreaEmitter2::CmdDepth AreaEmitter2::msDepthCmd;
//-----------------------------------------------------------------------------
bool AreaEmitter2::initDefaults()
{
    // called by the constructor as initDefaults("Type")

    // Defaults
    mDirection = Vector3::UNIT_Z;
    mUp = Vector3::UNIT_Y;
    setSize( 100, 100, 100 );

    // Set up parameters
    if( createParamDictionary( getType() + "Emitter2" ) )
    {
        addBaseParameters();
        ParamDictionary *dict = getParamDictionary();

        // Custom params
        dict->addParameter( ParameterDef( "width", "Width of the shape in world coordinates.", PT_REAL ),
                            &msWidthCmd );
        dict->addParameter(
            ParameterDef( "height", "Height of the shape in world coordinates.", PT_REAL ),
            &msHeightCmd );
        dict->addParameter( ParameterDef( "depth", "Depth of the shape in world coordinates.", PT_REAL ),
                            &msDepthCmd );
        return true;
    }

    return false;
}
//-----------------------------------------------------------------------------
void AreaEmitter2::setDirection( const Vector3 &inDirection )
{
    ParticleEmitter::setDirection( inDirection );

    // Update the ranges
    genAreaAxes();
}
//-----------------------------------------------------------------------------
void AreaEmitter2::setSize( const Vector3 &size )
{
    mSize = size;
    genAreaAxes();
}
//-----------------------------------------------------------------------------
void AreaEmitter2::setSize( Real x, Real y, Real z )
{
    mSize.x = x;
    mSize.y = y;
    mSize.z = z;
    genAreaAxes();
}
//-----------------------------------------------------------------------------
void AreaEmitter2::setWidth( Real width )
{
    mSize.x = width;
    genAreaAxes();
}
//-----------------------------------------------------------------------------
Real AreaEmitter2::getWidth() const
{
    return mSize.x;
}
//-----------------------------------------------------------------------------
void AreaEmitter2::setHeight( Real height )
{
    mSize.y = height;
    genAreaAxes();
}
//-----------------------------------------------------------------------------
Real AreaEmitter2::getHeight() const
{
    return mSize.y;
}
//-----------------------------------------------------------------------------
void AreaEmitter2::setDepth( Real depth )
{
    mSize.z = depth;
    genAreaAxes();
}
//-----------------------------------------------------------------------------
Real AreaEmitter2::getDepth() const
{
    return mSize.z;
}
//-----------------------------------------------------------------------------
void AreaEmitter2::genAreaAxes()
{
    const Vector3 left = mUp.crossProduct( mDirection );

    mXRange = left * ( mSize.x * 0.5f );
    mYRange = mUp * ( mSize.y * 0.5f );
    mZRange = mDirection * ( mSize.z * 0.5f );
}
//-----------------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
String AreaEmitter2::CmdWidth::doGet( const void *target ) const
{
    return StringConverter::toString( static_cast<const AreaEmitter2 *>( target )->getWidth() );
}
void AreaEmitter2::CmdWidth::doSet( void *target, const String &val )
{
    static_cast<AreaEmitter2 *>( target )->setWidth( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String AreaEmitter2::CmdHeight::doGet( const void *target ) const
{
    return StringConverter::toString( static_cast<const AreaEmitter2 *>( target )->getHeight() );
}
void AreaEmitter2::CmdHeight::doSet( void *target, const String &val )
{
    static_cast<AreaEmitter2 *>( target )->setHeight( StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String AreaEmitter2::CmdDepth::doGet( const void *target ) const
{
    return StringConverter::toString( static_cast<const AreaEmitter2 *>( target )->getDepth() );
}
void AreaEmitter2::CmdDepth::doSet( void *target, const String &val )
{
    static_cast<AreaEmitter2 *>( target )->setDepth( StringConverter::parseReal( val ) );
}
