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
#include "OgreScaleInterpolatorAffector.h"

#include "OgreParticle.h"
#include "OgreParticleSystem.h"
#include "OgreStringConverter.h"

#include <sstream>

namespace Ogre
{
    // init statics
    ScaleInterpolatorAffector::CmdScaleAdjust ScaleInterpolatorAffector::msScaleCmd[MAX_STAGES];
    ScaleInterpolatorAffector::CmdTimeAdjust ScaleInterpolatorAffector::msTimeCmd[MAX_STAGES];

    //-----------------------------------------------------------------------
    ScaleInterpolatorAffector::ScaleInterpolatorAffector( ParticleSystem *psys ) :
        ParticleAffector( psys )
    {
        for( int i = 0; i < MAX_STAGES; i++ )
        {
            mScaleAdj[i] = 1.0f;
            mTimeAdj[i] = 1.0f;
        }

        mType = "ScaleInterpolator";

        // Init parameters
        if( createParamDictionary( "ScaleInterpolatorAffector" ) )
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
    //-----------------------------------------------------------------------
    void ScaleInterpolatorAffector::_affectParticles( ParticleSystem *pSystem, Real timeElapsed )
    {
        Particle *p;
        ParticleIterator pi = pSystem->_getIterator();

        while( !pi.end() )
        {
            p = pi.getNext();
            const Real life_time = p->mTotalTimeToLive;
            Real particle_time = 1.0f - ( p->mTimeToLive / life_time );

            Real scale;
            if( particle_time <= mTimeAdj[0] )
            {
                scale = mScaleAdj[0];
            }
            else if( particle_time >= mTimeAdj[MAX_STAGES - 1] )
            {
                scale = mScaleAdj[MAX_STAGES - 1];
            }
            else
            {
                scale = Real( 1.0f );
                for( int i = 0; i < MAX_STAGES - 1; i++ )
                {
                    if( particle_time >= mTimeAdj[i] && particle_time < mTimeAdj[i + 1] )
                    {
                        particle_time -= mTimeAdj[i];
                        particle_time /= ( mTimeAdj[i + 1] - mTimeAdj[i] );
                        scale = ( ( mScaleAdj[i + 1] * particle_time ) +
                                  ( mScaleAdj[i] * ( 1.0f - particle_time ) ) );
                        break;
                    }
                }
            }

            p->setDimensions( pSystem->getDefaultWidth() * scale, pSystem->getDefaultHeight() * scale );
        }
    }

    //-----------------------------------------------------------------------
    void ScaleInterpolatorAffector::setScaleAdjust( size_t index, Real scale )
    {
        mScaleAdj[index] = scale;
    }
    //-----------------------------------------------------------------------
    Real ScaleInterpolatorAffector::getScaleAdjust( size_t index ) const { return mScaleAdj[index]; }

    //-----------------------------------------------------------------------
    void ScaleInterpolatorAffector::setTimeAdjust( size_t index, Real time ) { mTimeAdj[index] = time; }
    //-----------------------------------------------------------------------
    Real ScaleInterpolatorAffector::getTimeAdjust( size_t index ) const { return mTimeAdj[index]; }

    //-----------------------------------------------------------------------
    void ScaleInterpolatorAffector::_initParticle( Ogre::Particle *pParticle )
    {
        pParticle->setDimensions( mParent->getDefaultWidth() * mScaleAdj[0],
                                  mParent->getDefaultHeight() * mScaleAdj[0] );
    }

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // Command objects
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String ScaleInterpolatorAffector::CmdScaleAdjust::doGet( const void *target ) const
    {
        return StringConverter::toString(
            static_cast<const ScaleInterpolatorAffector *>( target )->getScaleAdjust( mIndex ) );
    }
    void ScaleInterpolatorAffector::CmdScaleAdjust::doSet( void *target, const String &val )
    {
        static_cast<ScaleInterpolatorAffector *>( target )->setScaleAdjust(
            mIndex, StringConverter::parseReal( val ) );
    }
    //-----------------------------------------------------------------------
    String ScaleInterpolatorAffector::CmdTimeAdjust::doGet( const void *target ) const
    {
        return StringConverter::toString(
            static_cast<const ScaleInterpolatorAffector *>( target )->getTimeAdjust( mIndex ) );
    }
    void ScaleInterpolatorAffector::CmdTimeAdjust::doSet( void *target, const String &val )
    {
        static_cast<ScaleInterpolatorAffector *>( target )->setTimeAdjust(
            mIndex, StringConverter::parseReal( val ) );
    }

}  // namespace Ogre
