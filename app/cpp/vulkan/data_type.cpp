#include "data_type.hpp"

#include <stdexcept>

size_t vulkan::GetDataTypeSizeInBytes(DataType type) {
  switch (type) {
    case DataType::BYTE:return 1;
    case DataType::UINT_16:return 2;
    case DataType::UINT_32:
    case DataType::FLOAT:return 4;
    default: throw std::runtime_error("unsupported enum");
  }
}
