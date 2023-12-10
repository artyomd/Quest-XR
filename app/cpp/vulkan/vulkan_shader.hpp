#pragma once

#include <vulkan/vulkan.h>

#include "vulkan_rendering_context.hpp"
#include "vulkan_utils.hpp"
#include <spirv_reflect.h>

#include <string>
#include <utility>
#include <map>
#include <memory>
#include <variant>

namespace vulkan {
class VulkanShader {
 private:
  std::vector<uint32_t> code_{};
  std::string entry_point_name_;
  VkShaderStageFlagBits type_;

  VkDevice device_;
  VkShaderModule shader_module_ = nullptr;
  SpvReflectShaderModule reflect_shader_module_{};
  std::vector<VkPushConstantRange> push_constants_{};
 public:
  VulkanShader(const std::shared_ptr<VulkanRenderingContext> &context,
               const std::vector<uint32_t> &code,
               std::string entry_point_name);

  [[nodiscard]] VkPipelineShaderStageCreateInfo GetShaderStageInfo() const;

  const std::vector<VkPushConstantRange> &GetPushConstants() const;

  virtual ~VulkanShader();
};
}
