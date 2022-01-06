#include "vertex_buffer_layout.hpp"

const std::vector<vulkan::VertexAttribute> &vulkan::VertexBufferLayout::GetElements() const {
  return elements_;
}

void vulkan::VertexBufferLayout::Push(vulkan::VertexAttribute attribute) {
  elements_.emplace_back(attribute);
}

size_t vulkan::VertexBufferLayout::GetElementSize() const {
  size_t size = 0;
  for (auto elem: elements_) {
    size += elem.count * GetDataTypeSizeInBytes(elem.type);
  }
  return size;
}
