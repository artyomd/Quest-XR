#pragma once

#include "openxr-include.hpp"
#include <glm/glm.hpp>

#include "vulkan/vulkan_rendering_context.hpp"
#include "vulkan/vulkan_rendering_pipeline.hpp"
#include "vulkan/vulkan_utils.hpp"

class VulkanSwapchainContext {
 private:
  std::shared_ptr<vulkan::VulkanRenderingContext> rendering_context_;
  VkFormat swapchain_image_format_;
  VkExtent2D swapchain_extent_;
  std::vector<XrSwapchainImageVulkan2KHR> swapchain_images_{};
  std::vector<VkImageView> swapchain_image_views_{};

  std::vector<VkFramebuffer> swapchain_frame_buffers_{};

  VkImage color_image_ = VK_NULL_HANDLE;
  VkDeviceMemory color_image_memory_ = VK_NULL_HANDLE;
  VkImageView color_image_view_ = VK_NULL_HANDLE;

  VkImage depth_image_ = VK_NULL_HANDLE;
  VkDeviceMemory depth_image_memory_ = VK_NULL_HANDLE;
  VkImageView depth_image_view_ = VK_NULL_HANDLE;

  std::vector<VkCommandBuffer> graphics_command_buffers_{};
  const uint32_t max_frames_in_flight_ = 2;

  std::vector<VkFence> in_flight_fences_;
  std::vector<VkFence> images_in_flight_;

  bool inited_ = false;

  uint32_t current_fame_ = 0;

  VkViewport viewport_ = {0, 0, 0, 0, 0, 1.0};
  VkRect2D scissor_ = {{0, 0}, {0, 0}};

  void CreateColorResources();
  void CreateDepthResources();
  void CreateFrameBuffers();
  void CreateCommandBuffers();
  void CreateSyncObjects();
 public:
  VulkanSwapchainContext() = delete;
  VulkanSwapchainContext(std::shared_ptr<vulkan::VulkanRenderingContext> vulkan_rendering_context,
                         uint32_t capacity,
                         const XrSwapchainCreateInfo &swapchain_create_info);

  XrSwapchainImageBaseHeader *GetFirstImagePointer();

  void InitSwapchainImageViews();

  void Draw(uint32_t image_index,
            std::shared_ptr<vulkan::VulkanRenderingPipeline> pipeline,
            uint32_t index_count,
            std::vector<glm::mat4> transforms);

  [[nodiscard]] bool IsInited() const;

  virtual ~VulkanSwapchainContext();
};