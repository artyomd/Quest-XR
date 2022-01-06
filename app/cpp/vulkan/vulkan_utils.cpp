#include "vulkan_utils.hpp"

#include <stdexcept>

VkFormat vulkan::GetVkFormat(DataType type, uint32_t count) {
  switch (type) {
    case DataType::BYTE:
      switch (count) {
        case 1:return VK_FORMAT_R8_UNORM;
        case 2:return VK_FORMAT_R8G8_UNORM;
        case 3:return VK_FORMAT_R8G8B8_UNORM;
        case 4:return VK_FORMAT_R8G8B8A8_UNORM;
        default:throw std::runtime_error("unsupported count");
      }
    case DataType::UINT_16:
      switch (count) {
        case 1:return VK_FORMAT_R16_UINT;
        case 2:return VK_FORMAT_R16G16_UINT;
        case 3: return VK_FORMAT_R16G16B16_UINT;
        case 4: return VK_FORMAT_R16G16B16A16_UINT;
        default:throw std::runtime_error("unsupported count");
      }
    case DataType::UINT_32:
      switch (count) {
        case 1:return VK_FORMAT_R32_UINT;
        case 2:return VK_FORMAT_R32G32_UINT;
        case 3: return VK_FORMAT_R32G32B32_UINT;
        case 4: return VK_FORMAT_R32G32B32A32_UINT;
        default:throw std::runtime_error("unsupported count");
      }
    case DataType::FLOAT:
      switch (count) {
        case 1:return VK_FORMAT_R32_SFLOAT;
        case 2:return VK_FORMAT_R32G32_SFLOAT;
        case 3: return VK_FORMAT_R32G32B32_SFLOAT;
        case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:throw std::runtime_error("unsupported count");
      }
    default:throw std::runtime_error("unsupported enum");
  }
}

VkIndexType vulkan::GetVkType(DataType type) {
  switch (type) {
    case DataType::UINT_16:return VK_INDEX_TYPE_UINT16;
    case DataType::UINT_32:return VK_INDEX_TYPE_UINT32;
    default:throw std::runtime_error("unsupported enum");
  }
}

VkShaderStageFlagBits vulkan::GetVkShaderStageFlag(ShaderType shader_type) {
  switch (shader_type) {
    case ShaderType::VERTEX:return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderType::FRAGMENT:return VK_SHADER_STAGE_FRAGMENT_BIT;
    default: throw std::runtime_error("invalid shader type");
  }
}

VkPrimitiveTopology vulkan::GetVkDrawMode(DrawMode draw_mode) {
  switch (draw_mode) {
    case DrawMode::POINT_LIST:return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case DrawMode::LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case DrawMode::LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case DrawMode::TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case DrawMode::TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case DrawMode::TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    default: throw std::runtime_error("unsupported draw mode");
  }
}

VkCullModeFlags vulkan::GetVkCullMode(CullMode cull_mode) {
  switch (cull_mode) {
    case CullMode::NONE:return VK_CULL_MODE_NONE;
    case CullMode::FRONT: return VK_CULL_MODE_FRONT_BIT;
    case CullMode::BACK: return VK_CULL_MODE_BACK_BIT;
    case CullMode::FRONT_AND_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
    default: throw std::runtime_error("unsupported cull mode");
  }
}
VkFrontFace vulkan::GetVkFrontFace(FrontFace front_face) {
  switch (front_face) {
    case FrontFace::CW: return VK_FRONT_FACE_CLOCKWISE;
    case FrontFace::CCW: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    default: throw std::runtime_error("unsupported front face");
  }
}

VkCompareOp vulkan::GetVkCompareOp(CompareOp compare_op) {
  switch (compare_op) {
    case CompareOp::NEVER: return VK_COMPARE_OP_NEVER;
    case CompareOp::LESS: return VK_COMPARE_OP_LESS;
    case CompareOp::EQUAL: return VK_COMPARE_OP_EQUAL;
    case CompareOp::LESS_OR_EQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOp::GREATER: return VK_COMPARE_OP_GREATER;
    case CompareOp::NOT_EQUAL: return VK_COMPARE_OP_EQUAL;
    case CompareOp::GREATER_OR_EQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOp::ALWAYS: return VK_COMPARE_OP_ALWAYS;
    default: throw std::runtime_error("unsupported compare op");;
  }
}

VkBufferUsageFlags vulkan::GetVkBufferUsage(BufferUsage buffer_usage) {
  VkBufferUsageFlags vk_flag = 0;
  if (buffer_usage & BufferUsage::TRANSFER_SRC) {
    vk_flag |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }
  if (buffer_usage & BufferUsage::TRANSFER_DST) {
    vk_flag |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (buffer_usage & BufferUsage::INDEX_BUFFER) {
    vk_flag |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  }
  if (buffer_usage & BufferUsage::UNIFORM_BUFFER) {
    vk_flag |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }
  if (buffer_usage & BufferUsage::VERTEX_BUFFER) {
    vk_flag |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  }
  return vk_flag;
}

VkMemoryPropertyFlags vulkan::GetVkMemoryType(MemoryType memory_property) {
  switch (memory_property) {
    case MemoryType::DEVICE_LOCAL:return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    case MemoryType::HOST_VISIBLE:
      return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    default:throw std::runtime_error("unknown memory type");
  }
}
