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
#include "OgreColourInterpolatorAffector2.h"

#include "OgreParticle.h"
#include "OgreParticleSystem.h"
#include "OgreStringConverter.h"

#include <sstream>

using namespace Ogre;

// init statics
ColourInterpolatorAffector2::CmdColourAdjust ColourInterpolatorAffector2::msColourCmd[MAX_STAGES];
ColourInterpolatorAffector2::CmdTimeAdjust ColourInterpolatorAffector2::msTimeCmd[MAX_STAGES];
//-----------------------------------------------------------------------
ColourInterpolatorAffector2::ColourInterpolatorAffector2()
{
    for( int i = 0; i < MAX_STAGES; i++ )
    {
        // set default colour to transparent grey, transparent since we might not want to display the
        // particle here grey because when a colour component is 0.5f the maximum difference to
        // another colour component is 0.5f
        mColourAdj[i].setAll( Vector4( 0.5f, 0.5f, 0.5f, 0.0f ) );
        // Avoid mTimeAdj[i+1] - mTimeAdj[i] == 0; which causes divisions by 0.
        // And they should all be above 1.0f when unused.
        mTimeAdj[i] = Mathlib::SetAll( 1.0f + static_cast<Real>( i ) );
    }

    // Init parameters
    if( createParamDictionary( "ColourInterpolatorAffector2" ) )
    {
        ParamDictionary *dict = getParamDictionary();

        for( size_t i = 0; i < MAX_STAGES; i++ )
        {
            msColourCmd[i].mIndex = i;
            msTimeCmd[i].mIndex = i;

            StringStream stage;
            stage << i;
            String colour_title = String( "colour" ) + stage.str();
            String time_title = String( "time" ) + stage.str();
            String colour_descr = String( "Stage " ) + stage.str() + String( " colour." );
            String time_descr = String( "Stage " ) + stage.str() + String( " time." );

            dict->addParameter( ParameterDef( colour_title, colour_descr, PT_COLOURVALUE ),
                                &msColourCmd[i] );
            dict->addParameter( ParameterDef( time_title, time_descr, PT_REAL ), &msTimeCmd[i] );
        }
    }
}
//-----------------------------------------------------------------------
void ColourInterpolatorAffector2::run( ParticleCpuData cpuData, const size_t numParticles,
                                       const ArrayReal ) const
{
    for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
    {
        const ArrayReal lifeTime = *cpuData.mTotalTimeToLive;
        const ArrayReal particleTime = Mathlib::ONE - ( *cpuData.mTimeToLive / lifeTime );

        for( int j = 0; j < MAX_STAGES - 1; j++ )
        {
            /*
            if( !skipStage )
                cpuData.mColour = lerp( mColourAdj[j], mColourAdj[j + 1], fW ) );
            */
            const ArrayMaskR skipStage = Mathlib::CompareLess( particleTime, mTimeAdj[j] );
            const ArrayReal fW = ( particleTime - mTimeAdj[j] ) / ( mTimeAdj[j + 1] - mTimeAdj[j] );
            const ArrayVector4 colour = Math::lerp( mColourAdj[j], mColourAdj[j + 1], fW );
            cpuData.mColour->Cmov4( skipStage, colour );
        }

        cpuData.advancePack();
    }
}
//-----------------------------------------------------------------------
void ColourInterpolatorAffector2::setColourAdjust( size_t index, ColourValue colour )
{
    mColourAdj[index].setAll( Vector4( colour.ptr() ) );
}
//-----------------------------------------------------------------------
ColourValue ColourInterpolatorAffector2::getColourAdjust( size_t index ) const
{
    Vector4 individualValue;
    mColourAdj[index].getAsVector4( individualValue, 0u );
    return ColourValue( individualValue );
}
//-----------------------------------------------------------------------
void ColourInterpolatorAffector2::setTimeAdjust( size_t index, Real time )
{
    mTimeAdj[index] = Mathlib::SetAll( time );
}
//-----------------------------------------------------------------------
Real ColourInterpolatorAffector2::getTimeAdjust( size_t index ) const
{
    return Mathlib::Get0( mTimeAdj[index] );
}
//-----------------------------------------------------------------------------------
String ColourInterpolatorAffector2::getType() const
{
    return "ColourInterpolator";
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
String ColourInterpolatorAffector2::CmdColourAdjust::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourInterpolatorAffector2 *>( target )->getColourAdjust( mIndex ) );
}
void ColourInterpolatorAffector2::CmdColourAdjust::doSet( void *target, const String &val )
{
    static_cast<ColourInterpolatorAffector2 *>( target )->setColourAdjust(
        mIndex, StringConverter::parseColourValue( val ) );
}
//-----------------------------------------------------------------------
String ColourInterpolatorAffector2::CmdTimeAdjust::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ColourInterpolatorAffector2 *>( target )->getTimeAdjust( mIndex ) );
}
void ColourInterpolatorAffector2::CmdTimeAdjust::doSet( void *target, const String &val )
{
    static_cast<ColourInterpolatorAffector2 *>( target )->setTimeAdjust(
        mIndex, StringConverter::parseReal( val ) );
}
