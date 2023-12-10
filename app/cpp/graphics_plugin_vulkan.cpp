#include "graphics_plugin.hpp"

#include "openxr_utils.hpp"

#include "vulkan_swapchain_context.hpp"
#include "vulkan/data_type.hpp"
#include "vulkan/vulkan_rendering_context.hpp"
#include "vulkan/vulkan_rendering_pipeline.hpp"
#include "vulkan/vulkan_utils.hpp"

#include <array>
#include <map>
#include <memory>

#include <spdlog/spdlog.h>

namespace {
const std::vector<float> kCubePositions = {
    -0.5, -0.5, 0.5, 1.0, 0.0, 0.0,
    0.5, -0.5, 0.5, 0.0, 1.0, 0.0,
    0.5, 0.5, 0.5, 0.0, 0.0, 1.0,
    -0.5, 0.5, 0.5, 1.0, 1.0, 1.0,
    -0.5, -0.5, -0.5, 1.0, 0.0, 0.0,
    0.5, -0.5, -0.5, 0.0, 1.0, 0.0,
    0.5, 0.5, -0.5, 0.0, 0.0, 1.0,
    -0.5, 0.5, -0.5, 1.0, 1.0, 1.0
};
const std::vector<unsigned short> kCubeIndices{
    0, 1, 2,
    2, 3, 0,
    1, 5, 6,
    6, 2, 1,
    7, 6, 5,
    5, 4, 7,
    4, 0, 3,
    3, 7, 4,
    4, 5, 1,
    1, 0, 4,
    3, 2, 6,
    6, 7, 3
};

VkResult CreateDebugUtilsMessengerExt(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
    const VkAllocationCallbacks *p_allocator,
    VkDebugUtilsMessengerEXT *p_debug_messenger
) {
  auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
  );
  if (func != nullptr) {
    return func(instance, p_create_info, p_allocator, p_debug_messenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerExt(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks *p_allocator
) {
  auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
      vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")
  );
  if (func != nullptr) {
    func(instance, debug_messenger, p_allocator);
  }
}

class VulkanGraphicsPlugin : public GraphicsPlugin {
  [[nodiscard]] std::vector<std::string> GetOpenXrInstanceExtensions() const override {
    return {XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME};
  }

  void InitializeDevice(XrInstance xr_instance, XrSystemId system_id) override {
    XrGraphicsRequirementsVulkan2KHR graphics_requirements{};
    graphics_requirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR;

    PFN_xrGetVulkanGraphicsRequirements2KHR pfn_get_vulkan_graphics_requirements_khr = nullptr;
    CHECK_XRCMD(xrGetInstanceProcAddr(xr_instance, "xrGetVulkanGraphicsRequirements2KHR",
                                      reinterpret_cast<PFN_xrVoidFunction *>(&pfn_get_vulkan_graphics_requirements_khr)));
    if (pfn_get_vulkan_graphics_requirements_khr == nullptr) {
      throw std::runtime_error("unable to obtain address of xrGetVulkanGraphicsRequirements2KHR");
    }
    CHECK_XRCMD(pfn_get_vulkan_graphics_requirements_khr(xr_instance,
                                                         system_id,
                                                         &graphics_requirements));

    PFN_xrCreateVulkanInstanceKHR pfn_xr_create_vulkan_instance_khr = nullptr;
    CHECK_XRCMD(xrGetInstanceProcAddr(xr_instance, "xrCreateVulkanInstanceKHR",
                                      reinterpret_cast<PFN_xrVoidFunction *>(&pfn_xr_create_vulkan_instance_khr)));
    if (pfn_xr_create_vulkan_instance_khr == nullptr) {
      throw std::runtime_error("unable to obtain address of xrCreateVulkanInstanceKHR");
    }
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName= "quest-xr",
        .applicationVersion = 0u,
        .pEngineName = "No Engine",
        .engineVersion = 0u,
        .apiVersion = VK_MAKE_VERSION(XR_VERSION_MAJOR(graphics_requirements.maxApiVersionSupported),
                                      XR_VERSION_MINOR(graphics_requirements.maxApiVersionSupported),
                                      XR_VERSION_PATCH(graphics_requirements.maxApiVersionSupported)),
    };

    std::vector<const char *> layers{};
    std::vector<const char *> instance_extensions{};

    bool debug_utils_enabled = false;
    bool validation_features_enabled = false;
    void *instance_create_info_next = nullptr;
    std::vector<VkValidationFeatureEnableEXT>
        enabled_features = {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
                            VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
                            VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
                            VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,};
    VkValidationFeaturesEXT validation_features_ext = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .enabledValidationFeatureCount = static_cast<uint32_t>(enabled_features.size()),
        .pEnabledValidationFeatures = enabled_features.data(),
        .disabledValidationFeatureCount = 0,
        .pDisabledValidationFeatures = nullptr,
    };

#if !defined(NDEBUG)
    auto available_layers = vulkan::GetAvailableInstanceLayers();
    for (const auto &kLayer: available_layers) {
      if (strcmp(kLayer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
        layers.emplace_back("VK_LAYER_KHRONOS_validation");
        break;
      }
    }
    if (!layers.empty()) {
      auto available_extensions =
          vulkan::GetAvailableInstanceExtensions("VK_LAYER_KHRONOS_validation");
      for (const auto &kExt: available_extensions) {
        if (strcmp(kExt.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
          instance_extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
          debug_utils_enabled = true;
        } else if (strcmp(kExt.extensionName, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME) == 0) {
          instance_extensions.emplace_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
          validation_features_enabled = true;
        }
      }
    }
    if (validation_features_enabled) {
      instance_create_info_next = &validation_features_ext;
    }
#endif

    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    instance_create_info.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();
    instance_create_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
    instance_create_info.ppEnabledExtensionNames =
        instance_extensions.empty() ? nullptr : instance_extensions.data();

    XrVulkanInstanceCreateInfoKHR vulkan_instance_create_info_khr{};
    vulkan_instance_create_info_khr.type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR;
    vulkan_instance_create_info_khr.systemId = system_id;
    vulkan_instance_create_info_khr.createFlags = 0;
    vulkan_instance_create_info_khr.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkan_instance_create_info_khr.vulkanCreateInfo = &instance_create_info;
    vulkan_instance_create_info_khr.vulkanAllocator = nullptr;

    VkResult instance_create_result = VK_SUCCESS;
    CHECK_XRCMD(pfn_xr_create_vulkan_instance_khr(xr_instance,
                                                  &vulkan_instance_create_info_khr,
                                                  &vulkan_instance_,
                                                  &instance_create_result));
    if (instance_create_result != VK_SUCCESS) {
      throw std::runtime_error("unable to create vulkan instance");
    }

    if (debug_utils_enabled) {
      VkDebugUtilsMessengerCreateInfoEXT create_info = {
          .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
          .pNext = nullptr,
          .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
              | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
          .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
              | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
              | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
          .pfnUserCallback =  [](
              VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
              VkDebugUtilsMessageTypeFlagsEXT flags,
              const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
              void *) -> VKAPI_ATTR VkBool32 VKAPI_CALL {
            auto flag_to_string = [](VkDebugUtilsMessageTypeFlagsEXT flag) {
              std::string flags;
              if (flag & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
                flags += "|GENERAL";
              }
              if (flag & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
                flags += "|VALIDATION";
              }
              if (flag & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
                flags += "|PERFORMANCE";
              }
              flags += "|";
              return flags;
            };
            if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
              spdlog::info("validation layer {}: {}",
                           flag_to_string(flags),
                           p_callback_data->pMessage);
            } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
              spdlog::warn("validation layer {}: {}",
                           flag_to_string(flags),
                           p_callback_data->pMessage);
            } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
              spdlog::error("validation layer {}: {}",
                            flag_to_string(flags),
                            p_callback_data->pMessage);
            } else {
              spdlog::debug("validation layer {}: {}",
                            flag_to_string(flags),
                            p_callback_data->pMessage);
            }
            return VK_FALSE;
          },};
      CHECK_VKCMD(CreateDebugUtilsMessengerExt(vulkan_instance_,
                                               &create_info,
                                               nullptr,
                                               &debug_messenger_));
    }

    PFN_xrGetVulkanGraphicsDevice2KHR pfn_get_vulkan_graphics_device_khr = nullptr;
    CHECK_XRCMD(xrGetInstanceProcAddr(xr_instance, "xrGetVulkanGraphicsDevice2KHR",
                                      reinterpret_cast<PFN_xrVoidFunction *>(&pfn_get_vulkan_graphics_device_khr)));
    if (pfn_get_vulkan_graphics_device_khr == nullptr) {
      throw std::runtime_error("unable to obtain address of xrGetVulkanGraphicsDevice2KHR");
    }
    XrVulkanGraphicsDeviceGetInfoKHR vulkan_graphics_device_get_info_khr{};
    vulkan_graphics_device_get_info_khr.type = XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR;
    vulkan_graphics_device_get_info_khr.systemId = system_id;
    vulkan_graphics_device_get_info_khr.vulkanInstance = vulkan_instance_;
    CHECK_XRCMD(pfn_get_vulkan_graphics_device_khr(xr_instance,
                                                   &vulkan_graphics_device_get_info_khr,
                                                   &physical_device_));

    PFN_xrCreateVulkanDeviceKHR pfn_xr_create_vulkan_device_khr = nullptr;
    CHECK_XRCMD(xrGetInstanceProcAddr(xr_instance, "xrCreateVulkanDeviceKHR",
                                      reinterpret_cast<PFN_xrVoidFunction *>(&pfn_xr_create_vulkan_device_khr)));
    if (pfn_xr_create_vulkan_device_khr == nullptr) {
      throw std::runtime_error("failed to get address of xrCreateVulkanDeviceKHR");
    }

    float queue_priorities = 0;
    VkDeviceQueueCreateInfo queue_info{};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priorities;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_,
                                             &queue_family_count,
                                             &queue_family_properties[0]);
    for (uint32_t i = 0; i < queue_family_count; ++i) {
      if ((queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u) {
        graphics_queue_family_index_ = queue_info.queueFamilyIndex = i;
        break;
      }
    }

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = &queue_info;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = nullptr;
    device_create_info.enabledExtensionCount = 0;
    device_create_info.ppEnabledExtensionNames = nullptr;
    device_create_info.pEnabledFeatures = &features;

    XrVulkanDeviceCreateInfoKHR vulkan_device_create_info_khr{};
    vulkan_device_create_info_khr.type = XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR;
    vulkan_device_create_info_khr.systemId = system_id;
    vulkan_device_create_info_khr.createFlags = 0;
    vulkan_device_create_info_khr.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkan_device_create_info_khr.vulkanPhysicalDevice = physical_device_;
    vulkan_device_create_info_khr.vulkanCreateInfo = &device_create_info;
    vulkan_device_create_info_khr.vulkanAllocator = nullptr;

    VkResult vulkan_device_create_result = VK_SUCCESS;
    CHECK_XRCMD(pfn_xr_create_vulkan_device_khr(xr_instance,
                                                &vulkan_device_create_info_khr,
                                                &logical_device_,
                                                &vulkan_device_create_result));
    if (vulkan_device_create_result != VK_SUCCESS) {
      throw std::runtime_error("unable to create vulkan logical device");
    }
    vkGetDeviceQueue(logical_device_, queue_info.queueFamilyIndex, 0, &graphic_queue_);

    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = graphics_queue_family_index_;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    CHECK_VKCMD(vkCreateCommandPool(logical_device_, &pool_info, nullptr, &graphics_command_pool_));

    graphics_binding_.type = XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR;
    graphics_binding_.instance = vulkan_instance_;
    graphics_binding_.physicalDevice = physical_device_;
    graphics_binding_.device = logical_device_;
    graphics_binding_.queueFamilyIndex = queue_info.queueFamilyIndex;
    graphics_binding_.queueIndex = 0;
  }

  void InitializeResources() {

    const std::vector<uint32_t> kVertexShader = {
#include "vert.spv"
    };
    const std::vector<uint32_t> kFragmentShader = {
#include "frag.spv"
    };

    auto vertex_shader = std::make_shared<vulkan::VulkanShader>(rendering_context_,
                                                                kVertexShader,
                                                                "main");
    auto fragment_shader = std::make_shared<vulkan::VulkanShader>(rendering_context_,
                                                                  kFragmentShader,
                                                                  "main");

    vulkan::VertexBufferLayout vertex_buffer_layout = vulkan::VertexBufferLayout();
    vertex_buffer_layout.Push({0, vulkan::DataType::FLOAT, 3});
    vertex_buffer_layout.Push({1, vulkan::DataType::FLOAT, 3});

    auto pipeline_config = vulkan::RenderingPipelineConfig{
        .draw_mode = vulkan::DrawMode::TRIANGLE_LIST,
        .cull_mode = vulkan::CullMode::BACK,
        .front_face = vulkan::FrontFace::CCW,
        .enable_depth_test = true,
        .depth_function = vulkan::CompareOp::LESS,
    };
    pipeline_ = std::make_shared<vulkan::VulkanRenderingPipeline>(
        rendering_context_,
        vertex_shader,
        fragment_shader,
        vertex_buffer_layout,
        pipeline_config
    );
    auto vertex_buffer = std::make_shared<vulkan::VulkanBuffer>(
        rendering_context_,
        sizeof(float) * kCubePositions.size(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vertex_buffer->Update(kCubePositions.data());
    pipeline_->SetVertexBuffer(vertex_buffer);

    auto index_buffer = std::make_shared<vulkan::VulkanBuffer>(
        rendering_context_,
        sizeof(unsigned short) * kCubeIndices.size(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    index_buffer->Update(kCubeIndices.data());
    pipeline_->SetIndexBuffer(index_buffer, vulkan::DataType::UINT_16);
  }

  [[nodiscard]] int64_t SelectSwapchainFormat(const std::vector<int64_t> &runtime_formats) override {
    if (rendering_context_ != nullptr) {
      throw std::runtime_error("select swapchain format must be called only once");
    }
    constexpr VkFormat kPreferredSwapchainFormats[] = { //4 channel formats
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_B8G8R8A8_UNORM};

    auto swapchain_format_it = std::find_first_of(runtime_formats.begin(), runtime_formats.end(),
                                                  std::begin(kPreferredSwapchainFormats),
                                                  std::end(kPreferredSwapchainFormats));
    if (swapchain_format_it == runtime_formats.end()) {
      throw std::runtime_error("No runtime swapchain format supported for swapchain");
    }
    rendering_context_ = std::make_shared<vulkan::VulkanRenderingContext>(
        physical_device_,
        logical_device_,
        graphic_queue_,
        graphics_command_pool_,
        (VkFormat) (*swapchain_format_it));
    InitializeResources();
    return *swapchain_format_it;
  }

  [[nodiscard]] const XrBaseInStructure *GetGraphicsBinding() const override {
    return reinterpret_cast <const XrBaseInStructure *>(&graphics_binding_);
  }

  XrSwapchainImageBaseHeader *AllocateSwapchainImageStructs(uint32_t capacity,
                                                            const XrSwapchainCreateInfo &swapchain_create_info) override {
    auto swapchain_context = std::make_shared<VulkanSwapchainContext>(rendering_context_,
                                                                      capacity,
                                                                      swapchain_create_info);
    auto images = swapchain_context->GetFirstImagePointer();
    image_to_context_mapping_.insert(std::make_pair(images, swapchain_context));
    return images;
  }

  void SwapchainImageStructsReady(XrSwapchainImageBaseHeader *images) override {
    auto context = this->image_to_context_mapping_[images];
    if (context->IsInited()) {
      throw std::runtime_error("trying to init same image twice");
    }
    context->InitSwapchainImageViews();
  }
  void RenderView(const XrCompositionLayerProjectionView &layer_view,
                  XrSwapchainImageBaseHeader *swapchain_images,
                  const uint32_t image_index,
                  const std::vector<math::Transform> &cube_transforms) override {
    if (layer_view.subImage.imageArrayIndex != 0) {
      throw std::runtime_error("Texture arrays not supported");
    }
    glm::mat4 proj = math::CreateProjectionFov(layer_view.fov, 0.05f, 100.0f);
    glm::mat4 view = math::InvertRigidBody(
        glm::translate(glm::identity<glm::mat4>(), math::XrVector3FToGlm(layer_view.pose.position))
            * glm::mat4_cast(math::XrQuaternionFToGlm(layer_view.pose.orientation))
    );
    std::vector<glm::mat4> transforms{};
    for (const math::Transform &cube: cube_transforms) {
      glm::mat4 model = glm::scale(glm::translate(glm::identity<glm::mat4>(), cube.position)
                                       * glm::mat4_cast(cube.orientation), cube.scale);
      transforms.emplace_back(proj * view * model);
    }
    auto swapchain_context = image_to_context_mapping_[swapchain_images];

    swapchain_context->Draw(image_index,
                            pipeline_,
                            kCubeIndices.size(),
                            transforms);
  }

  void DeinitDevice() override {
    image_to_context_mapping_.clear();
    pipeline_ = nullptr;
    rendering_context_ = nullptr;
    vkDestroyCommandPool(logical_device_, graphics_command_pool_, nullptr);
    vkDestroyDevice(logical_device_, nullptr);
    if (debug_messenger_ != VK_NULL_HANDLE) {
      DestroyDebugUtilsMessengerExt(vulkan_instance_, debug_messenger_, nullptr);
    }
    vkDestroyInstance(vulkan_instance_, nullptr);
  }

 private:
  XrGraphicsBindingVulkan2KHR graphics_binding_{};

  VkInstance vulkan_instance_ = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;

  std::shared_ptr<vulkan::VulkanRenderingContext> rendering_context_ = nullptr;
  std::shared_ptr<vulkan::VulkanRenderingPipeline> pipeline_ = nullptr;

  VkDevice logical_device_ = VK_NULL_HANDLE;
  uint32_t graphics_queue_family_index_ = 0;
  VkQueue graphic_queue_ = VK_NULL_HANDLE;
  VkCommandPool graphics_command_pool_ = VK_NULL_HANDLE;

  std::map<XrSwapchainImageBaseHeader *, std::shared_ptr<VulkanSwapchainContext>>
      image_to_context_mapping_{};
};
}  // namespace

std::shared_ptr<GraphicsPlugin> CreateGraphicsPlugin() {
  return std::make_shared<VulkanGraphicsPlugin>();
}
