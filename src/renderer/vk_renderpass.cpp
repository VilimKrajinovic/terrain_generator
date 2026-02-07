#include "vk_renderpass.h"
#include "core/log.h"
#include <string.h>

VkResult vk_renderpass_create(VkDevice device, VkFormat color_format, VkRenderPassContext *ctx) {
    LOG_INFO("Creating render pass");

    memset(ctx, 0, sizeof(VkRenderPassContext));

    // Color attachment
    VkAttachmentDescription color_attachment = {
        .format = color_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    // Attachment reference
    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    // Subpass
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
    };

    // Subpass dependency
    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    // Create render pass
    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VkResult result = vkCreateRenderPass(device, &render_pass_info, NULL, &ctx->render_pass);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Failed to create render pass: %d", result);
        return result;
    }

    LOG_INFO("Render pass created");
    return VK_SUCCESS;
}

void vk_renderpass_destroy(VkDevice device, VkRenderPassContext *ctx) {
    if (ctx->render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, ctx->render_pass, NULL);
    }
    LOG_DEBUG("Render pass destroyed");
    memset(ctx, 0, sizeof(VkRenderPassContext));
}
