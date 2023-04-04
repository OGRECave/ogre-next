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

#ifndef _OgreVulkanDevice_H_
#define _OgreVulkanDevice_H_

#include "OgreVulkanPrerequisites.h"

#include "OgreVulkanQueue.h"

#include "vulkan/vulkan_core.h"

#include "OgreHeaderPrefix.h"

namespace Ogre
{
    /// Use it to pass an external instance
    ///
    /// We will verify if the layers and extensions you claim
    /// were enabled are actually supported.
    ///
    /// This is so because in Qt you can request these layers/extensions
    /// but you get no feedback from Qt whether they were present and
    /// thus successfully enabled.
    ///
    /// However if the instance actually supports the layer/extension
    /// you requested but the third party library explicitly chose not to
    /// enable it for any random reason, then we will wrongly think
    /// it's enabled / present.
    struct VulkanExternalInstance
    {
        VkInstance instance;
        FastArray<VkLayerProperties> instanceLayers;
        FastArray<VkExtensionProperties> instanceExtensions;
    };

    /// Use it to pass an external device
    ///
    /// See VulkanExternalInstance on extensions verification.
    struct VulkanExternalDevice
    {
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        FastArray<VkExtensionProperties> deviceExtensions;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
    };

    struct _OgreVulkanExport VulkanDevice
    {
        struct SelectedQueue
        {
            VulkanQueue::QueueFamily usage;
            uint32 familyIdx;
            uint32 queueIdx;
            SelectedQueue();
        };

        struct ExtraVkFeatures
        {
            // VkPhysicalDevice16BitStorageFeatures
            VkBool32 storageInputOutput16;

            // VkPhysicalDeviceShaderFloat16Int8Features
            VkBool32 shaderFloat16;
            VkBool32 shaderInt8;
        };

        // clang-format off
        VkInstance          mInstance;
        VkPhysicalDevice    mPhysicalDevice;
        VkDevice            mDevice;

        VkQueue             mPresentQueue;
        /// Graphics queue is *guaranteed by spec* to also be able to run compute and transfer
        /// A GPU may not have a graphics queue though (Ogre can't run there)
        VulkanQueue             mGraphicsQueue;
        /// Additional compute queues to run async compute (besides the main graphics one)
        FastArray<VulkanQueue>  mComputeQueues;
        /// Additional transfer queues to run async transfers (besides the main graphics one)
        FastArray<VulkanQueue>  mTransferQueues;
        // clang-format on

        VkPhysicalDeviceProperties mDeviceProperties;
        VkPhysicalDeviceMemoryProperties mDeviceMemoryProperties;
        VkPhysicalDeviceFeatures mDeviceFeatures;
        ExtraVkFeatures mDeviceExtraFeatures;
        FastArray<VkQueueFamilyProperties> mQueueProps;

        /// Extensions requested when created. Sorted
        FastArray<IdString> mDeviceExtensions;

        VulkanVaoManager *mVaoManager;
        VulkanRenderSystem *mRenderSystem;

        uint32 mSupportedStages;

        bool mIsExternal;

        void fillDeviceFeatures();

        static void destroyQueues( FastArray<VulkanQueue> &queueArray );

        void findGraphicsQueue( FastArray<uint32> &inOutUsedQueueCount );
        void findComputeQueue( FastArray<uint32> &inOutUsedQueueCount, uint32 maxNumQueues );
        void findTransferQueue( FastArray<uint32> &inOutUsedQueueCount, uint32 maxNumQueues );

        void fillQueueCreationInfo( uint32 maxComputeQueues, uint32 maxTransferQueues,
                                    FastArray<VkDeviceQueueCreateInfo> &outQueueCiArray );

    public:
        VulkanDevice( VkInstance instance, uint32 deviceIdx, VulkanRenderSystem *renderSystem );
        VulkanDevice( VkInstance instance, const VulkanExternalDevice &externalDevice,
                      VulkanRenderSystem *renderSystem );
        ~VulkanDevice();

    protected:
        static VkDebugReportCallbackCreateInfoEXT addDebugCallback(
            PFN_vkDebugReportCallbackEXT debugCallback, RenderSystem *renderSystem );

    public:
        static VkInstance createInstance( const String &appName, FastArray<const char *> &extensions,
                                          FastArray<const char *> &layers,
                                          PFN_vkDebugReportCallbackEXT debugCallback,
                                          RenderSystem *renderSystem );

        static void addExternalInstanceExtensions( FastArray<VkExtensionProperties> &extensions );

    protected:
        void createPhysicalDevice( uint32 deviceIdx );

    public:
        void createDevice( FastArray<const char *> &extensions, uint32 maxComputeQueues,
                           uint32 maxTransferQueues );

        bool hasDeviceExtension( const IdString extension ) const;

        static bool hasInstanceExtension( const IdString extension );

        void initQueues();

        void commitAndNextCommandBuffer(
            SubmissionType::SubmissionType submissionType = SubmissionType::FlushOnly );

        /// Waits for the GPU to finish all pending commands.
        void stall();
    };

    // Mask away read flags from srcAccessMask
    static const uint32 c_srcValidAccessFlags =
        0xFFFFFFFF ^
        ( VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
          VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |
          VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
          VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
          VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_HOST_READ_BIT | VK_ACCESS_MEMORY_READ_BIT |
          VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT |
          VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT |
          VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT |
          VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV |
          VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT | VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV );
}  // namespace Ogre

#include "OgreHeaderSuffix.h"

#endif
