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
#include "OgreScaleInterpolatorAffector2.h"

#include "OgreStringConverter.h"

#include <sstream>

using namespace Ogre;

// init statics
ScaleInterpolatorAffector2::CmdScaleAdjust ScaleInterpolatorAffector2::msScaleCmd[MAX_STAGES];
ScaleInterpolatorAffector2::CmdTimeAdjust ScaleInterpolatorAffector2::msTimeCmd[MAX_STAGES];
//-----------------------------------------------------------------------------
ScaleInterpolatorAffector2::ScaleInterpolatorAffector2()
{
    for( int i = 0; i < MAX_STAGES; i++ )
    {
        mScaleAdj[i] = Mathlib::ONE;
        // Avoid mTimeAdj[i+1] - mTimeAdj[i] == 0; which causes divisions by 0.
        // And they should all be above 1.0f when unused.
        mTimeAdj[i] = Mathlib::SetAll( 1.0f + static_cast<Real>( i ) );
    }

    // Init parameters
    if( createParamDictionary( "ScaleInterpolatorAffector2" ) )
    {
        ParamDictionary *dict = getParamDictionary();

        for( size_t i = 0; i < MAX_STAGES; i++ )
        {
            msScaleCmd[i].mIndex = i;
            msTimeCmd[i].mIndex = i;

            StringStream stage;
            stage << i;
            String scale_title = String( "scale" ) + stage.str();
            String time_title = String( "time" ) + stage.str();
            String scale_descr = String( "Stage " ) + stage.str() + String( " scale." );
            String time_descr = String( "Stage " ) + stage.str() + String( " time." );

            dict->addParameter( ParameterDef( scale_title, scale_descr, PT_REAL ), &msScaleCmd[i] );
            dict->addParameter( ParameterDef( time_title, time_descr, PT_REAL ), &msTimeCmd[i] );
        }
    }
}
//-----------------------------------------------------------------------------
void ScaleInterpolatorAffector2::run( ParticleCpuData cpuData, const size_t numParticles,
                                      const ArrayReal ) const
{
    ArrayVector2 defaultDimensions = ArrayVector2::UNIT_SCALE;  // TODO: Different from original
    for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
    {
        const ArrayReal lifeTime = *cpuData.mTotalTimeToLive;
        const ArrayReal particleTime = Mathlib::ONE - ( *cpuData.mTimeToLive / lifeTime );

        for( int j = 0; j < MAX_STAGES - 1; j++ )
        {
            /*
            if( !skipStage )
                cpuData.mDimensions = defaultDimensions * lerp( mScaleAdj[j], mScaleAdj[j + 1], fW ) );
            */
            const ArrayMaskR skipStage = Mathlib::CompareLess( particleTime, mTimeAdj[j] );
            const ArrayReal fW = ( particleTime - mTimeAdj[j] ) / ( mTimeAdj[j + 1] - mTimeAdj[j] );
            const ArrayReal scale = Math::lerp( mScaleAdj[j], mScaleAdj[j + 1], fW );

            const ArrayVector2 newDimensions = defaultDimensions * scale;
            cpuData.mDimensions->Cmov4( skipStage, newDimensions );
        }

        cpuData.advancePack();
    }
}
//-----------------------------------------------------------------------------
void ScaleInterpolatorAffector2::setScaleAdjust( size_t index, Real scale )
{
    mScaleAdj[index] = Mathlib::SetAll( scale );
}
//-----------------------------------------------------------------------------
Real ScaleInterpolatorAffector2::getScaleAdjust( size_t index ) const
{
    return Mathlib::Get0( mScaleAdj[index] );
}
//-----------------------------------------------------------------------------
void ScaleInterpolatorAffector2::setTimeAdjust( size_t index, Real time )
{
    mTimeAdj[index] = Mathlib::SetAll( time );
}
//-----------------------------------------------------------------------------
Real ScaleInterpolatorAffector2::getTimeAdjust( size_t index ) const
{
    return Mathlib::Get0( mTimeAdj[index] );
}
//-----------------------------------------------------------------------------------
String ScaleInterpolatorAffector2::getType() const { return "ScaleInterpolator"; }
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
String ScaleInterpolatorAffector2::CmdScaleAdjust::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ScaleInterpolatorAffector2 *>( target )->getScaleAdjust( mIndex ) );
}
void ScaleInterpolatorAffector2::CmdScaleAdjust::doSet( void *target, const String &val )
{
    static_cast<ScaleInterpolatorAffector2 *>( target )->setScaleAdjust(
        mIndex, StringConverter::parseReal( val ) );
}
//-----------------------------------------------------------------------------
String ScaleInterpolatorAffector2::CmdTimeAdjust::doGet( const void *target ) const
{
    return StringConverter::toString(
        static_cast<const ScaleInterpolatorAffector2 *>( target )->getTimeAdjust( mIndex ) );
}
void ScaleInterpolatorAffector2::CmdTimeAdjust::doSet( void *target, const String &val )
{
    static_cast<ScaleInterpolatorAffector2 *>( target )->setTimeAdjust(
        mIndex, StringConverter::parseReal( val ) );
}
