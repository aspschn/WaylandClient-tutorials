#include "render-pass.h"

// C
#include <stdio.h>

#include "device.h"

namespace vk {

RenderPass::RenderPass(VkFormat format,
        std::shared_ptr<Device> device)
{
    // Init.
    this->_vk_render_pass = nullptr;

    // Create.
    VkResult result;

    VkAttachmentDescription attachment_description;
    attachment_description.format = format;
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_description.flags = 0;

    VkAttachmentReference attachment_reference;
    attachment_reference.attachment = 0;
    attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &attachment_reference;

    subpass_description.flags = 0;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = NULL;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = NULL;
    subpass_description.pResolveAttachments = NULL;
    subpass_description.pDepthStencilAttachment = NULL;

    // Subpass dependency.
    VkSubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.attachmentCount = 1;
    create_info.pAttachments = &attachment_description;
    create_info.subpassCount = 1;
    create_info.pSubpasses = &subpass_description;
    create_info.dependencyCount = 1;
    create_info.pDependencies = &dependency;

    create_info.flags = 0;
    create_info.pNext = NULL;

    result = vkCreateRenderPass(device->vk_device(), &create_info,
        NULL, &this->_vk_render_pass);
    if (result != VK_SUCCESS) {
        fprintf(stderr, "Failed to create render pass!\n");
    }
    fprintf(stderr, "Render pass created. - render pass: %p\n",
        this->_vk_render_pass);
}

VkRenderPass RenderPass::vk_render_pass() const
{
    return this->_vk_render_pass;
}

} // namespace vk
