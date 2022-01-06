#pragma once

#include "openxr-include.hpp"
#include "math_utils.h"

#include <vector>
#include <string>

class GraphicsPlugin {
 public:
  virtual std::vector<std::string> GetOpenXrInstanceExtensions() const = 0;
  virtual void InitializeDevice(XrInstance instance, XrSystemId system_id) = 0;
  virtual const XrBaseInStructure *GetGraphicsBinding() const = 0;
  virtual int64_t SelectSwapchainFormat(const std::vector<int64_t> &runtime_formats) = 0;

  virtual XrSwapchainImageBaseHeader *AllocateSwapchainImageStructs(uint32_t capacity,
                                                                    const XrSwapchainCreateInfo &swapchain_create_info) = 0;

  virtual void SwapchainImageStructsReady(XrSwapchainImageBaseHeader *images) = 0;

  virtual void RenderView(const XrCompositionLayerProjectionView &layer_view,
                          XrSwapchainImageBaseHeader *swapchain_images,
                          const uint32_t image_index,
                          const std::vector<math::Transform> &cube_transforms) = 0;

  virtual void DeinitDevice() = 0;

  virtual ~GraphicsPlugin() = default;
};

std::shared_ptr<GraphicsPlugin> CreateGraphicsPlugin();
