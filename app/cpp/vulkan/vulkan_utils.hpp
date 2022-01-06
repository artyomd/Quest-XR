//
// Created by artyomd on 10/8/20.
//
#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "data_type.hpp"
#include "redering_pipeline_config.hpp"
#include "logger.hpp"

#define VK_SUCCEEDED(result) ((result) >= 0)

#define VK_FAILED(result) ((result) < 0)

#define VK_UNQUALIFIED_SUCCESS(result) ((result) == 0)

#define CHECK_VKCMD(cmd) \
  CHECK(!VK_FAILED(cmd), "VkResult [{}] while calling {} in {}:{}", cmd, #cmd, __FILE__, __LINE__)

namespace vulkan {

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
