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

#include "Windowing/win32/OgreVulkanWin32Support.h"

#include "OgreException.h"

#include <sstream>

namespace Ogre
{
    template <class C>
    void remove_duplicates( C &c )
    {
        typename C::iterator p = std::unique( c.begin(), c.end() );
        c.erase( p, c.end() );
    }

    static bool OrderByResolution( const DEVMODE &a, const DEVMODE &b )
    {
        if( a.dmPelsWidth != b.dmPelsWidth )
            return a.dmPelsWidth < b.dmPelsWidth;
        return a.dmPelsHeight < b.dmPelsHeight;
    }

    VulkanWin32Support::VulkanWin32Support() {}

    void VulkanWin32Support::addConfig( VulkanRenderSystem *renderSystem )
    {
        VulkanSupport::addConfig( renderSystem );

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
        ConfigOption optVSyncMethod;
        ConfigOption optAllowMemoryless;

        // FS setting possibilities
        optFullScreen.name = "Full Screen";
        optFullScreen.possibleValues.push_back( "Yes" );
        optFullScreen.possibleValues.push_back( "No" );
        optFullScreen.currentValue = "Yes";
        optFullScreen.immutable = false;

        // Video mode possibilities
        {
            DEVMODE DevMode;
            DevMode.dmSize = sizeof( DEVMODE );
            optVideoMode.name = "Video Mode";
            optVideoMode.immutable = false;
            for( DWORD i = 0; EnumDisplaySettings( NULL, i, &DevMode ); ++i )
            {
                if( DevMode.dmBitsPerPel < 16 || DevMode.dmPelsHeight < 480 )
                    continue;
                mDevModes.push_back( DevMode );
            }
        }
        std::sort( mDevModes.begin(), mDevModes.end(), OrderByResolution );
        {
            char tmpBuffer[64];
            LwString resolutionStr( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

            FastArray<DEVMODE>::const_iterator itor = mDevModes.begin();
            FastArray<DEVMODE>::const_iterator endt = mDevModes.end();

            while( itor != endt )
            {
                resolutionStr.clear();
                resolutionStr.a( (uint32)itor->dmPelsWidth, " x ", (uint32)itor->dmPelsHeight );
                optVideoMode.possibleValues.push_back( resolutionStr.c_str() );
                ++itor;
            }
        }

        remove_duplicates( optVideoMode.possibleValues );
        if( !optVideoMode.possibleValues.empty() )
            optVideoMode.currentValue = optVideoMode.possibleValues.back();

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
        mOptions[optColourDepth.name] = optColourDepth;
        mOptions[optDisplayFrequency.name] = optDisplayFrequency;
        mOptions[optVSync.name] = optVSync;
        mOptions[optVSyncInterval.name] = optVSyncInterval;
        mOptions[optVSyncMethod.name] = optVSyncMethod;
        mOptions[optAllowMemoryless.name] = optAllowMemoryless;

        refreshConfig();
    }

    void VulkanWin32Support::refreshConfig()
    {
        ConfigOptionMap::iterator optVideoMode = mOptions.find( "Video Mode" );
        ConfigOptionMap::iterator moptColourDepth = mOptions.find( "Colour Depth" );
        ConfigOptionMap::iterator moptDisplayFrequency = mOptions.find( "Display Frequency" );
        if( optVideoMode == mOptions.end() || moptColourDepth == mOptions.end() ||
            moptDisplayFrequency == mOptions.end() )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Can't find mOptions!",
                         "VulkanWin32Support::refreshConfig" );
        ConfigOption *optColourDepth = &moptColourDepth->second;
        ConfigOption *optDisplayFrequency = &moptDisplayFrequency->second;

        const String &val = optVideoMode->second.currentValue;
        String::size_type pos = val.find( 'x' );
        if( pos == String::npos )
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Invalid Video Mode provided",
                         "VulkanWin32Support::refreshConfig" );
        DWORD width = StringConverter::parseUnsignedInt( val.substr( 0, pos ) );
        DWORD height = StringConverter::parseUnsignedInt( val.substr( pos + 1, String::npos ) );

        for( FastArray<DEVMODE>::const_iterator i = mDevModes.begin(); i != mDevModes.end(); ++i )
        {
            if( i->dmPelsWidth != width || i->dmPelsHeight != height )
                continue;
            optColourDepth->possibleValues.push_back(
                StringConverter::toString( (unsigned int)i->dmBitsPerPel ) );
            optDisplayFrequency->possibleValues.push_back(
                StringConverter::toString( (unsigned int)i->dmDisplayFrequency ) );
        }
        remove_duplicates( optColourDepth->possibleValues );
        remove_duplicates( optDisplayFrequency->possibleValues );
        optColourDepth->currentValue = optColourDepth->possibleValues.back();
        bool freqValid =
            std::find( optDisplayFrequency->possibleValues.begin(),
                       optDisplayFrequency->possibleValues.end(),
                       optDisplayFrequency->currentValue ) != optDisplayFrequency->possibleValues.end();

        if( ( optDisplayFrequency->currentValue != "N/A" ) && !freqValid &&
            !optDisplayFrequency->possibleValues.empty() )
        {
            optDisplayFrequency->currentValue = optDisplayFrequency->possibleValues.front();
        }
    }

    void VulkanWin32Support::setConfigOption( const String &name, const String &value )
    {
        ConfigOptionMap::iterator it = mOptions.find( name );

        // Update
        if( it != mOptions.end() )
            it->second.currentValue = value;
        else
        {
            StringStream str;
            str << "Option named '" << name << "' does not exist.";
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, str.str(),
                         "VulkanWin32Support::setConfigOption" );
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
    IdString VulkanWin32Support::getInterfaceName() const { return "win32"; }
    //-------------------------------------------------------------------------
    String VulkanWin32Support::getInterfaceNameStr() const { return "win32"; }
}  // namespace Ogre
