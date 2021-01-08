// FILE AUTOMATICALLY GENERATED. Run python3 OgreVulkanDelayedFuncs.py

// Fix for UNITY builds
#pragma once

namespace Ogre
{
    class VulkanDelayedFuncBase
    {
    public:
        uint32 frameIdx;

        virtual ~VulkanDelayedFuncBase();
        virtual void execute() = 0;
    };

    class VulkanDelayed_vkDestroyImage : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkImage image;
        VkAllocationCallbacks const *pAllocator;

        virtual void execute();
    };
    void delayed_vkDestroyImage( VaoManager *vaoMgr, VkDevice device, VkImage image,
                                 VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyImageView : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkImageView pView;
        VkAllocationCallbacks const *pAllocator;

        virtual void execute();
    };
    void delayed_vkDestroyImageView( VaoManager *vaoMgr, VkDevice device, VkImageView pView,
                                     VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyPipeline : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkPipeline pipeline;
        VkAllocationCallbacks const *pAllocator;

        virtual void execute();
    };
    void delayed_vkDestroyPipeline( VaoManager *vaoMgr, VkDevice device, VkPipeline pipeline,
                                    VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroySampler : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkSampler sampler;
        VkAllocationCallbacks const *pAllocator;

        virtual void execute();
    };
    void delayed_vkDestroySampler( VaoManager *vaoMgr, VkDevice device, VkSampler sampler,
                                   VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyBufferView : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkBufferView bufferView;
        VkAllocationCallbacks const *pAllocator;

        virtual void execute();
    };
    void delayed_vkDestroyBufferView( VaoManager *vaoMgr, VkDevice device, VkBufferView bufferView,
                                      VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyShaderModule : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkShaderModule shaderModule;
        VkAllocationCallbacks const *pAllocator;

        virtual void execute();
    };
    void delayed_vkDestroyShaderModule( VaoManager *vaoMgr, VkDevice device, VkShaderModule shaderModule,
                                        VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyFramebuffer : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkFramebuffer framebuffer;
        VkAllocationCallbacks const *pAllocator;

        virtual void execute();
    };
    void delayed_vkDestroyFramebuffer( VaoManager *vaoMgr, VkDevice device, VkFramebuffer framebuffer,
                                       VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyRenderPass : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkRenderPass renderPass;
        VkAllocationCallbacks const *pAllocator;

        virtual void execute();
    };
    void delayed_vkDestroyRenderPass( VaoManager *vaoMgr, VkDevice device, VkRenderPass renderPass,
                                      VkAllocationCallbacks const *pAllocator );

}  // namespace Ogre
