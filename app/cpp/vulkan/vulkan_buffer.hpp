//
// Created by Artyom Dangizyan on 1/1/21.
//

#pragma once

#include <cstddef>
#include "vulkan_rendering_context.hpp"

namespace vulkan {
class VulkanBuffer {
 public:
  VulkanBuffer() = delete;
  VulkanBuffer(const VulkanBuffer &) = delete;
  VulkanBuffer(const std::shared_ptr<VulkanRenderingContext> &context, const size_t &length,
               VkBufferUsageFlags usage,
               VkMemoryPropertyFlags properties);
  void Update(const void *data);
  void CopyFrom(std::shared_ptr<VulkanBuffer> src_buffer,
                size_t size,
                size_t src_offset,
                size_t dst_offset);
  [[nodiscard]] VkBuffer GetBuffer() const;
  [[nodiscard]] size_t GetSizeInBytes() const;
  virtual ~VulkanBuffer();
 protected:
  std::shared_ptr<VulkanRenderingContext> context_;
  VkDevice device_;
  size_t size_in_bytes_;
  VkBuffer buffer_ = nullptr;
  VkDeviceMemory memory_ = nullptr;
 private:
  bool host_visible_;
};
}
