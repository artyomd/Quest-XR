#include "vulkan_swapchain_context.hpp"

VulkanSwapchainContext::VulkanSwapchainContext(std::shared_ptr<vulkan::VulkanRenderingContext>
                                               vulkan_rendering_context,
                                               uint32_t capacity,
                                               const XrSwapchainCreateInfo &swapchain_create_info
) :
    rendering_context_(vulkan_rendering_context),
    swapchain_image_format_(static_cast<VkFormat>(swapchain_create_info.format)),
    swapchain_extent_({swapchain_create_info.width, swapchain_create_info.height}) {
  swapchain_images_.resize(capacity);
  swapchain_image_views_.resize(capacity);
  swapchain_frame_buffers_.resize(capacity);

  viewport_ = {
      .x = 0.0F,
      .y = static_cast<float>(swapchain_extent_.height),
      .width = static_cast<float>(swapchain_extent_.width),
      .height = -static_cast<float>(swapchain_extent_.height),
      .minDepth = 0.0,
      .maxDepth = 1.0,
  };
  scissor_.extent = {
      .width = static_cast<uint32_t>(swapchain_extent_.width),
      .height = static_cast<uint32_t>(swapchain_extent_.height),
  };

  for (auto &image: swapchain_images_) {
    image.type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR;
  }
}

XrSwapchainImageBaseHeader *VulkanSwapchainContext::GetFirstImagePointer() {
  return reinterpret_cast<XrSwapchainImageBaseHeader *>(&swapchain_images_[0]);
}

void VulkanSwapchainContext::InitSwapchainImageViews() {
  if (inited_) {
    throw std::runtime_error("cannot double init");
  }
  for (size_t i = 0; i < swapchain_images_.size(); i++) {
    rendering_context_->CreateImageView(
        swapchain_images_[i].image,
        swapchain_image_format_,
        VK_IMAGE_ASPECT_COLOR_BIT,
        &swapchain_image_views_[i]);
  }
  CreateColorResources();
  CreateDepthResources();
  CreateFrameBuffers();
  CreateCommandBuffers();
  CreateSyncObjects();

  inited_ = true;
}

void VulkanSwapchainContext::Draw(uint32_t image_index,
                                  std::shared_ptr<vulkan::VulkanRenderingPipeline> pipeline,
                                  uint32_t index_count,
                                  std::vector<glm::mat4> transforms) {
  if (images_in_flight_[current_fame_] != VK_NULL_HANDLE) {
    vkWaitForFences(rendering_context_->GetDevice(),
                    1,
                    &images_in_flight_[current_fame_],
                    VK_TRUE,
                    UINT64_MAX);
  }
  images_in_flight_[current_fame_] = in_flight_fences_[current_fame_];

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(graphics_command_buffers_[current_fame_], &begin_info);

  VkRenderPassBeginInfo render_pass_info = {};
  render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass = rendering_context_->GetRenderPass();
  render_pass_info.framebuffer = swapchain_frame_buffers_[image_index];
  render_pass_info.renderArea.offset = {0, 0};
  render_pass_info.renderArea.extent = swapchain_extent_;

  std::array<VkClearValue, 2> clear_values = {};
  clear_values[0].color = {{0.184313729f, 0.309803933f, 0.309803933f, 1.0f}};
  clear_values[1].depthStencil = {1.0f, 0};
  render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
  render_pass_info.pClearValues = clear_values.data();
  vkCmdBeginRenderPass(graphics_command_buffers_[current_fame_],
                       &render_pass_info,
                       VK_SUBPASS_CONTENTS_INLINE);
////render
  pipeline->BindPipeline(graphics_command_buffers_[current_fame_]);
  vkCmdSetViewport(graphics_command_buffers_[current_fame_], 0, 1, &viewport_);
  vkCmdSetScissor(graphics_command_buffers_[current_fame_], 0, 1, &scissor_);
  for (const auto &transform:transforms) {
    vkCmdPushConstants(graphics_command_buffers_[current_fame_],
                       pipeline->GetPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0,
                       sizeof(transform),
                       &transform);
    vkCmdDrawIndexed(graphics_command_buffers_[current_fame_],
                     static_cast<uint32_t>(index_count),
                     1,
                     0,
                     0,
                     0);
  }
////render
  vkCmdEndRenderPass(graphics_command_buffers_[current_fame_]);
  vkEndCommandBuffer(graphics_command_buffers_[current_fame_]);

  VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 0;
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &graphics_command_buffers_[current_fame_];
  submit_info.signalSemaphoreCount = 0;

  vkResetFences(rendering_context_->GetDevice(), 1, &in_flight_fences_[current_fame_]);
  if (vkQueueSubmit(rendering_context_->GetGraphicsQueue(),
                    1,
                    &submit_info,
                    in_flight_fences_[current_fame_])
      != VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }
  current_fame_ = (current_fame_ + 1) % max_frames_in_flight_;
}

[[nodiscard]] bool VulkanSwapchainContext::IsInited() const {
  return inited_;
}

VulkanSwapchainContext::~VulkanSwapchainContext() {
  for (const auto &fence: in_flight_fences_) {
    vkDestroyFence(rendering_context_->GetDevice(), fence, nullptr);
  }
  vkFreeCommandBuffers(rendering_context_->GetDevice(),
                       rendering_context_->GetGraphicsPool(),
                       graphics_command_buffers_.size(),
                       &graphics_command_buffers_[0]);
  for (const auto &framebuffer: swapchain_frame_buffers_) {
    vkDestroyFramebuffer(rendering_context_->GetDevice(), framebuffer, nullptr);
  }
  if (depth_image_view_ != VK_NULL_HANDLE) {
    vkDestroyImageView(rendering_context_->GetDevice(), depth_image_view_, nullptr);
    vkDestroyImage(rendering_context_->GetDevice(), depth_image_, nullptr);
    vkFreeMemory(rendering_context_->GetDevice(), depth_image_memory_, nullptr);
  }
  if (color_image_view_ != VK_NULL_HANDLE) {
    vkDestroyImageView(rendering_context_->GetDevice(), color_image_view_, nullptr);
    vkDestroyImage(rendering_context_->GetDevice(), color_image_, nullptr);
    vkFreeMemory(rendering_context_->GetDevice(), color_image_memory_, nullptr);
  }
  for (auto image_view: swapchain_image_views_) {
    vkDestroyImageView(rendering_context_->GetDevice(), image_view, nullptr);
  }
}

void VulkanSwapchainContext::CreateColorResources() {
  rendering_context_->CreateImage(swapchain_extent_.width,
                                  swapchain_extent_.height,
                                  rendering_context_->GetRecommendedMsaaSamples(),
                                  swapchain_image_format_,
                                  VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                                      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  &color_image_,
                                  &color_image_memory_);
  rendering_context_->CreateImageView(color_image_,
                                      swapchain_image_format_,
                                      VK_IMAGE_ASPECT_COLOR_BIT,
                                      &color_image_view_);
  rendering_context_->TransitionImageLayout(color_image_,
                                            VK_IMAGE_LAYOUT_UNDEFINED,
                                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void VulkanSwapchainContext::CreateDepthResources() {
  VkFormat depth_format = rendering_context_->GetDepthAttachmentFormat();
  rendering_context_->CreateImage(swapchain_extent_.width, swapchain_extent_.height,
                                  rendering_context_->GetRecommendedMsaaSamples(),
                                  depth_format,
                                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  &depth_image_,
                                  &depth_image_memory_);
  rendering_context_->CreateImageView(depth_image_,
                                      depth_format,
                                      VK_IMAGE_ASPECT_DEPTH_BIT,
                                      &depth_image_view_);
  rendering_context_->TransitionImageLayout(depth_image_,
                                            VK_IMAGE_LAYOUT_UNDEFINED,
                                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void VulkanSwapchainContext::CreateFrameBuffers() {
  for (size_t i = 0; i < swapchain_image_views_.size(); i++) {
    std::array<VkImageView, 3> attachments = {
        color_image_view_,
        depth_image_view_,
        swapchain_image_views_[i]
    };

    VkFramebufferCreateInfo framebuffer_info = {};
    framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass = rendering_context_->GetRenderPass();
    framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebuffer_info.pAttachments = attachments.data();
    framebuffer_info.width = swapchain_extent_.width;
    framebuffer_info.height = swapchain_extent_.height;
    framebuffer_info.layers = 1;
    CHECK_VKCMD(vkCreateFramebuffer(rendering_context_->GetDevice(),
                                    &framebuffer_info,
                                    nullptr,
                                    &swapchain_frame_buffers_[i]));
  }
}

void VulkanSwapchainContext::CreateCommandBuffers() {
  graphics_command_buffers_.resize(max_frames_in_flight_);
  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = rendering_context_->GetGraphicsPool();
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = static_cast<uint32_t>(graphics_command_buffers_.size());
  CHECK_VKCMD(vkAllocateCommandBuffers(rendering_context_->GetDevice(),
                                       &alloc_info,
                                       graphics_command_buffers_.data()))
}

void VulkanSwapchainContext::CreateSyncObjects() {
  in_flight_fences_.resize(max_frames_in_flight_);
  images_in_flight_.resize(swapchain_images_.size(), VK_NULL_HANDLE);
  VkFenceCreateInfo fence_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  for (size_t i = 0; i < max_frames_in_flight_; i++) {
    if (vkCreateFence(rendering_context_->GetDevice(),
                      &fence_info,
                      nullptr,
                      &in_flight_fences_[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to create sync objects for a frame!");
    }
  }
}