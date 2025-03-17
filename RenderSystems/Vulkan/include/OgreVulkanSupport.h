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

#ifndef _OgreVulkanSupport_H_
#define _OgreVulkanSupport_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreConfigOptionMap.h"
#include "OgreIdString.h"
#include "OgreRenderSystemCapabilities.h"

namespace Ogre
{
    class VulkanRenderSystem;

    class _OgreVulkanExport VulkanSupport
    {
        bool mSupported;

    public:
        VulkanSupport() : mSupported( false ) {}
        virtual ~VulkanSupport() {}

        void setSupported();
        bool isSupported() const { return mSupported; }

        /**
         * Add any special config values to the system.
         * Must have a "Full Screen" value that is a bool and a "Video Mode" value
         * that is a string in the form of wxh
         */
        virtual void addConfig( VulkanRenderSystem *renderSystem );
        virtual void setConfigOption( const String &name, const String &value );

        virtual String validateConfigOptions();

        String getSelectedDeviceName() const;

        ConfigOptionMap &getConfigOptions( VulkanRenderSystem *renderSystem );

        /// @copydoc RenderSystem::getDisplayMonitorCount
        virtual unsigned int getDisplayMonitorCount() const { return 1; }

        virtual IdString getInterfaceName() const;
        virtual String getInterfaceNameStr() const;

    protected:
        // Stored options
        ConfigOptionMap mOptions;
    };
}  // namespace Ogre

#ifndef DEFINING_VK_SUPPORT_IMPL
#    ifdef OGRE_VULKAN_WINDOW_WIN32
#        include "Windowing/win32/OgreVulkanWin32Support.h"
#    endif
#    ifdef OGRE_VULKAN_WINDOW_XCB
#        include "Windowing/X11/OgreVulkanXcbSupport.h"
#    endif
#    ifdef OGRE_VULKAN_WINDOW_ANDROID
#        include "Windowing/Android/OgreVulkanAndroidSupport.h"
#    endif

namespace Ogre
{
    inline VulkanSupport *getVulkanSupport( int i )
    {
        int currSupport = 0;
#    ifdef OGRE_VULKAN_WINDOW_NULL
        {
            if( i == currSupport++ )
                return new VulkanSupport();
        }
#    endif
#    ifdef OGRE_VULKAN_WINDOW_WIN32
        {
            if( i == currSupport++ )
                return new VulkanWin32Support();
        }
#    endif
#    ifdef OGRE_VULKAN_WINDOW_XCB
        {
            if( i == currSupport++ )
                return new VulkanXcbSupport();
        }
#    endif
#    ifdef OGRE_VULKAN_WINDOW_ANDROID
        {
            if( i == currSupport++ )
                return new VulkanAndroidSupport();
        }
#    endif
        return 0;
    }

    inline int getNumVulkanSupports()
    {
        return 0
#    ifdef OGRE_VULKAN_WINDOW_NULL
               + 1
#    endif
#    ifdef OGRE_VULKAN_WINDOW_WIN32
               + 1
#    endif
#    ifdef OGRE_VULKAN_WINDOW_XCB
               + 1
#    endif
#    ifdef OGRE_VULKAN_WINDOW_ANDROID
               + 1
#    endif
            ;
    }
}  // namespace Ogre

#endif

#endif
