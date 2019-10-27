/*
-----------------------------------------------------------------------------
This source file is part of OGRE
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

#ifndef _OgreVulkanDevice_H_
#define _OgreVulkanDevice_H_

#include "OgreVulkanPrerequisites.h"

#include "vulkan/vulkan_core.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    struct _OgreVulkanExport VulkanDevice
    {
        enum QueueFamily
        {
            Graphics,
            Compute,
            Transfer,
            NumQueueFamilies
        };
        struct SelectedQueue
        {
            uint32 familyIdx;
            uint32 queueIdx;

            SelectedQueue();

            bool hasValidFamily( void ) const;
        };

        // clang-format off
        VkInstance          mInstance;
        VkPhysicalDevice    mPhysicalDevice;
        VkDevice            mDevice;
        // clang-format on

        VkQueue mQueues[NumQueueFamilies];

        VkPhysicalDeviceMemoryProperties mMemoryProperties;
        FastArray<VkQueueFamilyProperties> mQueueProps;
        // Index via mQueueProps[mSelectedQueues[Graphics].familyIdx]
        SelectedQueue mSelectedQueues[NumQueueFamilies];

    protected:
        /** Modifies mSelectedQueues[family].queueIdx; attempting to have each QueueFamily its own
            unique queue, but share if there's HW limitations.
        */
        void calculateQueueIdx( QueueFamily family );

        /** Calls calculateQueueIdx on all families except the first one (Graphics)

            Then fills:
                * VkDeviceQueueCreateInfo::queueFamilyIndex
                * VkDeviceQueueCreateInfo::queueCount

            The rest of VkDeviceQueueCreateInfo's members are not read nor written

            @see    VulkanDevice::calculateQueueIdx
        @param outQueueCreateInfo
            Pointer for us to fill. Must have a size of at least outQueueCreateInfo[NumQueueFamilies]
        @param outNumQueues
            Outputs value in range [1; NumQueueFamilies]
        */
        void fillQueueSelectionData( VkDeviceQueueCreateInfo *outQueueCreateInfo, uint32 &outNumQueues );

    public:
        VulkanDevice( VkInstance instance, uint32 deviceIdx );

        static VkInstance createInstance( const String &appName,
                                          const FastArray<const char *> &extensions );

        void createPhysicalDevice( uint32 deviceIdx );

        void createDevice( FastArray<const char *> &extensions );
    };

}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
