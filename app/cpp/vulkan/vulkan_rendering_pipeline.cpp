#include "vulkan_rendering_pipeline.hpp"

vulkan::VulkanRenderingPipeline::VulkanRenderingPipeline(
    std::shared_ptr<VulkanRenderingContext> context,
    std::shared_ptr<VulkanShader> vertex_shader,
    std::shared_ptr<VulkanShader> fragment_shader,
    const VertexBufferLayout &vbl,
    RenderingPipelineConfig config) :
    context_(context),
    device_(context_->GetDevice()),
    config_(config) {
  this->vertex_shader_ = std::dynamic_pointer_cast<VulkanShader>(vertex_shader);
  this->fragment_shader_ = std::dynamic_pointer_cast<VulkanShader>(fragment_shader);
  CreatePipeline(vbl);
}

void vulkan::VulkanRenderingPipeline::SetVertexBuffer(std::shared_ptr<VulkanBuffer> buffer) {
  this->vertex_buffer_ = std::dynamic_pointer_cast<VulkanBuffer>(buffer);
}

void vulkan::VulkanRenderingPipeline::SetIndexBuffer(std::shared_ptr<VulkanBuffer> buffer,
                                                     DataType element_type) {
  this->index_buffer_ = std::dynamic_pointer_cast<VulkanBuffer>(buffer);
  this->index_type_ = GetVkType(element_type);
}

void vulkan::VulkanRenderingPipeline::CreatePipeline(const VertexBufferLayout &vbl) {
  VkPipelineShaderStageCreateInfo shader_stages[] = {
      vertex_shader_->GetShaderStageInfo(),
      fragment_shader_->GetShaderStageInfo()
  };
  auto vertex_push_constants = vertex_shader_->GetPushConstants();
  auto fragment_push_constants = fragment_shader_->GetPushConstants();
  auto pipeline_push_constants = vertex_push_constants;
  pipeline_push_constants.reserve(pipeline_push_constants.size() + fragment_push_constants.size());
  pipeline_push_constants.insert(pipeline_push_constants.end(),
                                 fragment_push_constants.begin(),
                                 fragment_push_constants.end());

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = GetVkDrawMode(config_.draw_mode);
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewport_state = {};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = VK_NULL_HANDLE;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = VK_NULL_HANDLE;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0F;
  rasterizer.cullMode = GetVkCullMode(config_.cull_mode);
  rasterizer.frontFace = GetVkFrontFace(config_.front_face);
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.rasterizationSamples = context_->GetRecommendedMsaaSamples();

  VkPipelineColorBlendAttachmentState color_blend_attachment = {};
  color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
          | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo color_blending = {};
  color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0F;
  color_blending.blendConstants[1] = 0.0F;
  color_blending.blendConstants[2] = 0.0F;
  color_blending.blendConstants[3] = 0.0F;

  VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
  depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil.depthTestEnable = config_.enable_depth_test;
  depth_stencil.depthWriteEnable = VK_TRUE;
  depth_stencil.depthCompareOp = GetVkCompareOp(config_.depth_function);
  depth_stencil.depthBoundsTestEnable = VK_FALSE;
  depth_stencil.minDepthBounds = 0.0F;
  depth_stencil.maxDepthBounds = 1.0F;

  VkPipelineLayoutCreateInfo pipeline_layout_info = {};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 0;
  pipeline_layout_info.pSetLayouts = nullptr;
  pipeline_layout_info.pushConstantRangeCount = pipeline_push_constants.size();
  pipeline_layout_info.pPushConstantRanges = pipeline_push_constants.data();
  CHECK_VKCMD(vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_));

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
  dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  std::vector<VkDynamicState> dynamic_states = {VkDynamicState::VK_DYNAMIC_STATE_VIEWPORT,
                                                VkDynamicState::VK_DYNAMIC_STATE_SCISSOR};
  dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
  dynamic_state_create_info.pDynamicStates = dynamic_states.data();

  const auto &elements = vbl.GetElements();
  auto stride = static_cast<uint32_t>(vbl.GetElementSize());
  size_t offset = 0;
  std::vector<VkVertexInputAttributeDescription> attribute_descriptions{};
  for (auto element: elements) {
    VkVertexInputAttributeDescription description{
        .location = element.binding_index,
        .binding = 0,
        .format = GetVkFormat(element.type, static_cast<uint32_t>(element.count)),
        .offset = static_cast<uint32_t>(offset),
    };
    attribute_descriptions.push_back(description);
    offset += element.count * GetDataTypeSizeInBytes(element.type);
  }

  VkVertexInputBindingDescription vertex_input_binding_description{};
  vertex_input_binding_description.binding = 0;
  vertex_input_binding_description.stride = stride;
  vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding_description;
  vertex_input_info.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attribute_descriptions.size());
  vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pTessellationState = VK_NULL_HANDLE;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pDepthStencilState = &depth_stencil;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.pDynamicState = VK_NULL_HANDLE;
  pipeline_info.layout = pipeline_layout_;
  pipeline_info.renderPass = context_->GetRenderPass();
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_info.pDynamicState = &dynamic_state_create_info;

  CHECK_VKCMD(vkCreateGraphicsPipelines(device_,
                                        VK_NULL_HANDLE,
                                        1,
                                        &pipeline_info,
                                        nullptr,
                                        &pipeline_));
}

void vulkan::VulkanRenderingPipeline::BindPipeline(VkCommandBuffer command_buffer) {
  vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
  VkDeviceSize offsets[] = {0};
  auto buffer = vertex_buffer_->GetBuffer();
  vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer, offsets);
  vkCmdBindIndexBuffer(command_buffer, index_buffer_->GetBuffer(), 0, this->index_type_);
}

vulkan::VulkanRenderingPipeline::~VulkanRenderingPipeline() {
  context_->WaitForGpuIdle();
  vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
  vkDestroyPipeline(device_, pipeline_, nullptr);
}
VkPipelineLayout vulkan::VulkanRenderingPipeline::GetPipelineLayout() const {
  return pipeline_layout_;
}
