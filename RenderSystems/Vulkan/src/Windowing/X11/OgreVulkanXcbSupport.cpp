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

#include "Windowing/X11/OgreVulkanXcbSupport.h"

#include "OgreException.h"
#include "OgreLogManager.h"
#include "OgreString.h"

#include <xcb/randr.h>
#include <xcb/xcb.h>

#include "Windowing/X11/OgreVulkanXcbWindow.h"

namespace Ogre
{
    //-----------------------------------------------------------------------------
    VulkanXcbSupport::VulkanXcbSupport() { queryXcb(); }
    //-----------------------------------------------------------------------------
    void VulkanXcbSupport::queryXcb()
    {
        int scr = 0;
        xcb_connection_t *connection = xcb_connect( 0, &scr );
        if( !connection || xcb_connection_has_error( connection ) )
        {
            xcb_disconnect( connection );
            LogManager::getSingleton().logMessage( "XCB: failed to connect to the display server",
                                                   LML_CRITICAL );
            return;
        }

        // Get the screen
        const xcb_setup_t *setup = xcb_get_setup( connection );
        xcb_screen_iterator_t iter = xcb_setup_roots_iterator( setup );
        while( scr-- > 0 )
            xcb_screen_next( &iter );
        xcb_screen_t *screen = iter.data;

        // Create a dummy window
        xcb_window_t windowDummy = xcb_generate_id( connection );
        xcb_create_window( connection, 0, windowDummy, screen->root, 0, 0, 1, 1, 0, 0, 0, 0, 0 );

        xcb_flush( connection );

        // Send a request for screen resources to the X server
        xcb_randr_get_screen_resources_cookie_t screenResCookie;
        memset( &screenResCookie, 0, sizeof( screenResCookie ) );
        screenResCookie = xcb_randr_get_screen_resources( connection, windowDummy );

        // Receive reply from X server
        xcb_randr_get_screen_resources_reply_t *screenResReply = 0;
        screenResReply = xcb_randr_get_screen_resources_reply( connection, screenResCookie, 0 );

        if( !screenResReply )
        {
            LogManager::getSingleton().logMessage(
                "XCB: failed to get a reply from RANDR to get display resolution", LML_CRITICAL );
            xcb_destroy_window( connection, windowDummy );
            xcb_flush( connection );
            xcb_disconnect( connection );
            return;
        }

        size_t numModes = (size_t)xcb_randr_get_screen_resources_modes_length( screenResReply );
        xcb_randr_mode_info_t *modes = xcb_randr_get_screen_resources_modes( screenResReply );

        for( size_t i = 0u; i < numModes; ++i )
        {
            Resolution res;
            res.width = modes[i].width;
            res.height = modes[i].height;

            Frequency freq;
            freq.numerator = modes[i].dot_clock;

            uint16 vtotal = modes[i].vtotal;
            if( modes[i].mode_flags & XCB_RANDR_MODE_FLAG_DOUBLE_SCAN )
                vtotal *= 2u;
            if( modes[i].mode_flags & XCB_RANDR_MODE_FLAG_INTERLACE )
                vtotal /= 2u;
            freq.denominator = modes[i].htotal * vtotal;

            mVideoModes[res].push_back( freq );
        }

        free( screenResReply );

        xcb_destroy_window( connection, windowDummy );
        xcb_flush( connection );
        xcb_disconnect( connection );
    }
    //-----------------------------------------------------------------------------
    void VulkanXcbSupport::addConfig( VulkanRenderSystem *renderSystem )
    {
        VulkanSupport::addConfig( renderSystem );

        ConfigOption optFullScreen;
        ConfigOption optVideoMode;
        ConfigOption optDisplayFrequency;
        ConfigOption optVSync;
        ConfigOption optVSyncInterval;
        ConfigOption optVSyncMethod;
        ConfigOption optAllowMemoryless;

        // FS setting possibilities
        optFullScreen.name = "Full Screen";
        optFullScreen.possibleValues.push_back( "Yes" );
        optFullScreen.possibleValues.push_back( "No" );
        optFullScreen.currentValue = "Yes";
        optFullScreen.immutable = false;

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

        optAllowMemoryless.name = "Allow Memoryless RTT";
        optAllowMemoryless.immutable = false;
        optAllowMemoryless.possibleValues.push_back( "Yes" );
        optAllowMemoryless.possibleValues.push_back( "No" );
        optAllowMemoryless.currentValue = optAllowMemoryless.possibleValues.front();

        mOptions[optFullScreen.name] = optFullScreen;
        mOptions[optVideoMode.name] = optVideoMode;
        mOptions[optDisplayFrequency.name] = optDisplayFrequency;
        mOptions[optVSync.name] = optVSync;
        mOptions[optVSyncInterval.name] = optVSyncInterval;
        mOptions[optVSyncMethod.name] = optVSyncMethod;
        mOptions[optAllowMemoryless.name] = optAllowMemoryless;

        refreshConfig();
    }
    //-----------------------------------------------------------------------------
    void VulkanXcbSupport::refreshConfig()
    {
        ConfigOptionMap::iterator optFullScreen = mOptions.find( "Full Screen" );
        ConfigOptionMap::iterator optVideoMode = mOptions.find( "Video Mode" );
        ConfigOptionMap::iterator optDisplayFrequency = mOptions.find( "Display Frequency" );

        bool bIsFullscreen = false;
        if( optFullScreen != mOptions.end() && optFullScreen->second.currentValue == "Yes" )
            bIsFullscreen = true;

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
                                 "VulkanXcbSupport::refreshConfig" );
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
    }
    //-----------------------------------------------------------------------------
    void VulkanXcbSupport::setConfigOption( const String &name, const String &value )
    {
        ConfigOptionMap::iterator it = mOptions.find( name );

        // Update
        if( it != mOptions.end() )
            it->second.currentValue = value;
        else
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Option named '" + name + "' does not exist.",
                         "VulkanXcbSupport::setConfigOption" );
        }

        if( name == "Video Mode" )
            refreshConfig();

        if( name == "Full Screen" )
        {
            it = mOptions.find( "Display Frequency" );
            if( value == "No" )
            {
                it->second.currentValue = "N/A";
                it->second.immutable = true;
            }
            else
            {
                if( ( it->second.currentValue.empty() || it->second.currentValue == "N/A" ) &&
                    !it->second.possibleValues.empty() )
                {
                    it->second.currentValue = it->second.possibleValues.front();
                }
                it->second.immutable = false;
            }
        }

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
    IdString VulkanXcbSupport::getInterfaceName() const { return "xcb"; }
    //-------------------------------------------------------------------------
    String VulkanXcbSupport::getInterfaceNameStr() const { return "xcb"; }
}  // namespace Ogre
