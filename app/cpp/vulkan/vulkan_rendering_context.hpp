#pragma once

#include <vulkan/vulkan.h>

#include "data_type.hpp"

#include <memory>

namespace vulkan {
class VulkanRenderingContext
    : public std::enable_shared_from_this<VulkanRenderingContext> {
 private:
  VkFormat color_attachment_format_ = VK_FORMAT_UNDEFINED;
  VkFormat depth_attachment_format_ = VK_FORMAT_UNDEFINED;

  VkPhysicalDevice physical_device_;
  VkDevice device_;
  VkQueue graphics_queue_;
  VkCommandPool graphics_pool_;
  VkSampleCountFlagBits recommended_msaa_samples_;
  VkRenderPass render_pass_ = VK_NULL_HANDLE;

  VkSampleCountFlagBits GetMaxUsableSampleCount();
 public:
  VulkanRenderingContext(VkPhysicalDevice physical_device,
                         VkDevice device,
                         VkQueue graphics_queue,
                         VkCommandPool graphics_pool,
                         VkFormat color_attachment_format);

  [[nodiscard]] VkDevice GetDevice() const;

  VkFormat GetDepthAttachmentFormat() const;

  void WaitForGpuIdle() const;

  virtual ~VulkanRenderingContext();

  void CreateBuffer(VkDeviceSize size,
                    VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties,
                    VkBuffer *buffer,
                    VkDeviceMemory *buffer_memory);

  void CreateImage(uint32_t width,
                   uint32_t height,
                   VkSampleCountFlagBits num_samples,
                   VkFormat format,
                   VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties,
                   VkImage *image,
                   VkDeviceMemory *image_memory) const;

  void CopyBuffer(VkBuffer src_buffer,
                  VkBuffer dst_buffer,
                  VkDeviceSize size,
                  VkDeviceSize src_offset = 0,
                  VkDeviceSize dst_offset = 0);

  void TransitionImageLayout(VkImage image,
                             VkImageLayout old_layout,
                             VkImageLayout new_layout);

  void CreateImageView(VkImage image,
                       VkFormat format,
                       VkImageAspectFlagBits aspect_mask,
                       VkImageView *image_view);

  VkCommandBuffer BeginSingleTimeCommands(VkCommandPool command_pool);

  void EndSingleTimeCommands(VkQueue queue, VkCommandPool pool, VkCommandBuffer command_buffer);

  [[nodiscard]] uint32_t FindMemoryType(uint32_t type_filter,
                                        VkMemoryPropertyFlags properties) const;

  [[nodiscard]] VkRenderPass GetRenderPass() const;

  VkCommandPool GetGraphicsPool() const;

  VkQueue GetGraphicsQueue() const;

  [[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates,
                                             VkImageTiling tiling,
                                             VkFormatFeatureFlags features) const;

  [[nodiscard]] VkSampleCountFlagBits GetRecommendedMsaaSamples() const;
};
}
