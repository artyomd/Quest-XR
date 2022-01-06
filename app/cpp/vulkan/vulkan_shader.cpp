#include "vulkan_shader.hpp"

vulkan::VulkanShader::VulkanShader(const std::shared_ptr<VulkanRenderingContext> &context,
                                   std::string sipr_v_shader_source,
                                   std::string entry_point_name,
                                   ShaderType type)
    : code_(std::move(sipr_v_shader_source)),
      entry_point_name_(std::move(entry_point_name)),
      type_(type),
      device_(context->GetDevice()) {
  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code_.size();
  create_info.pCode = reinterpret_cast<const uint32_t *>(code_.data());

  if (vkCreateShaderModule(device_, &create_info, nullptr, &shader_module_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  shader_stage_info_.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shader_stage_info_.stage = GetVkShaderStageFlag(type);
  shader_stage_info_.module = shader_module_;
  shader_stage_info_.pName = this->entry_point_name_.data();
  shader_stage_info_.pSpecializationInfo = nullptr;

  SpvReflectResult
      result = spvReflectCreateShaderModule(code_.size(), code_.data(), &reflect_shader_module_);

  CHECK(result == SPV_REFLECT_RESULT_SUCCESS, "spirv reflect failed")

  uint32_t count = 0;
  result = spvReflectEnumerateEntryPointPushConstantBlocks(&reflect_shader_module_,
                                                           this->entry_point_name_.data(),
                                                           &count,
                                                           nullptr);

  CHECK(result == SPV_REFLECT_RESULT_SUCCESS, "spirv reflect failed")

  std::vector<SpvReflectBlockVariable *> blocks(count);
  result = spvReflectEnumerateEntryPointPushConstantBlocks(&reflect_shader_module_,
                                                           this->entry_point_name_.data(),
                                                           &count,
                                                           blocks.data());

  CHECK(result == SPV_REFLECT_RESULT_SUCCESS, "spirv reflect failed")

  for (const auto &block:blocks) {
    VkPushConstantRange range{
        .stageFlags = GetVkShaderStageFlag(type),
        .offset = block->offset,
        .size = block->size,
    };
    push_constants_.emplace_back(range);
  }
}

VkPipelineShaderStageCreateInfo vulkan::VulkanShader::GetShaderStageInfo() const {
  return shader_stage_info_;
}

vulkan::VulkanShader::~VulkanShader() {
  spvReflectDestroyShaderModule(&reflect_shader_module_);
  vkDestroyShaderModule(device_, shader_module_, nullptr);
}
const std::vector<VkPushConstantRange> &vulkan::VulkanShader::GetPushConstants() const {
  return push_constants_;
}

