#pragma once

#include <cstddef>
#include <cstdint>

namespace vulkan {
enum class DataType {
  BYTE,
  UINT_16,//SHORT
  UINT_32,
  FLOAT,
};

typedef enum BufferUsage {
  TRANSFER_SRC = 1,
  TRANSFER_DST = 2,
  UNIFORM_BUFFER = 4,
  INDEX_BUFFER = 8,
  VERTEX_BUFFER = 16,
} BufferUsage;

enum class MemoryType {
  DEVICE_LOCAL,
  HOST_VISIBLE,
};

enum class ShaderType {
  VERTEX,
  FRAGMENT,
  COUNT,
};

size_t GetDataTypeSizeInBytes(DataType type);
}
