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
#include "OgreLinearForceAffector2.h"

#include "OgreStringConverter.h"

using namespace Ogre;

// Instantiate statics
LinearForceAffector2::CmdForceVector LinearForceAffector2::msForceVectorCmd;
LinearForceAffector2::CmdForceApp LinearForceAffector2::msForceAppCmd;
//-----------------------------------------------------------------------------
LinearForceAffector2::LinearForceAffector2() :
    mForceVector( 0, -100, 0 ),  // Default to gravity-like
    mForceApplication( FA_ADD )
{
    // Set up parameters
    if( createParamDictionary( "LinearForceAffector2" ) )
    {
        // Add extra parameters
        ParamDictionary *dict = getParamDictionary();
        dict->addParameter(
            ParameterDef( "force_vector", "The vector representing the force to apply.", PT_VECTOR3 ),
            &msForceVectorCmd );
        dict->addParameter( ParameterDef( "force_application",
                                          "How to apply the force vector to particles.", PT_STRING ),
                            &msForceAppCmd );
    }
}
//-----------------------------------------------------------------------------
void LinearForceAffector2::run( ParticleCpuData cpuData, const size_t numParticles,
                                const ArrayReal timeSinceLast ) const
{
    ArrayVector3 forceVector;
    forceVector.setAll( mForceVector );

    if( mForceApplication == FA_ADD )
    {
        // Scale force by time
        const ArrayVector3 scaledVector = forceVector * timeSinceLast;
        for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
        {
            *cpuData.mDirection += scaledVector;
            cpuData.advancePack();
        }
    }
    else
    {
        // FA_AVERAGE
        for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
        {
            *cpuData.mDirection = ( *cpuData.mDirection + forceVector ) * 0.5f;
            cpuData.advancePack();
        }
    }
}
//-----------------------------------------------------------------------------
void LinearForceAffector2::setForceVector( const Vector3 &force ) { mForceVector = force; }
//-----------------------------------------------------------------------------
void LinearForceAffector2::setForceApplication( ForceApplication fa ) { mForceApplication = fa; }
//-----------------------------------------------------------------------------
Vector3 LinearForceAffector2::getForceVector() const { return mForceVector; }
//-----------------------------------------------------------------------------
LinearForceAffector2::ForceApplication LinearForceAffector2::getForceApplication() const
{
    return mForceApplication;
}
//-----------------------------------------------------------------------------
String LinearForceAffector2::getType() const { return "LinearForce"; }
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
String LinearForceAffector2::CmdForceVector::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const LinearForceAffector2 *>( target )->getForceVector() );
}
void LinearForceAffector2::CmdForceVector::doSet( void *target, const String &val )
{
    static_cast<LinearForceAffector2 *>( target )->setForceVector(
        StringConverter::parseVector3( val ) );
}
//-----------------------------------------------------------------------------
String LinearForceAffector2::CmdForceApp::doGet( const void *target ) const
{
    ForceApplication app = static_cast<const LinearForceAffector2 *>( target )->getForceApplication();
    switch( app )
    {
    case LinearForceAffector2::FA_AVERAGE:
        return "average";
        break;
    case LinearForceAffector2::FA_ADD:
        return "add";
        break;
    }
    // Compiler nicety
    return "";
}
void LinearForceAffector2::CmdForceApp::doSet( void *target, const String &val )
{
    if( val == "average" )
    {
        static_cast<LinearForceAffector2 *>( target )->setForceApplication( FA_AVERAGE );
    }
    else if( val == "add" )
    {
        static_cast<LinearForceAffector2 *>( target )->setForceApplication( FA_ADD );
    }
}
