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
#include "OgreColourImageAffector2.h"

#include "OgreImage2.h"
#include "OgreParticle.h"
#include "OgreParticleSystem.h"
#include "OgrePixelFormatGpuUtils.h"
#include "OgreStringConverter.h"

#include <sstream>

using namespace Ogre;

// init statics
ColourImageAffector2::CmdImageAdjust ColourImageAffector2::msImageCmd;
//-----------------------------------------------------------------------
ColourImageAffector2::ColourImageAffector2() : mInitialized( false )
{
    // Init parameters
    if( createParamDictionary( "ColourImageAffector2" ) )
    {
        ParamDictionary *dict = getParamDictionary();

        dict->addParameter( ParameterDef( "image", "image where the colours come from", PT_STRING ),
                            &msImageCmd );
    }
}
//-----------------------------------------------------------------------
void ColourImageAffector2::run( ParticleCpuData cpuData, const size_t numParticles,
                                const ArrayReal ) const
{
    const size_t widthN2 = mColourData.size() - 2u;
    const ArrayReal fWidthN2 = Mathlib::SetAll( Real( widthN2 ) );

    OGRE_ALIGNED_DECL( int32, scalarIdx[ARRAY_PACKED_REALS], OGRE_SIMD_ALIGNMENT );
    OGRE_ALIGNED_DECL( int32, scalarNextIdx[ARRAY_PACKED_REALS], OGRE_SIMD_ALIGNMENT );

    for( size_t i = 0u; i < numParticles; i += ARRAY_PACKED_REALS )
    {
        const ArrayReal lifeTime = *cpuData.mTotalTimeToLive;
        const ArrayReal particleTime =
            Mathlib::Saturate( Mathlib::ONE - ( *cpuData.mTimeToLive / lifeTime ) );

        const ArrayReal floatIndex = particleTime * fWidthN2;
        const ArrayInt index = Mathlib::Truncate( floatIndex );
        const ArrayInt nextIndex = Mathlib::Truncate( floatIndex + 1.0f );

        CastArrayToInt32( scalarIdx, index );
        CastArrayToInt32( scalarNextIdx, nextIndex );

        ArrayVector4 colourFrom, colourTo;
        for( size_t j = 0u; j < ARRAY_PACKED_REALS; ++j )
        {
            colourFrom.setFromVector4( mColourData[(size_t)scalarIdx[j]], j );
            colourTo.setFromVector4( mColourData[(size_t)scalarNextIdx[j]], j );
        }

        const ArrayReal fW = floatIndex - Mathlib::ConvertToF32( index );
        *cpuData.mColour = Math::lerp( colourFrom, colourTo, fW );

        cpuData.advancePack();
    }
}
//-----------------------------------------------------------------------
void ColourImageAffector2::oneTimeInit()
{
    mInitialized = true;

    Image2 colourImage;
    colourImage.load( mColourImageName, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME );
    const PixelFormatGpu format = colourImage.getPixelFormat();
    if( !PixelFormatGpuUtils::isAccessible( format ) )
    {
        OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                     "Error: Image '" + mColourImageName + "' is not accessible (rgba) image.",
                     "ColourImageAffector2::_loadImage" );
    }

    const size_t width = colourImage.getWidth();
    // We add an extra entry at the end so we don't have to worry about out of bounds when fW = 1.0
    mColourData.resizePOD( width + 1u );

    for( size_t x = 0u; x < width; ++x )
        mColourData[x] = Vector4( colourImage.getColourAt( x, 0u, 0u ).ptr() );
    mColourData[width] = Vector4( colourImage.getColourAt( width - 1u, 0u, 0u ).ptr() );
}
//-----------------------------------------------------------------------
void ColourImageAffector2::setImageAdjust( const String &name )
{
    OGRE_ASSERT_LOW( !mInitialized );
    mColourImageName = name;
}
//-----------------------------------------------------------------------
const String &ColourImageAffector2::getImageAdjust() const
{
    return mColourImageName;
}
//-----------------------------------------------------------------------------------
String ColourImageAffector2::getType() const
{
    return "ColourImage";
}
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
// Command objects
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
String ColourImageAffector2::CmdImageAdjust::doGet( const void *target ) const
{
    return static_cast<const ColourImageAffector2 *>( target )->getImageAdjust();
}
void ColourImageAffector2::CmdImageAdjust::doSet( void *target, const String &val )
{
    static_cast<ColourImageAffector2 *>( target )->setImageAdjust( val );
}
