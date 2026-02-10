#include "vk_pipeline.h"
#include "vk_shader.h"
#include "core/log.h"
#include "geometry/vertex.h"
#include <stddef.h>
#include <string.h>

// Get vertex binding description
static VkVertexInputBindingDescription get_vertex_binding_description(void) {
  return VkVertexInputBindingDescription{
    .binding   = 0,
    .stride    = sizeof(Vertex),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
}

// Get vertex attribute descriptions
static void get_vertex_attribute_descriptions(VkVertexInputAttributeDescription *attrs) {
  // Position
  attrs[0] = VkVertexInputAttributeDescription{
    .binding  = 0,
    .location = 0,
    .format   = VK_FORMAT_R32G32B32_SFLOAT,
    .offset   = offsetof(Vertex, position),
  };

  // Color
  attrs[1] = VkVertexInputAttributeDescription{
    .binding  = 0,
    .location = 1,
    .format   = VK_FORMAT_R32G32B32_SFLOAT,
    .offset   = offsetof(Vertex, color),
  };
}

VkResult vk_pipeline_create(VkDevice device, VkRenderPass render_pass, VkExtent2D extent, VkPipelineContext *ctx) {
  (void)extent; // Currently unused, will be used for non-dynamic viewport
  LOG_INFO("Creating graphics pipeline");

  memset(ctx, 0, sizeof(VkPipelineContext));

  // Load shaders
  VkShaderModule vert_module, frag_module;

  VkResult result = vk_shader_load(device, "shaders/basic.vert.spv", &vert_module);
  if(result != VK_SUCCESS) {
    return result;
  }

  result = vk_shader_load(device, "shaders/basic.frag.spv", &frag_module);
  if(result != VK_SUCCESS) {
    vk_shader_destroy(device, vert_module);
    return result;
  }

  // Shader stages
  VkPipelineShaderStageCreateInfo shader_stages[] = {
    vk_shader_stage_info(VK_SHADER_STAGE_VERTEX_BIT, vert_module),
    vk_shader_stage_info(VK_SHADER_STAGE_FRAGMENT_BIT, frag_module),
  };

  // Vertex input
  VkVertexInputBindingDescription   binding_desc = get_vertex_binding_description();
  VkVertexInputAttributeDescription attr_descs[2];
  get_vertex_attribute_descriptions(attr_descs);

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {
    .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount   = 1,
    .pVertexBindingDescriptions      = &binding_desc,
    .vertexAttributeDescriptionCount = 2,
    .pVertexAttributeDescriptions    = attr_descs,
  };

  // Input assembly
  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  // Viewport and scissor (dynamic state)
  VkPipelineViewportStateCreateInfo viewport_state = {
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount  = 1,
  };

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer = {
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable        = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode             = VK_POLYGON_MODE_FILL,
    .lineWidth               = 1.0f,
    .cullMode                = VK_CULL_MODE_BACK_BIT,
    .frontFace               = VK_FRONT_FACE_CLOCKWISE,
    .depthBiasEnable         = VK_FALSE,
  };

  // Multisampling
  VkPipelineMultisampleStateCreateInfo multisampling = {
    .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable  = VK_FALSE,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  // Color blending
  VkPipelineColorBlendAttachmentState color_blend_attachment = {
    .colorWriteMask
    = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable = VK_FALSE,
  };

  VkPipelineColorBlendStateCreateInfo color_blending = {
    .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable   = VK_FALSE,
    .attachmentCount = 1,
    .pAttachments    = &color_blend_attachment,
  };

  // Dynamic state
  VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state = {
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = 2,
    .pDynamicStates    = dynamic_states,
  };

  // Pipeline layout
  VkPipelineLayoutCreateInfo layout_info = {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount         = 0,
    .pushConstantRangeCount = 0,
  };

  result = vkCreatePipelineLayout(device, &layout_info, NULL, &ctx->layout);
  if(result != VK_SUCCESS) {
    LOG_ERROR("Failed to create pipeline layout: %d", result);
    vk_shader_destroy(device, vert_module);
    vk_shader_destroy(device, frag_module);
    return result;
  }

  // Create graphics pipeline
  VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount          = 2,
    .pStages             = shader_stages,
    .pVertexInputState   = &vertex_input_info,
    .pInputAssemblyState = &input_assembly,
    .pViewportState      = &viewport_state,
    .pRasterizationState = &rasterizer,
    .pMultisampleState   = &multisampling,
    .pDepthStencilState  = NULL,
    .pColorBlendState    = &color_blending,
    .pDynamicState       = &dynamic_state,
    .layout              = ctx->layout,
    .renderPass          = render_pass,
    .subpass             = 0,
    .basePipelineHandle  = VK_NULL_HANDLE,
    .basePipelineIndex   = -1,
  };

  result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &ctx->pipeline);

  // Cleanup shaders (no longer needed after pipeline creation)
  vk_shader_destroy(device, vert_module);
  vk_shader_destroy(device, frag_module);

  if(result != VK_SUCCESS) {
    LOG_ERROR("Failed to create graphics pipeline: %d", result);
    vkDestroyPipelineLayout(device, ctx->layout, NULL);
    return result;
  }

  LOG_INFO("Graphics pipeline created");
  return VK_SUCCESS;
}

void vk_pipeline_destroy(VkDevice device, VkPipelineContext *ctx) {
  if(ctx->pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(device, ctx->pipeline, NULL);
  }
  if(ctx->layout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device, ctx->layout, NULL);
  }

  LOG_DEBUG("Graphics pipeline destroyed");
  memset(ctx, 0, sizeof(VkPipelineContext));
}
