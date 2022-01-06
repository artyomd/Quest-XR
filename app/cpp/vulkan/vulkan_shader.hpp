#pragma once

#include <vulkan/vulkan.h>
#include <SPIRV-Reflect/spirv_reflect.h>

#include "vulkan_rendering_context.hpp"
#include "vulkan_utils.hpp"

#include <string>
#include <utility>
#include <map>
#include <memory>
#include <variant>

namespace vulkan {
class VulkanShader {
 private:
  const std::string code_;
  const std::string entry_point_name_;
  ShaderType type_;

  VkDevice device_;
  VkShaderModule shader_module_ = nullptr;
  SpvReflectShaderModule reflect_shader_module_{};
  VkPipelineShaderStageCreateInfo shader_stage_info_{};

  std::vector<VkPushConstantRange> push_constants_{};
 public:
  VulkanShader(const std::shared_ptr<VulkanRenderingContext> &context,
               std::string sipr_v_shader_location,
               std::string entry_point_name,
               ShaderType type
  );

  [[nodiscard]] VkPipelineShaderStageCreateInfo GetShaderStageInfo() const;

  const std::vector<VkPushConstantRange> &GetPushConstants() const;

  virtual ~VulkanShader();
};
}
