#include "vulkan_shader.hpp"

#include <magic_enum.hpp>
#include <spdlog/fmt/fmt.h>

vulkan::VulkanShader::VulkanShader(const std::shared_ptr<VulkanRenderingContext> &context,
                                   const std::vector<uint32_t> &code,
                                   std::string entry_point_name)
    : code_(std::move(code)),
      entry_point_name_(std::move(entry_point_name)),
      device_(context->GetDevice()) {
  VkShaderModuleCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = code_.size() * sizeof(uint32_t),
      .pCode = code_.data(),
  };
  if (vkCreateShaderModule(device_, &create_info, nullptr, &shader_module_) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  SpvReflectResult
      result =
      spvReflectCreateShaderModule(code_.size() * sizeof(uint32_t),
                                   code_.data(),
                                   &reflect_shader_module_);
  if (result != SPV_REFLECT_RESULT_SUCCESS) {
    throw std::runtime_error("spir-v reflection failed");
  }
  switch (reflect_shader_module_.shader_stage) {
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
      this->type_ = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
      break;
    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
      this->type_ = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
      break;
    default:throw std::runtime_error("unhandled shader stage");
  }

  uint32_t count = 0;
  result = spvReflectEnumerateEntryPointPushConstantBlocks(&reflect_shader_module_,
                                                           this->entry_point_name_.data(),
                                                           &count,
                                                           nullptr);

  if (result != SPV_REFLECT_RESULT_SUCCESS)[[unlikely]] {
    throw std::runtime_error(fmt::format("spirv reflect failed with error {}\n",
                                         magic_enum::enum_name(result)));
  }

  std::vector<SpvReflectBlockVariable *> blocks(count);
  result = spvReflectEnumerateEntryPointPushConstantBlocks(&reflect_shader_module_,
                                                           this->entry_point_name_.data(),
                                                           &count,
                                                           blocks.data());

  if (result != SPV_REFLECT_RESULT_SUCCESS)[[unlikely]] {
    throw std::runtime_error(fmt::format("spirv reflect failed with error {}\n",
                                         magic_enum::enum_name(result)));
  }

  for (const auto &block: blocks) {
    VkPushConstantRange range{
        .stageFlags = type_,
        .offset = block->offset,
        .size = block->size,
    };
    push_constants_.emplace_back(range);
  }

}

VkPipelineShaderStageCreateInfo vulkan::VulkanShader::GetShaderStageInfo() const {
  return {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = type_,
      .module = shader_module_,
      .pName = this->entry_point_name_.data(),
      .pSpecializationInfo = nullptr,
  };
}

vulkan::VulkanShader::~VulkanShader() {
  spvReflectDestroyShaderModule(&reflect_shader_module_);
  vkDestroyShaderModule(device_, shader_module_, nullptr);
}
const std::vector<VkPushConstantRange> &vulkan::VulkanShader::GetPushConstants() const {
  return push_constants_;
}

