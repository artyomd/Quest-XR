//
// Created by artyomd on 10/8/20.
//
#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "data_type.hpp"
#include "redering_pipeline_config.hpp"

#include <string>

#define CHECK_VKCMD(x) \
  vulkan::CheckResult(x, __FILE__, __LINE__)

namespace vulkan {

void CheckResult(VkResult result, const std::string &file, uint32_t line);

std::vector<VkExtensionProperties> GetAvailableInstanceExtensions(std::string layer_name);

std::vector<VkLayerProperties> GetAvailableInstanceLayers();

VkBufferUsageFlags GetVkBufferUsage(BufferUsage buffer_usage);

VkMemoryPropertyFlags GetVkMemoryType(MemoryType memory_property);

VkIndexType GetVkType(DataType type);

VkFormat GetVkFormat(DataType type, uint32_t count);

VkPrimitiveTopology GetVkDrawMode(DrawMode draw_mode);

VkCullModeFlags GetVkCullMode(CullMode cull_mode);

VkFrontFace GetVkFrontFace(FrontFace front_face);

VkCompareOp GetVkCompareOp(CompareOp compare_op);

VkShaderStageFlagBits GetVkShaderStageFlag(ShaderType shader_type);
}
