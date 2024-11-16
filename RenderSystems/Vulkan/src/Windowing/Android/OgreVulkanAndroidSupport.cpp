/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE-Next
  (Object-oriented Graphics Rendering Engine)
  For the latest info, see http://www.ogre3d.org

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

#include "Windowing/Android/OgreVulkanAndroidSupport.h"

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreString.h"

namespace Ogre
{
    //-----------------------------------------------------------------------------
    VulkanAndroidSupport::VulkanAndroidSupport() {}
    //-----------------------------------------------------------------------------
    void VulkanAndroidSupport::addConfig( VulkanRenderSystem *renderSystem )
    {
        VulkanSupport::addConfig( renderSystem );

        ConfigOption optVideoMode;
        ConfigOption optDisplayFrequency;
        ConfigOption optVSync;
        ConfigOption optVSyncInterval;
        ConfigOption optVSyncMethod;

        // Video mode possibilities
        optVideoMode.name = "Video Mode";
        optVideoMode.immutable = false;

        VideoModesMap::const_reverse_iterator itor = mVideoModes.rbegin();
        VideoModesMap::const_reverse_iterator endt = mVideoModes.rend();

        while( itor != endt )
        {
            char tmpBuffer[128];
            LwString resolutionStr( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
            resolutionStr.a( itor->first.width, " x ", itor->first.height );
            optVideoMode.possibleValues.push_back( resolutionStr.c_str() );
            ++itor;
        }
        if( !optVideoMode.possibleValues.empty() )
            optVideoMode.currentValue = optVideoMode.possibleValues.front();

        optDisplayFrequency.name = "Display Frequency";
        optDisplayFrequency.immutable = false;
        optDisplayFrequency.currentValue.clear();

        optVSync.name = "VSync";
        optVSync.immutable = false;
        optVSync.possibleValues.push_back( "No" );
        optVSync.possibleValues.push_back( "Yes" );
        optVSync.currentValue = "No";

        optVSyncInterval.name = "VSync Interval";
        optVSyncInterval.immutable = false;
        optVSyncInterval.possibleValues.push_back( "1" );
        optVSyncInterval.possibleValues.push_back( "2" );
        optVSyncInterval.possibleValues.push_back( "3" );
        optVSyncInterval.possibleValues.push_back( "4" );
        optVSyncInterval.currentValue = "1";

        optVSyncMethod.name = "VSync Method";
        optVSyncMethod.immutable = false;
        optVSyncMethod.possibleValues.push_back( "Render Ahead / FIFO" );
        optVSyncMethod.possibleValues.push_back( "Lowest Latency" );
        optVSyncMethod.currentValue = optVSyncMethod.possibleValues.front();

        mOptions[optVideoMode.name] = optVideoMode;
        mOptions[optDisplayFrequency.name] = optDisplayFrequency;
        mOptions[optVSync.name] = optVSync;
        mOptions[optVSyncInterval.name] = optVSyncInterval;
        mOptions[optVSyncMethod.name] = optVSyncMethod;

        refreshConfig();
    }
    //-----------------------------------------------------------------------------
    void VulkanAndroidSupport::refreshConfig()
    {
#if 0
        ConfigOptionMap::iterator optVideoMode = mOptions.find( "Video Mode" );
        ConfigOptionMap::iterator optDisplayFrequency = mOptions.find( "Display Frequency" );

        bool bIsFullscreen = true;

        if( optVideoMode != mOptions.end() && optDisplayFrequency != mOptions.end() )
        {
            optDisplayFrequency->second.possibleValues.clear();
            if( !bIsFullscreen )
            {
                optDisplayFrequency->second.possibleValues.push_back( "N/A" );
            }
            else
            {
                StringVector resStr = StringUtil::split( optVideoMode->second.currentValue, "x" );

                if( resStr.size() != 2u )
                {
                    OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS,
                                 "Malformed resolution string: " + optVideoMode->second.currentValue,
                                 "VulkanAndroidSupport::refreshConfig" );
                }

                Resolution res;
                res.width = static_cast<uint16>( atoi( resStr[0].c_str() ) );
                res.height = static_cast<uint16>( atoi( resStr[1].c_str() ) );

                VideoModesMap::const_iterator itor = mVideoModes.find( res );
                if( itor != mVideoModes.end() )
                {
                    FastArray<Frequency>::const_iterator itFreq = itor->second.begin();
                    FastArray<Frequency>::const_iterator enFreq = itor->second.end();
                    while( itFreq != enFreq )
                    {
                        char tmpBuffer[128];
                        LwString freqStr( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
                        freqStr.a( LwString::Float( (float)itFreq->toFreq(), 2 ) );
                        optDisplayFrequency->second.possibleValues.push_back( freqStr.c_str() );
                        ++itFreq;
                    }
                }
                else
                {
                    optDisplayFrequency->second.possibleValues.push_back( "N/A" );
                }

                if( !optDisplayFrequency->second.possibleValues.empty() )
                {
                    optDisplayFrequency->second.currentValue =
                        optDisplayFrequency->second.possibleValues.back();
                }
            }
        }
#endif
    }
    //-----------------------------------------------------------------------------
    void VulkanAndroidSupport::setConfigOption( const String &name, const String &value )
    {
        ConfigOptionMap::iterator it = mOptions.find( name );

        // Update
        if( it != mOptions.end() )
            it->second.currentValue = value;
        else
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Option named '" + name + "' does not exist.",
                         "VulkanAndroidSupport::setConfigOption" );
        }

        if( name == "Video Mode" )
            refreshConfig();

        if( name == "VSync" )
        {
            it = mOptions.find( "VSync Method" );
            if( !StringConverter::parseBool( value ) )
            {
                it->second.currentValue = "N/A";
                it->second.immutable = true;
            }
            else
            {
                it->second.currentValue = it->second.possibleValues.front();
                it->second.immutable = false;
            }
        }
    }
    //-------------------------------------------------------------------------
	IdString VulkanAndroidSupport::getInterfaceName() const { return "android"; }
    //-------------------------------------------------------------------------
	String VulkanAndroidSupport::getInterfaceNameStr() const { return "android"; }
}  // namespace Ogre
