// FILE AUTOMATICALLY GENERATED. Run python3 OgreVulkanDelayedFuncs.py
#include "Vao/OgreVulkanVaoManager.h"

#include "vulkan/vulkan_core.h"

#include "OgreVulkanDelayedFuncs.h"

namespace Ogre
{
    VulkanDelayedFuncBase::~VulkanDelayedFuncBase() {}

    void VulkanDelayed_vkDestroyImage::execute() { vkDestroyImage( device, image, pAllocator ); }

    void delayed_vkDestroyImage( VaoManager *vaoMgr, VkDevice device, VkImage image,
                                 VkAllocationCallbacks const *pAllocator )
    {
        VulkanVaoManager *vkVaoMgr = static_cast<VulkanVaoManager *>( vaoMgr );
        VulkanDelayed_vkDestroyImage *cmd = new VulkanDelayed_vkDestroyImage();
        cmd->device = device;
        cmd->image = image;
        cmd->pAllocator = pAllocator;

        vkVaoMgr->addDelayedFunc( cmd );
    }

    void VulkanDelayed_vkDestroyImageView::execute() { vkDestroyImageView( device, pView, pAllocator ); }

    void delayed_vkDestroyImageView( VaoManager *vaoMgr, VkDevice device, VkImageView pView,
                                     VkAllocationCallbacks const *pAllocator )
    {
        VulkanVaoManager *vkVaoMgr = static_cast<VulkanVaoManager *>( vaoMgr );
        VulkanDelayed_vkDestroyImageView *cmd = new VulkanDelayed_vkDestroyImageView();
        cmd->device = device;
        cmd->pView = pView;
        cmd->pAllocator = pAllocator;

        vkVaoMgr->addDelayedFunc( cmd );
    }

    void VulkanDelayed_vkDestroyPipeline::execute()
    {
        vkDestroyPipeline( device, pipeline, pAllocator );
    }

    void delayed_vkDestroyPipeline( VaoManager *vaoMgr, VkDevice device, VkPipeline pipeline,
                                    VkAllocationCallbacks const *pAllocator )
    {
        VulkanVaoManager *vkVaoMgr = static_cast<VulkanVaoManager *>( vaoMgr );
        VulkanDelayed_vkDestroyPipeline *cmd = new VulkanDelayed_vkDestroyPipeline();
        cmd->device = device;
        cmd->pipeline = pipeline;
        cmd->pAllocator = pAllocator;

        vkVaoMgr->addDelayedFunc( cmd );
    }

    void VulkanDelayed_vkDestroySampler::execute() { vkDestroySampler( device, sampler, pAllocator ); }

    void delayed_vkDestroySampler( VaoManager *vaoMgr, VkDevice device, VkSampler sampler,
                                   VkAllocationCallbacks const *pAllocator )
    {
        VulkanVaoManager *vkVaoMgr = static_cast<VulkanVaoManager *>( vaoMgr );
        VulkanDelayed_vkDestroySampler *cmd = new VulkanDelayed_vkDestroySampler();
        cmd->device = device;
        cmd->sampler = sampler;
        cmd->pAllocator = pAllocator;

        vkVaoMgr->addDelayedFunc( cmd );
    }

    void VulkanDelayed_vkDestroyBufferView::execute()
    {
        vkDestroyBufferView( device, bufferView, pAllocator );
    }

    void delayed_vkDestroyBufferView( VaoManager *vaoMgr, VkDevice device, VkBufferView bufferView,
                                      VkAllocationCallbacks const *pAllocator )
    {
        VulkanVaoManager *vkVaoMgr = static_cast<VulkanVaoManager *>( vaoMgr );
        VulkanDelayed_vkDestroyBufferView *cmd = new VulkanDelayed_vkDestroyBufferView();
        cmd->device = device;
        cmd->bufferView = bufferView;
        cmd->pAllocator = pAllocator;

        vkVaoMgr->addDelayedFunc( cmd );
    }

    void VulkanDelayed_vkDestroyShaderModule::execute()
    {
        vkDestroyShaderModule( device, shaderModule, pAllocator );
    }

    void delayed_vkDestroyShaderModule( VaoManager *vaoMgr, VkDevice device, VkShaderModule shaderModule,
                                        VkAllocationCallbacks const *pAllocator )
    {
        VulkanVaoManager *vkVaoMgr = static_cast<VulkanVaoManager *>( vaoMgr );
        VulkanDelayed_vkDestroyShaderModule *cmd = new VulkanDelayed_vkDestroyShaderModule();
        cmd->device = device;
        cmd->shaderModule = shaderModule;
        cmd->pAllocator = pAllocator;

        vkVaoMgr->addDelayedFunc( cmd );
    }

    void VulkanDelayed_vkDestroyFramebuffer::execute()
    {
        vkDestroyFramebuffer( device, framebuffer, pAllocator );
    }

    void delayed_vkDestroyFramebuffer( VaoManager *vaoMgr, VkDevice device, VkFramebuffer framebuffer,
                                       VkAllocationCallbacks const *pAllocator )
    {
        VulkanVaoManager *vkVaoMgr = static_cast<VulkanVaoManager *>( vaoMgr );
        VulkanDelayed_vkDestroyFramebuffer *cmd = new VulkanDelayed_vkDestroyFramebuffer();
        cmd->device = device;
        cmd->framebuffer = framebuffer;
        cmd->pAllocator = pAllocator;

        vkVaoMgr->addDelayedFunc( cmd );
    }

    void VulkanDelayed_vkDestroyRenderPass::execute()
    {
        vkDestroyRenderPass( device, renderPass, pAllocator );
    }

    void delayed_vkDestroyRenderPass( VaoManager *vaoMgr, VkDevice device, VkRenderPass renderPass,
                                      VkAllocationCallbacks const *pAllocator )
    {
        VulkanVaoManager *vkVaoMgr = static_cast<VulkanVaoManager *>( vaoMgr );
        VulkanDelayed_vkDestroyRenderPass *cmd = new VulkanDelayed_vkDestroyRenderPass();
        cmd->device = device;
        cmd->renderPass = renderPass;
        cmd->pAllocator = pAllocator;

        vkVaoMgr->addDelayedFunc( cmd );
    }

}  // namespace Ogre
