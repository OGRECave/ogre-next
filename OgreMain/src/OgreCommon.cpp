/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#include "OgreStableHeaders.h"

#include "OgrePrerequisites.h"

#include "OgreCommon.h"
#include "OgreString.h"

#include "OgreLogManager.h"
#include "OgreStringConverter.h"

#include "OgreLwString.h"

namespace Ogre 
{
    int findCommandLineOpts(int numargs, char** argv, UnaryOptionList& unaryOptList, 
        BinaryOptionList& binOptList)
    {
        int startIndex = 1;
        for (int i = 1; i < numargs; ++i)
        {
            String tmp(argv[i]);
            if (StringUtil::startsWith(tmp, "-"))
            {
                int indexIncrement = 0;
                UnaryOptionList::iterator ui = unaryOptList.find(argv[i]);
                if(ui != unaryOptList.end())
                {
                    ui->second = true;
                    indexIncrement = 1;
                }
                BinaryOptionList::iterator bi = binOptList.find(argv[i]);
                if(bi != binOptList.end())
                {
                    bi->second = argv[i+1];
                    indexIncrement = 2;
                    ++i;
                }

                startIndex += indexIncrement;

                if( !indexIncrement )
                {
                    // Invalid option
                    LogManager::getSingleton().logMessage("Invalid option " + tmp, LML_CRITICAL);
                }

            }
        }
        return startIndex;
    }
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------
    void SampleDescription::_set( uint8 colourSamples, uint8 coverageSamples,
                                  MsaaPatterns::MsaaPatterns pattern )
    {
        mColourSamples = colourSamples;
        mCoverageSamples = coverageSamples;
        mPattern = pattern;
    }
    //-----------------------------------------------------------------------------------
    void SampleDescription::setMsaa( uint8 msaa, MsaaPatterns::MsaaPatterns pattern )
    {
        mColourSamples = msaa;
        mCoverageSamples = 0u;
        mPattern = pattern;
    }
    //-----------------------------------------------------------------------------------
    bool SampleDescription::isMsaa( void ) const
    {
        return mCoverageSamples == 0u;
    }
    //-----------------------------------------------------------------------------------
    void SampleDescription::setCsaa( uint8 samples, bool bQuality )
    {
        if( samples != 8u && samples != 16u )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "CSAA setting must be 8x or 16x",
                         "SampleDescription::setCsaa" );
        }

        mColourSamples = bQuality ? 8u : 4u;
        mCoverageSamples = samples;
        mPattern = MsaaPatterns::Undefined;
    }
    //-----------------------------------------------------------------------------------
    void SampleDescription::setEqaa( uint8 colourSamples, uint8 coverageSamples )
    {
        mColourSamples = colourSamples;
        mCoverageSamples = coverageSamples;
        mPattern = MsaaPatterns::Undefined;
    }
    //-----------------------------------------------------------------------------------
    void SampleDescription::parseString( const String &fsaaSetting )
    {
        const uint8 samples = std::max( 1u, StringConverter::parseUnsignedInt( fsaaSetting ) );
        const bool csaa = fsaaSetting.find( "CSAA" ) != String::npos;
        const bool eqaa = fsaaSetting.find( "EQAA" ) != String::npos;

        if( csaa )
        {
            // "8x CSAA", "8xQ CSAA", "16x CSAA", "16xQ CSAA", also "8 [Quality]", "16 [Quality]"
            bool qualityHint = samples >= 8u && ( fsaaSetting.find( "Quality" ) != String::npos ||
                                                  fsaaSetting.find( "xQ CSAA" ) != String::npos );
            setCsaa( samples, qualityHint );
        }
        else if( eqaa )
        {
            // "2f4x EQAA", "4f8x EQAA", "4f16x EQAA", "8f16x EQAA", but not Dx9 only "2f8x EQAA"
            if( fsaaSetting.find("4f16x") != String::npos )
                setEqaa( 4, 16 );
            else
                setEqaa( samples, static_cast<uint8>( samples << 1u ) );
        }
        else
            setMsaa( samples ); // "Nx MSAA", also "Nx", "N"
    }
    //-----------------------------------------------------------------------------------
    bool SampleDescription::isCsaa( void ) const
    {
        // {4,8} {8,8} {4,16} {8,16} 
        return ( mCoverageSamples == 8u || mCoverageSamples == 16u )
            && ( mColourSamples == 4u || mColourSamples == 8u );
    }
    //-----------------------------------------------------------------------------------
    bool SampleDescription::isCsaaQuality( void ) const
    {
        // {8,8} {8,16}, but not the {4,8} {4,16} 
        return ( mCoverageSamples == 8u || mCoverageSamples == 16u )
            && mColourSamples == 8u;
    }
    //-----------------------------------------------------------------------------------
    void SampleDescription::getFsaaDesc( LwString &outFsaaSetting ) const
    {
        if( mCoverageSamples == 0 )
            outFsaaSetting.a( mColourSamples, "x MSAA" );
        else if( isCsaaQuality() )
            outFsaaSetting.a( mCoverageSamples, "xQ CSAA" );
        else if( isCsaa() )
            outFsaaSetting.a( mCoverageSamples, "x CSAA" );
        else
            outFsaaSetting.a( mColourSamples, "f", mCoverageSamples, "x EQAA" );
    }
    //-----------------------------------------------------------------------------------
    void SampleDescription::getFsaaDesc( String &outFsaaSetting ) const
    {
        char tmpBuffer[92];
        LwString desc( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
        getFsaaDesc( desc );
        outFsaaSetting += desc.c_str();
    }
}  // namespace Ogre
