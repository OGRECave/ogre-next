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
#include "OgreColourImageAffector.h"

#include "OgreException.h"
#include "OgreParticle.h"
#include "OgreParticleSystem.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreResourceGroupManager.h"
#include "OgreStringConverter.h"

namespace Ogre
{
    // init statics
    ColourImageAffector::CmdImageAdjust ColourImageAffector::msImageCmd;

    //-----------------------------------------------------------------------
    ColourImageAffector::ColourImageAffector( ParticleSystem *psys ) :
        ParticleAffector( psys ),
        mColourImageLoaded( false )
    {
        mType = "ColourImage";

        // Init parameters
        if( createParamDictionary( "ColourImageAffector" ) )
        {
            ParamDictionary *dict = getParamDictionary();

            dict->addParameter( ParameterDef( "image", "image where the colours come from", PT_STRING ),
                                &msImageCmd );
        }
    }
    //-----------------------------------------------------------------------
    void ColourImageAffector::_initParticle( Particle *pParticle )
    {
        if( !mColourImageLoaded )
        {
            _loadImage();
        }

        pParticle->mColour = mColourImage.getColourAt( 0, 0, 0 );
    }
    //-----------------------------------------------------------------------
    void ColourImageAffector::_affectParticles( ParticleSystem *pSystem, Real timeElapsed )
    {
        Particle *p;
        ParticleIterator pi = pSystem->_getIterator();

        if( !mColourImageLoaded )
        {
            _loadImage();
        }

        const size_t width = mColourImage.getWidth() - 1u;

        while( !pi.end() )
        {
            p = pi.getNext();
            const Real life_time = p->mTotalTimeToLive;
            Real particle_time = 1.0f - ( p->mTimeToLive / life_time );

            if( particle_time > 1.0f )
                particle_time = 1.0f;
            if( particle_time < 0.0f )
                particle_time = 0.0f;

            const Real float_index = particle_time * width;
            const size_t index = static_cast<size_t>( float_index );

            if( index >= width )
            {
                p->mColour = mColourImage.getColourAt( width, 0, 0 );
            }
            else
            {
                // Linear interpolation
                const Real fract = float_index - (Real)index;
                const Real to_colour = fract;
                const Real from_colour = 1.0f - to_colour;

                ColourValue from = mColourImage.getColourAt( index, 0, 0 ),
                            to = mColourImage.getColourAt( index + 1, 0, 0 );

                p->mColour.r = from.r * from_colour + to.r * to_colour;
                p->mColour.g = from.g * from_colour + to.g * to_colour;
                p->mColour.b = from.b * from_colour + to.b * to_colour;
                p->mColour.a = from.a * from_colour + to.a * to_colour;
            }
        }
    }

    //-----------------------------------------------------------------------
    void ColourImageAffector::setImageAdjust( String name )
    {
        mColourImageName = name;
        mColourImageLoaded = false;
    }
    //-----------------------------------------------------------------------
    void ColourImageAffector::_loadImage()
    {
        mColourImage.load( mColourImageName, mParent->getResourceGroupName() );

        PixelFormatGpu format = mColourImage.getPixelFormat();

        if( !PixelFormatGpuUtils::isAccessible( format ) )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Error: Image is not accessible (rgba) image.",
                         "ColourImageAffector::_loadImage" );
        }

        mColourImageLoaded = true;
    }
    //-----------------------------------------------------------------------
    String ColourImageAffector::getImageAdjust() const { return mColourImageName; }

    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    // Command objects
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String ColourImageAffector::CmdImageAdjust::doGet( const void *target ) const
    {
        return static_cast<const ColourImageAffector *>( target )->getImageAdjust();
    }
    void ColourImageAffector::CmdImageAdjust::doSet( void *target, const String &val )
    {
        static_cast<ColourImageAffector *>( target )->setImageAdjust( val );
    }

}  // namespace Ogre
