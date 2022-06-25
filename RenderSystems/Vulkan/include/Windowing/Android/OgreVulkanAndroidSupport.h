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

#ifndef _OgreVulkanAndroidSupport_H_
#define _OgreVulkanAndroidSupport_H_

#define DEFINING_VK_SUPPORT_IMPL
#include "OgreVulkanSupport.h"

namespace Ogre
{
    class VulkanAndroidWindow;
    class _OgreVulkanExport VulkanAndroidSupport final : public VulkanSupport
    {
        struct Frequency
        {
            uint32 numerator;
            uint32 denominator;

            double toFreq() const
            {
                if( !denominator )
                    return 0;
                else
                    return (double)numerator / double( denominator );
            }
        };

        struct Resolution
        {
            uint16 width;
            uint16 height;
            bool operator<( const Resolution &other ) const
            {
                if( this->width != other.width )
                    return this->width < other.width;
                if( this->height != other.height )
                    return this->height < other.height;
                return false;
            }
        };

        typedef map<Resolution, FastArray<Frequency> >::type VideoModesMap;
        VideoModesMap mVideoModes;

        void refreshConfig();

    public:
        VulkanAndroidSupport();
        /**
         * Add any special config values to the system.
         * Must have a "Full Screen" value that is a bool and a "Video Mode" value
         * that is a string in the form of wxhxb
         */
        void addConfig( VulkanRenderSystem *renderSystem ) override;

        void setConfigOption( const String &name, const String &value ) override;

        virtual IdString getInterfaceName() const override;
        virtual String getInterfaceNameStr() const override;
    };

}  // namespace Ogre

#endif
