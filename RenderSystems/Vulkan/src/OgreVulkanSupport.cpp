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

#include "OgreVulkanSupport.h"

#include "OgreVulkanRenderSystem.h"

#include "OgreVulkanUtils.h"

namespace Ogre
{
    void VulkanSupport::enumerateDevices( VulkanRenderSystem *renderSystem )
    {
        mDevices.clear();

        const FastArray<VkPhysicalDevice> &devices = renderSystem->getVkPhysicalDevices();
        uint numDevices = devices.size();

        char tmpBuffer[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE + 32];
        LwString logStr( LwString::FromEmptyPointer( tmpBuffer, sizeof( tmpBuffer ) ) );

        logStr.clear();
        logStr.a( "[Vulkan] Found ", numDevices, " devices" );
        LogManager::getSingleton().logMessage( logStr.c_str() );

        LogManager::getSingleton().logMessage( "[Vulkan] Found devices:" );

        mDevices.reserve( numDevices );
        for( uint32 i = 0u; i < numDevices; ++i )
        {
            VkPhysicalDeviceProperties deviceProps;
            vkGetPhysicalDeviceProperties( devices[i], &deviceProps );

            logStr.clear();
            logStr.a( deviceProps.deviceName, " #", i );
            mDevices.push_back( logStr.c_str() );

            LogManager::getSingleton().logMessage( logStr.c_str() );
        }
    }
    //-------------------------------------------------------------------------
    void VulkanSupport::initialize( VulkanRenderSystem *renderSystem )
    {
        if( !renderSystem->getVkInstance() )
            renderSystem->initializeVkInstance();

        if( mDevices.empty() )
            enumerateDevices( renderSystem );
    }
    //-------------------------------------------------------------------------
    void VulkanSupport::setSupported() { mSupported = true; }
    //-------------------------------------------------------------------------
    void VulkanSupport::addConfig( VulkanRenderSystem *renderSystem )
    {
        initialize( renderSystem );

        ConfigOption optDevices;
        ConfigOption optInterfaces;
        ConfigOption optFSAA;
        ConfigOption optSRGB;

        optDevices.name = "Device";

        FastArray<String>::const_iterator itor = mDevices.begin();
        FastArray<String>::const_iterator endt = mDevices.end();

        optDevices.possibleValues.push_back( "(default)" );

        while( itor != endt )
            optDevices.possibleValues.push_back( *itor++ );

        optDevices.currentValue = mDevices.front();
        optDevices.immutable = false;

        optInterfaces.name = "Interface";
#ifdef OGRE_VULKAN_WINDOW_WIN32
        optInterfaces.possibleValues.push_back( "win32" );
#endif
#ifdef OGRE_VULKAN_WINDOW_XCB
        optInterfaces.possibleValues.push_back( "xcb" );
        // optInterfaces.possibleValues.push_back( "wayland" );
#endif
#ifdef OGRE_VULKAN_WINDOW_ANDROID
        optInterfaces.possibleValues.push_back( "android" );
#endif
#ifdef OGRE_VULKAN_WINDOW_NULL
        optInterfaces.possibleValues.push_back( "null" );
#endif
        optInterfaces.currentValue = optInterfaces.possibleValues.front();
        optInterfaces.immutable = false;

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

        mOptions[optDevices.name] = optDevices;
        mOptions[optInterfaces.name] = optInterfaces;
        mOptions[optFSAA.name] = optFSAA;
        mOptions[optSRGB.name] = optSRGB;
    }
    //-------------------------------------------------------------------------
    void VulkanSupport::setConfigOption( const String &name, const String &value )
    {
        ConfigOptionMap::iterator it = mOptions.find( name );

        if( it == mOptions.end() )
        {
            OGRE_EXCEPT( Exception::ERR_INVALIDPARAMS, "Option named " + name + " does not exist.",
                         "VulkanSupport::setConfigOption" );
        }
        else
        {
            it->second.currentValue = value;
        }
    }
    //-------------------------------------------------------------------------
    String VulkanSupport::validateConfigOptions()
    {
        ConfigOptionMap::iterator it;

        it = mOptions.find( "Device" );
        if( it != mOptions.end() )
        {
            const String deviceName = it->second.currentValue;
            if( deviceName != "(default)" &&
                std::find( mDevices.begin(), mDevices.end(), deviceName ) == mDevices.end() )
            {
                setConfigOption( "Device", "(default)" );
                return "Requested rendering device could not be found, default will be used instead.";
            }
        }

        return BLANKSTRING;
    }
    //-------------------------------------------------------------------------
    uint32 VulkanSupport::getSelectedDeviceIdx() const
    {
        uint32 deviceIdx = 0u;

        ConfigOptionMap::const_iterator it = mOptions.find( "Device" );
        if( it != mOptions.end() )
        {
            const String deviceName = it->second.currentValue;
            FastArray<String>::const_iterator itDevice =
                std::find( mDevices.begin(), mDevices.end(), deviceName );
            if( itDevice != mDevices.end() )
                deviceIdx = uint32( itDevice - mDevices.begin() );
        }

        return deviceIdx;
    }
    //-------------------------------------------------------------------------
    ConfigOptionMap &VulkanSupport::getConfigOptions( VulkanRenderSystem *renderSystem )
    {
        initialize( renderSystem );
        return mOptions;
    }
    //-------------------------------------------------------------------------
    IdString VulkanSupport::getInterfaceName() const { return "null"; }
    //-------------------------------------------------------------------------
    String VulkanSupport::getInterfaceNameStr() const { return "null"; }
}  // namespace Ogre
