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
#include "OgreScaleAffector2.h"

#include "OgreStringConverter.h"

using namespace Ogre;

// init statics
ScaleAffector2::CmdScaleAdjust ScaleAffector2::msScaleCmd;
//-----------------------------------------------------------------------------
ScaleAffector2::ScaleAffector2() : mScaleAdj( 0 )
{
    // Init parameters
    if( createParamDictionary( "ScaleAffector2" ) )
    {
        ParamDictionary *dict = getParamDictionary();

        dict->addParameter( ParameterDef( "rate",
                                          "The amount by which to adjust the x and y scale "
                                          "components of particles per second.",
                                          PT_REAL ),
                            &msScaleCmd );
    }
}
//-----------------------------------------------------------------------------
void ScaleAffector2::run( ParticleCpuData cpuData, const size_t numParticles,
                          const ArrayReal timeSinceLast ) const
{
    // Scale adjustments by time
    const ArrayReal ds = Mathlib::SetAll( mScaleAdj ) * timeSinceLast;
    for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
    {
        *cpuData.mDimensions += ds;
        cpuData.advancePack();
    }
}
//-----------------------------------------------------------------------------
void ScaleAffector2::setAdjust( Real rate )
{
    mScaleAdj = rate;
}
//-----------------------------------------------------------------------------
Real ScaleAffector2::getAdjust() const
{
    return mScaleAdj;
}
//-----------------------------------------------------------------------------
String ScaleAffector2::getType() const
{
    return "Scaler";
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
String ScaleAffector2::CmdScaleAdjust::doGet( const void *target ) const
{
    return StringConverter::toString( static_cast<const ScaleAffector2 *>( target )->getAdjust() );
}
void ScaleAffector2::CmdScaleAdjust::doSet( void *target, const String &val )
{
    static_cast<ScaleAffector2 *>( target )->setAdjust( StringConverter::parseReal( val ) );
}
