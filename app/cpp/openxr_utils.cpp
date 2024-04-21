#include "openxr_utils.hpp"

#include <magic_enum.hpp>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <vector>

void CheckResult(XrResult result, const std::string &file, uint32_t line) {
  if (XR_FAILED(result)) [[unlikely]] {
    throw std::runtime_error(fmt::format("call failed with error {} {}:{}\n",
                                         magic_enum::enum_name(result),
                                         file,
                                         line));
  }
}

std::string GetXrVersionString(XrVersion ver) {
  return fmt::format("{}.{}.{}",
                     XR_VERSION_MAJOR(ver),
                     XR_VERSION_MINOR(ver),
                     XR_VERSION_PATCH(ver));
}

void LogLayersAndExtensions() {
  const auto log_extensions = [](const char *layer_name, int indent = 0) {
    uint32_t instance_extension_count;
    CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layer_name,
                                                       0,
                                                       &instance_extension_count,
                                                       nullptr));

    std::vector<XrExtensionProperties> extensions(instance_extension_count);
    for (XrExtensionProperties &extension: extensions) {
      extension.type = XR_TYPE_EXTENSION_PROPERTIES;
    }

    CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layer_name,
                                                       extensions.size(),
                                                       &instance_extension_count,
                                                       extensions.data()));

    const std::string kIndentStr(indent, ' ');
    spdlog::debug("{} Available Extensions: ({})",
                  kIndentStr.c_str(),
                  instance_extension_count);
    for (const XrExtensionProperties &extension: extensions) {
      spdlog::debug("{}  Name={} SpecVersion={}",
                    kIndentStr.c_str(),
                    extension.extensionName,
                    extension.extensionVersion);
    }
  };

  log_extensions(nullptr);

  {
    uint32_t layer_count;
    CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &layer_count, nullptr));

    std::vector<XrApiLayerProperties> layers(layer_count);
    for (XrApiLayerProperties &layer: layers) {
      layer.type = XR_TYPE_API_LAYER_PROPERTIES;
    }

    CHECK_XRCMD(xrEnumerateApiLayerProperties((uint32_t) layers.size(),
                                              &layer_count,
                                              layers.data()));

    spdlog::info("Available Layers: ({})", layer_count);
    for (const XrApiLayerProperties &layer: layers) {
      spdlog::debug("  Name={} SpecVersion={} LayerVersion={} Description={}",
                    layer.layerName,
                    GetXrVersionString(layer.specVersion).c_str(),
                    layer.layerVersion,
                    layer.description);
      log_extensions(layer.layerName, 4);
    }
  }
}

void LogInstanceInfo(XrInstance instance) {
  if (instance == XR_NULL_HANDLE) {
    throw std::runtime_error("instance is xr null handle");
  }

  XrInstanceProperties instance_properties{};
  instance_properties.type = XR_TYPE_INSTANCE_PROPERTIES;
  CHECK_XRCMD(xrGetInstanceProperties(instance, &instance_properties));

  spdlog::info("Instance RuntimeName={} RuntimeVersion={}",
               instance_properties.runtimeName,
               GetXrVersionString(instance_properties.runtimeVersion).c_str());
}

void LogViewConfigurations(XrInstance instance, XrSystemId system_id) {
  if (instance == XR_NULL_HANDLE) {
    throw std::runtime_error("instance is xr null handle");
  }
  if (system_id == XR_NULL_SYSTEM_ID) {
    throw std::runtime_error("system id is xr null system id");
  }

  uint32_t view_config_type_count;
  CHECK_XRCMD(xrEnumerateViewConfigurations(instance,
                                            system_id,
                                            0,
                                            &view_config_type_count,
                                            nullptr));
  std::vector<XrViewConfigurationType> view_config_types(view_config_type_count);
  CHECK_XRCMD(xrEnumerateViewConfigurations(instance,
                                            system_id,
                                            view_config_type_count,
                                            &view_config_type_count,
                                            view_config_types.data()));

  spdlog::info("Available View Configuration Types: ({})", view_config_type_count);
  for (XrViewConfigurationType view_config_type: view_config_types) {
    std::string view_config_type_name = "unknown";
    switch (view_config_type) {
      case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO:view_config_type_name = "PRIMARY_MONO";
        break;
      case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO:view_config_type_name = "PRIMARY_STEREO";
        break;
      case XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO:view_config_type_name = "QUAD_VARJO";
        break;
      case XR_VIEW_CONFIGURATION_TYPE_SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT:
        view_config_type_name = "SECONDARY_MONO_FIRST_PERSON_OBSERVER_MSFT";
        break;
      default:view_config_type_name = "unknown";
    }
    spdlog::debug(" View Configuration Type: {}", view_config_type_name);

    XrViewConfigurationProperties view_config_properties{};
    view_config_properties.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
    CHECK_XRCMD(xrGetViewConfigurationProperties(instance,
                                                 system_id,
                                                 view_config_type,
                                                 &view_config_properties));

    spdlog::debug("  View configuration FovMutable={}",
                  view_config_properties.fovMutable == XR_TRUE ? "True"
                                                               : "False");

    uint32_t view_count;
    CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance,
                                                  system_id,
                                                  view_config_type,
                                                  0,
                                                  &view_count,
                                                  nullptr));
    if (view_count > 0) {
      std::vector<XrViewConfigurationView> views(view_count);
      for (uint32_t i = 0; i < view_count; i++) {
        views[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        views[i].next = nullptr;
      }
      CHECK_XRCMD(
          xrEnumerateViewConfigurationViews(instance,
                                            system_id,
                                            view_config_type,
                                            view_count,
                                            &view_count,
                                            views.data()));

      for (uint32_t i = 0; i < views.size(); i++) {
        const XrViewConfigurationView &view = views[i];

        spdlog::debug("    View [{}]: Recommended Width={} Height={} SampleCount={}",
                      i,
                      view.recommendedImageRectWidth,
                      view.recommendedImageRectHeight,
                      view.recommendedSwapchainSampleCount);
        spdlog::debug("    View [{}]:     Maximum Width={} Height={} SampleCount={}",
                      i,
                      view.maxImageRectWidth,
                      view.maxImageRectHeight,
                      view.maxSwapchainSampleCount);
      }
    } else {
      spdlog::error("Empty view configuration type");
    }

    uint32_t count;
    CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(instance,
                                                 system_id,
                                                 view_config_type,
                                                 0,
                                                 &count,
                                                 nullptr));
    if (count < 0) {
      throw std::runtime_error("must have at least 1 env blend mode");
    }

    spdlog::info("Available Environment Blend Mode count : {}", count);

    std::vector<XrEnvironmentBlendMode> blend_modes(count);
    CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(instance,
                                                 system_id,
                                                 view_config_type,
                                                 count,
                                                 &count,
                                                 blend_modes.data()));

    for (XrEnvironmentBlendMode mode: blend_modes) {
      std::string blend_mode_name = "unknown";
      switch (mode) {
        case XR_ENVIRONMENT_BLEND_MODE_OPAQUE:blend_mode_name = "opaque";
          break;
        case XR_ENVIRONMENT_BLEND_MODE_ADDITIVE:blend_mode_name = "additive";
          break;
        case XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND:blend_mode_name = "alpha blend";
          break;
        default:blend_mode_name = "unknown";
      }
      spdlog::info("Environment Blend Mode: {}", blend_mode_name);
    }
  }
}

void LogReferenceSpaces(XrSession session) {
  uint32_t space_count;
  CHECK_XRCMD(xrEnumerateReferenceSpaces(session, 0, &space_count, nullptr));
  std::vector<XrReferenceSpaceType> spaces(space_count);
  CHECK_XRCMD(xrEnumerateReferenceSpaces(session, space_count, &space_count, spaces.data()));

  spdlog::info("Available reference spaces: {}", space_count);
  for (XrReferenceSpaceType space: spaces) {
    auto reference_space_name = "";
    switch (space) {
      case XR_REFERENCE_SPACE_TYPE_VIEW: reference_space_name = "view";
        break;
      case XR_REFERENCE_SPACE_TYPE_LOCAL:reference_space_name = "local";
        break;
      case XR_REFERENCE_SPACE_TYPE_STAGE: reference_space_name = "stage";
        break;
      case XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT: reference_space_name = "msft";
        break;
      case XR_REFERENCE_SPACE_TYPE_COMBINED_EYE_VARJO: reference_space_name = "eye_varjo";
        break;
      default:reference_space_name = "unknown";
    }
    spdlog::debug(" space: {}", reference_space_name);
  }
}

void LogSystemProperties(XrInstance instance, XrSystemId system_id) {

  XrSystemProperties system_properties{};
  system_properties.type = XR_TYPE_SYSTEM_PROPERTIES;
  CHECK_XRCMD(xrGetSystemProperties(instance, system_id, &system_properties));

  spdlog::info("System Properties: Name={} VendorId={}",
               system_properties.systemName,
               system_properties.vendorId);
  spdlog::info("System Graphics Properties: MaxWidth={} MaxHeight={} MaxLayers={}",
               system_properties.graphicsProperties.maxSwapchainImageWidth,
               system_properties.graphicsProperties.maxSwapchainImageHeight,
               system_properties.graphicsProperties.maxLayerCount);
  spdlog::info(
      "System Tracking Properties: OrientationTracking={} PositionTracking={}",
      system_properties.trackingProperties.orientationTracking == XR_TRUE
      ? "True"
      : "False",
      system_properties.trackingProperties.positionTracking == XR_TRUE ? "True"
                                                                       : "False");
}

void LogActionSourceName(XrSession session, XrAction action, const std::string &action_name) {
  XrBoundSourcesForActionEnumerateInfo get_info{};
  get_info.type = XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO;
  get_info.action = action;
  uint32_t path_count = 0;
  CHECK_XRCMD(xrEnumerateBoundSourcesForAction(session, &get_info, 0, &path_count, nullptr));
  std::vector<XrPath> paths(path_count);
  CHECK_XRCMD(xrEnumerateBoundSourcesForAction(session,
                                               &get_info,
                                               static_cast<uint32_t>(paths.size()),
                                               &path_count,
                                               paths.data()));

  std::string source_name;
  for (uint32_t i = 0; i < path_count; ++i) {
    constexpr XrInputSourceLocalizedNameFlags kAll = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT |
        XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
        XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

    XrInputSourceLocalizedNameGetInfo name_info{};
    name_info.type = XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO;
    name_info.sourcePath = paths[i];
    name_info.whichComponents = kAll;

    uint32_t size = 0;
    CHECK_XRCMD(xrGetInputSourceLocalizedName(session, &name_info, 0, &size, nullptr));
    if (size == 0) {
      continue;
    }
    std::vector<char> grab_source(size);
    CHECK_XRCMD(xrGetInputSourceLocalizedName(session,
                                              &name_info,
                                              static_cast<uint32_t>(grab_source.size()),
                                              &size,
                                              grab_source.data()));
    if (!source_name.empty()) {
      source_name += " and ";
    }
    source_name += "'";
    source_name += std::string(grab_source.data(), size - 1);
    source_name += "'";
  }

  spdlog::info("{} action is bound to {}",
               action_name.c_str(),
               !source_name.empty() ? source_name.c_str() : "nothing");
}