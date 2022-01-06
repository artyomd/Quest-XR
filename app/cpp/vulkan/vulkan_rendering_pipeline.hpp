#pragma once

#include <map>
#include <vulkan/vulkan.h>

#include "vulkan_buffer.hpp"
#include "vulkan_rendering_context.hpp"
#include "vulkan_shader.hpp"
#include "vertex_buffer_layout.hpp"

namespace vulkan {
class VulkanRenderingPipeline {
 private:
  std::shared_ptr<VulkanRenderingContext> context_;
  VkDevice device_;
  RenderingPipelineConfig config_;

  VkPipeline pipeline_{};
  VkPipelineLayout pipeline_layout_ = nullptr;

  std::shared_ptr<VulkanBuffer> vertex_buffer_ = nullptr;

  std::shared_ptr<VulkanBuffer> index_buffer_ = nullptr;
  VkIndexType index_type_ = VkIndexType::VK_INDEX_TYPE_UINT16;

  std::shared_ptr<VulkanShader> vertex_shader_ = nullptr;
  std::shared_ptr<VulkanShader> fragment_shader_ = nullptr;

  void CreatePipeline(const VertexBufferLayout &vbl);

 public:
  VulkanRenderingPipeline() = delete;
  VulkanRenderingPipeline(const VulkanRenderingPipeline &) = delete;
  VulkanRenderingPipeline(std::shared_ptr<VulkanRenderingContext> context,
                          std::shared_ptr<VulkanShader> vertex_shader,
                          std::shared_ptr<VulkanShader> fragment_shader,
                          const VertexBufferLayout &vbl,
                          RenderingPipelineConfig config);

  void SetIndexBuffer(std::shared_ptr<VulkanBuffer> buffer, DataType element_type);
  void SetVertexBuffer(std::shared_ptr<VulkanBuffer> buffer);
  void BindPipeline(VkCommandBuffer command_buffer);
  VkPipelineLayout GetPipelineLayout() const;
  virtual ~VulkanRenderingPipeline();
};
}
