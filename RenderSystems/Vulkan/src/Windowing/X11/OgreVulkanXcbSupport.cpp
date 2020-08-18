/*
  -----------------------------------------------------------------------------
  This source file is part of OGRE
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

#include "OgreLogManager.h"

#include <xcb/randr.h>
#include <xcb/xcb.h>

namespace Ogre
{
    template <class C>
    void remove_duplicates( C &c )
    {
        std::sort( c.begin(), c.end() );
        typename C::iterator p = std::unique( c.begin(), c.end() );
        c.erase( p, c.end() );
    }
    //-----------------------------------------------------------------------------
    VulkanXcbSupport::VulkanXcbSupport() { queryXcb(); }
    //-----------------------------------------------------------------------------
    void VulkanXcbSupport::queryXcb( void )
    {
        VideoModes videMode;

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

        size_t numCrtcs = 0;
        xcb_randr_crtc_t *firstCRTC;

        // Get a pointer to the first CRTC and number of CRTCs
        // It is crucial to notice that you are in fact getting
        // an array with firstCRTC being the first element of
        // this array and crtcs_length - length of this array
        if( screenResReply )
        {
            numCrtcs = (size_t)xcb_randr_get_screen_resources_crtcs_length( screenResReply );
            firstCRTC = xcb_randr_get_screen_resources_crtcs( screenResReply );
        }
        else
        {
            LogManager::getSingleton().logMessage(
                "XCB: failed to get a reply from RANDR to get display resolution", LML_CRITICAL );
            xcb_destroy_window( connection, windowDummy );
            xcb_flush( connection );
            xcb_disconnect( connection );
            return;
        }

        // Array of requests to the X server for CRTC info
        FastArray<xcb_randr_get_crtc_info_cookie_t> crtcResCookie;
        crtcResCookie.resize( numCrtcs );
        for( size_t i = 0u; i < numCrtcs; ++i )
            crtcResCookie[i] = xcb_randr_get_crtc_info( connection, *( firstCRTC + i ), 0 );

        // Array of replies from X server
        FastArray<xcb_randr_get_crtc_info_reply_t *> crtcResReply;
        crtcResReply.resize( numCrtcs );
        for( size_t i = 0u; i < numCrtcs; ++i )
        {
            crtcResReply[i] = xcb_randr_get_crtc_info_reply( connection, crtcResCookie[i], 0 );

            VideoModes videoMode;
            videoMode.width = crtcResReply[i]->width;
            videoMode.height = crtcResReply[i]->height;
            mVideoModes.push_back( videoMode );
        }

        xcb_destroy_window( connection, windowDummy );
        xcb_flush( connection );
        xcb_disconnect( connection );
    }
    //-----------------------------------------------------------------------------
    void VulkanXcbSupport::addConfig()
    {
        // TODO: EnumDisplayDevices http://msdn.microsoft.com/library/en-us/gdi/devcons_2303.asp
        /*vector<string> DisplayDevices;
        DISPLAY_DEVICE DisplayDevice;
        DisplayDevice.cb = sizeof(DISPLAY_DEVICE);
        DWORD i=0;
        while (EnumDisplayDevices(NULL, i++, &DisplayDevice, 0) {
            DisplayDevices.push_back(DisplayDevice.DeviceName);
        }*/

        ConfigOption optFullScreen;
        ConfigOption optVideoMode;
        ConfigOption optColourDepth;
        ConfigOption optDisplayFrequency;
        ConfigOption optVSync;
        ConfigOption optVSyncInterval;
        ConfigOption optFSAA;
        ConfigOption optRTTMode;
        ConfigOption optSRGB;
#if OGRE_NO_QUAD_BUFFER_STEREO == 0
        ConfigOption optStereoMode;
#endif

        // FS setting possibilities
        optFullScreen.name = "Full Screen";
        optFullScreen.possibleValues.push_back( "Yes" );
        optFullScreen.possibleValues.push_back( "No" );
        optFullScreen.currentValue = "Yes";
        optFullScreen.immutable = false;

        // Video mode possibilities
        optVideoMode.name = "Video Mode";
        optVideoMode.immutable = false;

        FastArray<VideoModes>::const_iterator itor = mVideoModes.begin();
        FastArray<VideoModes>::const_iterator endt = mVideoModes.end();

        while( itor != endt )
        {
            char tmpBuffer[128];
            LwString resolutionStr( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );
            resolutionStr.a( itor->width, " x ", itor->height );
            optVideoMode.possibleValues.push_back( resolutionStr.c_str() );
            ++itor;
        }
        remove_duplicates( optVideoMode.possibleValues );
        optVideoMode.currentValue = optVideoMode.possibleValues.front();

        optColourDepth.name = "Colour Depth";
        optColourDepth.immutable = false;
        optColourDepth.currentValue.clear();

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

        optFSAA.name = "FSAA";
        optFSAA.immutable = false;
        optFSAA.possibleValues.push_back( "1" );
        optFSAA.possibleValues.push_back( "2" );
        optFSAA.possibleValues.push_back( "4" );
        optFSAA.possibleValues.push_back( "8" );
        optFSAA.possibleValues.push_back( "16" );
        //        for( vector<int>::type::iterator it = mFSAALevels.begin(); it != mFSAALevels.end();
        //        ++it )
        //        {
        //            String val = StringConverter::toString( *it );
        //            optFSAA.possibleValues.push_back( val );
        //            /* not implementing CSAA in GL for now
        //            if (*it >= 8)
        //                optFSAA.possibleValues.push_back(val + " [Quality]");
        //            */
        //        }
        optFSAA.currentValue = "1";

        // SRGB on auto window
        optSRGB.name = "sRGB Gamma Conversion";
        optSRGB.possibleValues.push_back( "Yes" );
        optSRGB.possibleValues.push_back( "No" );
        optSRGB.currentValue = "Yes";
        optSRGB.immutable = false;

        mOptions[optFullScreen.name] = optFullScreen;
        mOptions[optVideoMode.name] = optVideoMode;
        mOptions[optColourDepth.name] = optColourDepth;
        mOptions[optDisplayFrequency.name] = optDisplayFrequency;
        mOptions[optVSync.name] = optVSync;
        mOptions[optVSyncInterval.name] = optVSyncInterval;
        mOptions[optFSAA.name] = optFSAA;
        mOptions[optRTTMode.name] = optRTTMode;
        mOptions[optSRGB.name] = optSRGB;

        refreshConfig();
    }
    //-----------------------------------------------------------------------------
    void VulkanXcbSupport::refreshConfig()
    {
        ConfigOptionMap::iterator optVideoMode = mOptions.find( "Video Mode" );
        ConfigOptionMap::iterator moptColourDepth = mOptions.find( "Colour Depth" );
        ConfigOptionMap::iterator moptDisplayFrequency = mOptions.find( "Display Frequency" );

        if( optVideoMode == mOptions.end() || moptColourDepth == mOptions.end() ||
            moptDisplayFrequency == mOptions.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find mOptions!",
                         "VulkanXcbSupport::refreshConfig" );
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
                if( it->second.currentValue.empty() || it->second.currentValue == "N/A" )
                    it->second.currentValue = it->second.possibleValues.front();
                it->second.immutable = false;
            }
        }
    }
    //-----------------------------------------------------------------------------
    String VulkanXcbSupport::validateConfig()
    {
        // TODO, DX9
        return BLANKSTRING;
    }
}  // namespace Ogre
