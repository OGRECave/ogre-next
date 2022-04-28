/*
-----------------------------------------------------------------------------
This source file is part of OGRE-Next
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-present Torus Knot Software Ltd

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

#include "LightProfiles/OgreIesLoader.h"

#include "OgreImage2.h"

#include "OgreException.h"
#include "OgreLwConstString.h"
#include "OgreStringConverter.h"

namespace Ogre
{
    enum PhotometricType
    {
        PHOTOMETRIC_TYPE_C = 1,
        PHOTOMETRIC_TYPE_B = 2,
        PHOTOMETRIC_TYPE_A = 3
    };
    //-------------------------------------------------------------------------
    IesLoader::IesLoader( const String &filename, const char *iesTextData ) :
        mCandelaMult( 0 ),
        mNumVertAngles( 0 ),
        mNumHorizAngles( 0 ),
        mBallastFactor( 0 ),
        mBallastLampPhotometricFactor( 0 ),
        mLampConeType( LampConeType::Type90 ),
        mLampHorizType( LampHorizType::Type360 ),
        mFilename( filename )
    {
        loadFromString( iesTextData );
    }
    //-------------------------------------------------------------------------
    void IesLoader::skipWhitespace( const char *text, size_t &offset )
    {
        while( text[offset] == ' ' || text[offset] == '\n' || text[offset] == ',' )
            ++offset;
    }
    //-------------------------------------------------------------------------
    void IesLoader::verifyDataIsSorted() const
    {
        FastArray<float>::const_iterator itor = mAngleData.begin();
        FastArray<float>::const_iterator endt = mAngleData.begin() + mNumVertAngles - 1u;
        while( itor != endt )
        {
            if( *itor >= *( itor + 1u ) )
            {
                OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                             "IES '" + mFilename +
                                 "' is invalid: cone angle data must be sorted in ascending order, "
                                 "without repeating values",
                             "IesLoader::verifyDataIsSorted" );
            }
            ++itor;
        }
    }
    //-------------------------------------------------------------------------
    void IesLoader::loadFromString( const char *iesTextData )
    {
        const char *versionStrings[] = { "IESNA:LM-63-1986", "IESNA:LM-63-1991", "IESNA91",
                                         "IESNA:LM-63-1995", "IESNA:LM-63-2002" };
        const size_t numVersionStrings = sizeof( versionStrings ) / sizeof( versionStrings[0] );

        LwConstString iesData( LwConstString::FromUnsafeCStr( iesTextData ) );

        bool validVersion = false;
        for( size_t i = 0u; i < numVersionStrings && !validVersion; ++i )
        {
            const size_t pos = iesData.find( versionStrings[i] );
            if( pos != (size_t)-1 )
                validVersion = true;
        }

        if( !validVersion )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "Cannot recognize '" + mFilename + "' as a valid IES file",
                         "IesLoader::loadFromString" );
        }

        // Jump to the tilt part. We don't care about the keywords
        size_t tiltStartPos = iesData.find( "TILT" );
        if( tiltStartPos == (size_t)-1 )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "IES '" + mFilename + "' is invalid: it has no TILT section",
                         "IesLoader::loadFromString" );
        }

        tiltStartPos = iesData.find( "TILT=NONE", tiltStartPos );
        if( tiltStartPos == (size_t)-1 )
        {
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                         "Only TILT=NONE files is supported. '" + mFilename + "'",
                         "IesLoader::loadFromString" );
        }

        const size_t numHeaderValues = 13u;
        float headerValues[numHeaderValues];
        memset( headerValues, 0, sizeof( headerValues ) );

        size_t dataStart = iesData.find_first_of( '\n', tiltStartPos );
        size_t parsedHeaderValues = 0u;

        while( dataStart != (size_t)-1 && parsedHeaderValues < numHeaderValues )
        {
            skipWhitespace( iesData.c_str(), dataStart );
            headerValues[parsedHeaderValues] = static_cast<float>( atof( iesData.c_str() + dataStart ) );
            dataStart = iesData.find_first_of( " ,\n", dataStart );
            ++parsedHeaderValues;
        }

        if( parsedHeaderValues != numHeaderValues )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "IES '" + mFilename + "' is invalid: too short",
                         "IesLoader::loadFromString" );
        }

        const size_t numLamps = static_cast<size_t>( headerValues[0] );
        if( numLamps != 1u )
        {
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                         "Only 1 lamp per file is supported. '" + mFilename + "'",
                         "IesLoader::loadFromString" );
        }

        // float lumensPerLamp = headerValues[1];
        mCandelaMult = headerValues[2];
        mNumVertAngles = static_cast<uint32>( headerValues[3] );
        mNumHorizAngles = static_cast<uint32>( headerValues[4] );

        if( mNumVertAngles == 0u || mNumHorizAngles == 0u )
        {
            OGRE_EXCEPT(
                Exception::ERR_INVALIDPARAMS,
                "IES '" + mFilename +
                    "' is invalid. Vertical and/or horizontal angles is 0 or not a valid number",
                "IesLoader::loadFromString" );
        }

        const uint32 photometricType = static_cast<uint32>( headerValues[5] );
        if( photometricType != PHOTOMETRIC_TYPE_C )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "IES '" + mFilename + "' only photometric type C files are supported",
                         "IesLoader::loadFromString" );
        }

        // 1 for feet, 2 for meters
        // const uint32 unitsType = static_cast<uint32>( headerValues[6] );

        // const float luminaireWidth = headerValues[7];
        // const float luminaireLength = headerValues[8];
        // const float luminaireHeight = headerValues[9];

        mBallastFactor = headerValues[10];
        mBallastLampPhotometricFactor = headerValues[11];
        // const float inputWatts = headerValues[12];

        const size_t totalNumAngles = mNumVertAngles + mNumHorizAngles;
        mAngleData.resize( mNumVertAngles + mNumHorizAngles );

        size_t parsedAngles = 0u;
        while( dataStart != (size_t)-1 && parsedAngles < totalNumAngles )
        {
            skipWhitespace( iesData.c_str(), dataStart );
            mAngleData[parsedAngles] = static_cast<float>( atof( iesData.c_str() + dataStart ) );
            dataStart = iesData.find_first_of( " ,\n", dataStart );
            ++parsedAngles;
        }

        if( parsedAngles != totalNumAngles )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "IES '" + mFilename + "' contains fewer angle entries than expected",
                         "IesLoader::loadFromString" );
        }

        if( mAngleData[0] == 0.0f && mAngleData[mNumVertAngles - 1u] == 90.0f )
            mLampConeType = LampConeType::Type90;
        else if( mAngleData[0] == 0.0f && mAngleData[mNumVertAngles - 1u] == 180.0f )
            mLampConeType = LampConeType::Type180;
        else
        {
            OGRE_EXCEPT( Exception::ERR_NOT_IMPLEMENTED,
                         "IES '" + mFilename +
                             "' unsupported cone angle! Only range [0;90°] and [0;180°] are supported. "
                             "Vertical Angles: " +
                             StringConverter::toString( mAngleData[0] ) + "-" +
                             StringConverter::toString( mAngleData[mNumVertAngles - 1u] ),
                         "IesLoader::loadFromString" );
        }

        verifyDataIsSorted();

        const float horizAngle =
            fabsf( mAngleData[mNumVertAngles] - mAngleData[mNumVertAngles + mNumHorizAngles - 1u] );

        if( mNumHorizAngles == 1u || fabsf( horizAngle - 360.0f ) <= 1e-6f )
        {
            mLampHorizType = LampHorizType::Type360;
        }
        else if( fabsf( horizAngle - 180.0f ) <= 1e-6f )
            mLampConeType = LampConeType::Type180;
        else if( fabsf( horizAngle - 90.0f ) <= 1e-6f )
            mLampConeType = LampConeType::Type90;
        else
        {
            OGRE_EXCEPT(
                Exception::ERR_NOT_IMPLEMENTED,
                "IES '" + mFilename + "' unsupported horizontal angle: " +
                    StringConverter::toString( mAngleData[mNumVertAngles] ) + "-" +
                    StringConverter::toString( mAngleData[mNumVertAngles + mNumHorizAngles - 1u] ),
                "IesLoader::loadFromString" );
        }

        const size_t numCandelas = mNumVertAngles * mNumHorizAngles;
        mCandelaValues.resize( numCandelas );

        parsedAngles = 0u;
        while( dataStart != (size_t)-1 && parsedAngles < numCandelas )
        {
            skipWhitespace( iesData.c_str(), dataStart );
            mCandelaValues[parsedAngles] = static_cast<float>( atof( iesData.c_str() + dataStart ) );
            dataStart = iesData.find_first_of( " ,\n", dataStart );
            ++parsedAngles;
        }

        if( parsedAngles != numCandelas )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                         "IES '" + mFilename + "' contains fewer candela value entries than expected",
                         "IesLoader::loadFromString" );
        }
    }
    //-------------------------------------------------------------------------
    uint32 IesLoader::getSuggestedTexWidth() const { return mNumVertAngles; }
    //-------------------------------------------------------------------------
    void IesLoader::convertToImage1D( Image2 &inOutImage, uint32 row )
    {
        struct IesData
        {
            float angle;
            float candela;
        };

        const float fScale = mCandelaMult * mBallastFactor * mBallastLampPhotometricFactor;

        const size_t imageWidth = inOutImage.getWidth();
        const size_t numVertAngles = mNumVertAngles;

        const float fWidthMinus1 = static_cast<float>( imageWidth - 1u );
        for( size_t x = 0u; x < imageWidth; ++x )
        {
            // fVertAngle is in range [0; 180]
            float fVertAngle = 180.0f * float( x ) / fWidthMinus1;

            FastArray<float>::const_iterator itAngle =
                std::lower_bound( mAngleData.begin(), mAngleData.begin() + numVertAngles, fVertAngle );

            IesData prevAngle;
            IesData nextAngle;
            if( itAngle == mAngleData.begin() + numVertAngles )
            {
                if( mLampConeType == LampConeType::Type90 )
                {
                    // There's no more data past 90°, use black
                    nextAngle.angle = 180.0f;
                    nextAngle.candela = 0.0f;
                }
                else
                {
                    // This should be impossible and we should never arrive here because fVertAngle is in
                    // range [0; 180] and mAngleData should have that. However due to floating point, we
                    // don't have the guarantee that fVertAngle <= 180.0
                    nextAngle.angle = 180.0f;
                    nextAngle.candela = mCandelaValues[numVertAngles - 1u];
                }
                prevAngle = nextAngle;
            }
            else
            {
                nextAngle.angle = *itAngle;
                const size_t idx = (size_t)( itAngle - mAngleData.begin() );
                nextAngle.candela = mCandelaValues[idx];

                if( itAngle != mAngleData.begin() )
                {
                    --itAngle;

                    prevAngle.angle = *itAngle;
                    const size_t idx2 = (size_t)( itAngle - mAngleData.begin() );
                    prevAngle.candela = mCandelaValues[idx2];
                }
                else
                    prevAngle = nextAngle;
            }

            float fDiv = nextAngle.angle - prevAngle.angle;
            fDiv = fDiv <= 1e-6f ? 1.0f : fDiv;

            const float fW = ( fVertAngle - prevAngle.angle ) / fDiv;
            const float interpCandela = Math::lerp( prevAngle.candela, nextAngle.candela, fW ) / 1024.0f;
            ColourValue colourVal( interpCandela * fScale );
            inOutImage.setColourAt( colourVal, x, row, 0u );
        }
    }
}  // namespace Ogre
