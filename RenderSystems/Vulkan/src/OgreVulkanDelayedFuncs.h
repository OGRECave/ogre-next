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

    class VulkanDelayed_vkDestroyImage final : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkImage image;
        VkAllocationCallbacks const *pAllocator;

        void execute() override;
    };
    void delayed_vkDestroyImage( VaoManager *vaoMgr, VkDevice device, VkImage image,
                                 VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyImageView final : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkImageView pView;
        VkAllocationCallbacks const *pAllocator;

        void execute() override;
    };
    void delayed_vkDestroyImageView( VaoManager *vaoMgr, VkDevice device, VkImageView pView,
                                     VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyPipeline final : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkPipeline pipeline;
        VkAllocationCallbacks const *pAllocator;

        void execute() override;
    };
    void delayed_vkDestroyPipeline( VaoManager *vaoMgr, VkDevice device, VkPipeline pipeline,
                                    VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroySampler final : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkSampler sampler;
        VkAllocationCallbacks const *pAllocator;

        void execute() override;
    };
    void delayed_vkDestroySampler( VaoManager *vaoMgr, VkDevice device, VkSampler sampler,
                                   VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyBufferView final : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkBufferView bufferView;
        VkAllocationCallbacks const *pAllocator;

        void execute() override;
    };
    void delayed_vkDestroyBufferView( VaoManager *vaoMgr, VkDevice device, VkBufferView bufferView,
                                      VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyShaderModule final : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkShaderModule shaderModule;
        VkAllocationCallbacks const *pAllocator;

        void execute() override;
    };
    void delayed_vkDestroyShaderModule( VaoManager *vaoMgr, VkDevice device, VkShaderModule shaderModule,
                                        VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyFramebuffer final : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkFramebuffer framebuffer;
        VkAllocationCallbacks const *pAllocator;

        void execute() override;
    };
    void delayed_vkDestroyFramebuffer( VaoManager *vaoMgr, VkDevice device, VkFramebuffer framebuffer,
                                       VkAllocationCallbacks const *pAllocator );

    class VulkanDelayed_vkDestroyRenderPass final : public VulkanDelayedFuncBase
    {
    public:
        VkDevice device;
        VkRenderPass renderPass;
        VkAllocationCallbacks const *pAllocator;

        void execute() override;
    };
    void delayed_vkDestroyRenderPass( VaoManager *vaoMgr, VkDevice device, VkRenderPass renderPass,
                                      VkAllocationCallbacks const *pAllocator );

}  // namespace Ogre
