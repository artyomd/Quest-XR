#include "openxr_program.hpp"

#include "platform.hpp"
#include "graphics_plugin.hpp"
#include "openxr_utils.hpp"
#include "logger.hpp"
#include "utils.hpp"

#include <array>
#include <cmath>
#include <vector>

static inline XrVector3f XrVector3f_Zero() {
  XrVector3f r;
  r.x = r.y = r.z = 0.0f;
  return r;
}

static inline XrQuaternionf XrQuaternionf_Identity() {
  XrQuaternionf r;
  r.x = r.y = r.z = 0.0;
  r.w = 1.0f;
  return r;
}

static inline XrPosef XrPosef_Identity() {
  XrPosef r;
  r.orientation = XrQuaternionf_Identity();
  r.position = XrVector3f_Zero();
  return r;
}

namespace Math::Pose {
XrPosef Translation(const XrVector3f &translation) {
  XrPosef t = XrPosef_Identity();
  t.position = translation;
  return t;
}

XrPosef RotateCCWAboutYAxis(float radians, XrVector3f translation) {
  XrPosef t = XrPosef_Identity();
  t.orientation.x = 0.f;
  t.orientation.y = std::sin(radians * 0.5f);
  t.orientation.z = 0.f;
  t.orientation.w = std::cos(radians * 0.5f);
  t.position = translation;
  return t;
}
}  // namespace Math::Pose

inline XrReferenceSpaceCreateInfo GetXrReferenceSpaceCreateInfo(const std::string &reference_space_type_str) {
  XrReferenceSpaceCreateInfo reference_space_create_info{};
  reference_space_create_info.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
  reference_space_create_info.poseInReferenceSpace = XrPosef_Identity();

  if (utils::EqualsIgnoreCase(reference_space_type_str, "View")) {
    reference_space_create_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  } else if (utils::EqualsIgnoreCase(reference_space_type_str, "ViewFront")) {
    // Render head-locked 2m in front of device.
    reference_space_create_info.poseInReferenceSpace = Math::Pose::Translation({0.f, 0.f, -2.f});
    reference_space_create_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  } else if (utils::EqualsIgnoreCase(reference_space_type_str, "Local")) {
    reference_space_create_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  } else if (utils::EqualsIgnoreCase(reference_space_type_str, "Stage")) {
    reference_space_create_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  } else if (utils::EqualsIgnoreCase(reference_space_type_str, "StageLeft")) {
    reference_space_create_info.poseInReferenceSpace =
        Math::Pose::RotateCCWAboutYAxis(0.f, {-2.f, 0.f, -2.f});
    reference_space_create_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  } else if (utils::EqualsIgnoreCase(reference_space_type_str, "StageRight")) {
    reference_space_create_info.poseInReferenceSpace =
        Math::Pose::RotateCCWAboutYAxis(0.f, {2.f, 0.f, -2.f});
    reference_space_create_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  } else if (utils::EqualsIgnoreCase(reference_space_type_str, "StageLeftRotated")) {
    reference_space_create_info.poseInReferenceSpace =
        Math::Pose::RotateCCWAboutYAxis(3.14f / 3.f, {-2.f, 0.5f, -2.f});
    reference_space_create_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  } else if (utils::EqualsIgnoreCase(reference_space_type_str, "StageRightRotated")) {
    reference_space_create_info.poseInReferenceSpace =
        Math::Pose::RotateCCWAboutYAxis(-3.14f / 3.f, {2.f, 0.5f, -2.f});
    reference_space_create_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  } else {
    throw std::invalid_argument(fmt::format("Unknown reference space type '{}'",
                                            reference_space_type_str.c_str()));
  }
  return reference_space_create_info;
}

OpenXrProgram::OpenXrProgram(std::shared_ptr<Platform> platform)
    : platform_(platform), graphics_plugin_(CreateGraphicsPlugin()) {}

void OpenXrProgram::CreateInstance() {
  LogLayersAndExtensions();
  CHECK(instance_ == XR_NULL_HANDLE, "xr instance must not have been inited");

  std::vector<const char *> extensions{};

  const std::vector<std::string> kPlatformExtensions = platform_->GetInstanceExtensions();
  std::transform(kPlatformExtensions.begin(),
                 kPlatformExtensions.end(),
                 std::back_inserter(extensions),
                 [](const std::string &ext) { return ext.c_str(); });

  const std::vector<std::string>
      kGraphicsExtensions = graphics_plugin_->GetOpenXrInstanceExtensions();
  std::transform(kGraphicsExtensions.begin(),
                 kGraphicsExtensions.end(),
                 std::back_inserter(extensions),
                 [](const std::string &ext) { return ext.c_str(); });

  XrInstanceCreateInfo create_info{};
  create_info.type = XR_TYPE_INSTANCE_CREATE_INFO;
  create_info.next = platform_->GetInstanceCreateExtension();
  create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  create_info.enabledExtensionNames = extensions.data();
  create_info.enabledApiLayerCount = 0;
  create_info.enabledApiLayerNames = nullptr;
  strcpy(create_info.applicationInfo.applicationName, "DEMO");
  create_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

  CHECK_XRCMD(xrCreateInstance(&create_info, &instance_));

  LogInstanceInfo(instance_);
}

void OpenXrProgram::InitializeSystem() {
  CHECK(instance_ != XR_NULL_HANDLE, "instance is xr null handle");
  CHECK(system_id_ == XR_NULL_SYSTEM_ID, "system id must be null system id");

  XrSystemGetInfo system_info{};
  system_info.type = XR_TYPE_SYSTEM_GET_INFO;
  system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  CHECK_XRCMD(xrGetSystem(instance_, &system_info, &system_id_));

  CHECK(system_id_ != XR_NULL_SYSTEM_ID, "system id must not be null system id");

  LogViewConfigurations(instance_, system_id_);

  graphics_plugin_->InitializeDevice(instance_, system_id_);
}

void OpenXrProgram::InitializeSession() {
  CHECK(instance_ != XR_NULL_HANDLE, "instance_ can not be xr null handle");
  CHECK(session_ == XR_NULL_HANDLE, "session must xr null handle");
  {
    utils::logger::Log(utils::logger::Level::VERBOSE, "Creating session...");

    XrSessionCreateInfo create_info{};
    create_info.type = XR_TYPE_SESSION_CREATE_INFO;
    create_info.next = graphics_plugin_->GetGraphicsBinding();
    create_info.systemId = system_id_;
    CHECK_XRCMD(xrCreateSession(instance_, &create_info, &session_));
  }
  LogReferenceSpaces(session_);
  InitializeActions();
  CreateVisualizedSpaces();

  {
    XrReferenceSpaceCreateInfo
        reference_space_create_info = GetXrReferenceSpaceCreateInfo("Local");
    CHECK_XRCMD(xrCreateReferenceSpace(session_, &reference_space_create_info, &app_space_));
  }
}

void OpenXrProgram::InitializeActions() {
  {
    XrActionSetCreateInfo action_set_info{};
    action_set_info.type = XR_TYPE_ACTION_SET_CREATE_INFO;
    strncpy(action_set_info.actionSetName, "gameplay", sizeof(action_set_info.actionSetName));
    strncpy(action_set_info.localizedActionSetName,
            "Gameplay",
            sizeof(action_set_info.localizedActionSetName));
    action_set_info.priority = 0;
    CHECK_XRCMD(xrCreateActionSet(instance_, &action_set_info, &input_.action_set));
  }

  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/left",
                             &input_.hand_subaction_path[side::LEFT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/right",
                             &input_.hand_subaction_path[side::RIGHT]));

  {
    XrActionCreateInfo action_info{};
    action_info.type = XR_TYPE_ACTION_CREATE_INFO;
    action_info.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
    strncpy(action_info.actionName, "grab_object", sizeof(action_info.actionName));
    strncpy(action_info.localizedActionName,
            "Grab Object",
            sizeof(action_info.localizedActionName));
    action_info.countSubactionPaths = uint32_t(input_.hand_subaction_path.size());
    action_info.subactionPaths = input_.hand_subaction_path.data();
    CHECK_XRCMD(xrCreateAction(input_.action_set, &action_info, &input_.grab_action));

    action_info.actionType = XR_ACTION_TYPE_POSE_INPUT;
    strncpy(action_info.actionName, "hand_pose", sizeof(action_info.actionName));
    strncpy(action_info.localizedActionName, "Hand Pose", sizeof(action_info.localizedActionName));
    action_info.countSubactionPaths = uint32_t(input_.hand_subaction_path.size());
    action_info.subactionPaths = input_.hand_subaction_path.data();
    CHECK_XRCMD(xrCreateAction(input_.action_set, &action_info, &input_.pose_action));

    action_info.actionType = XR_ACTION_TYPE_VIBRATION_OUTPUT;
    strncpy(action_info.actionName, "vibrate_hand", sizeof(action_info.actionName));
    strncpy(action_info.localizedActionName,
            "Vibrate Hand",
            sizeof(action_info.localizedActionName));
    action_info.countSubactionPaths = uint32_t(input_.hand_subaction_path.size());
    action_info.subactionPaths = input_.hand_subaction_path.data();
    CHECK_XRCMD(xrCreateAction(input_.action_set, &action_info, &input_.vibrate_action));

    action_info.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
    strncpy(action_info.actionName, "quit_session", sizeof(action_info.actionName));
    strncpy(action_info.localizedActionName,
            "Quit Session",
            sizeof(action_info.localizedActionName));
    action_info.countSubactionPaths = 0;
    action_info.subactionPaths = nullptr;
    CHECK_XRCMD(xrCreateAction(input_.action_set, &action_info, &input_.quit_action));
  }

  std::array<XrPath, side::COUNT> select_path{};
  std::array<XrPath, side::COUNT> squeeze_value_path{};
  std::array<XrPath, side::COUNT> squeeze_force_path{};
  std::array<XrPath, side::COUNT> squeeze_click_path{};
  std::array<XrPath, side::COUNT> pose_path{};
  std::array<XrPath, side::COUNT> haptic_path{};
  std::array<XrPath, side::COUNT> menu_click_path{};
  std::array<XrPath, side::COUNT> b_click_path{};
  std::array<XrPath, side::COUNT> trigger_value_path{};
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/left/input/select/click",
                             &select_path[side::LEFT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/right/input/select/click",
                             &select_path[side::RIGHT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/left/input/squeeze/value",
                             &squeeze_value_path[side::LEFT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/right/input/squeeze/value",
                             &squeeze_value_path[side::RIGHT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/left/input/squeeze/force",
                             &squeeze_force_path[side::LEFT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/right/input/squeeze/force",
                             &squeeze_force_path[side::RIGHT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/left/input/squeeze/click",
                             &squeeze_click_path[side::LEFT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/right/input/squeeze/click",
                             &squeeze_click_path[side::RIGHT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/left/input/grip/pose",
                             &pose_path[side::LEFT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/right/input/grip/pose",
                             &pose_path[side::RIGHT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/left/output/haptic",
                             &haptic_path[side::LEFT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/right/output/haptic",
                             &haptic_path[side::RIGHT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/left/input/menu/click",
                             &menu_click_path[side::LEFT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/right/input/menu/click",
                             &menu_click_path[side::RIGHT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/left/input/b/click",
                             &b_click_path[side::LEFT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/right/input/b/click",
                             &b_click_path[side::RIGHT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/left/input/trigger/value",
                             &trigger_value_path[side::LEFT]));
  CHECK_XRCMD(xrStringToPath(instance_,
                             "/user/hand/right/input/trigger/value",
                             &trigger_value_path[side::RIGHT]));
  {
    XrPath oculus_touch_interaction_profile_path;
    CHECK_XRCMD(
        xrStringToPath(instance_,
                       "/interaction_profiles/oculus/touch_controller",
                       &oculus_touch_interaction_profile_path));
    std::vector<XrActionSuggestedBinding>
        bindings{{{input_.grab_action, squeeze_value_path[side::LEFT]},
                  {input_.grab_action, squeeze_value_path[side::RIGHT]},
                  {input_.pose_action, pose_path[side::LEFT]},
                  {input_.pose_action, pose_path[side::RIGHT]},
                  {input_.quit_action, menu_click_path[side::LEFT]},
                  {input_.vibrate_action, haptic_path[side::LEFT]},
                  {input_.vibrate_action, haptic_path[side::RIGHT]}}};
    XrInteractionProfileSuggestedBinding suggested_bindings{};
    suggested_bindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
    suggested_bindings.interactionProfile = oculus_touch_interaction_profile_path;
    suggested_bindings.suggestedBindings = bindings.data();
    suggested_bindings.countSuggestedBindings = (uint32_t) bindings.size();
    CHECK_XRCMD(xrSuggestInteractionProfileBindings(instance_, &suggested_bindings));
  }
  XrActionSpaceCreateInfo action_space_info{};
  action_space_info.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
  action_space_info.action = input_.pose_action;
  action_space_info.poseInActionSpace.orientation.w = 1.f;
  action_space_info.subactionPath = input_.hand_subaction_path[side::LEFT];
  CHECK_XRCMD(xrCreateActionSpace(session_, &action_space_info, &input_.hand_space[side::LEFT]));
  action_space_info.subactionPath = input_.hand_subaction_path[side::RIGHT];
  CHECK_XRCMD(xrCreateActionSpace(session_, &action_space_info, &input_.hand_space[side::RIGHT]));

  XrSessionActionSetsAttachInfo attach_info{};
  attach_info.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
  attach_info.countActionSets = 1;
  attach_info.actionSets = &input_.action_set;
  CHECK_XRCMD(xrAttachSessionActionSets(session_, &attach_info));
}

void OpenXrProgram::CreateVisualizedSpaces() {
  CHECK(session_ != XR_NULL_HANDLE, "session can be xr null handle");

  std::string visualized_spaces[] =
      {"ViewFront", "Local", "Stage", "StageLeft", "StageRight", "StageLeftRotated",
       "StageRightRotated"};

  for (const auto &visualized_space : visualized_spaces) {
    XrReferenceSpaceCreateInfo
        reference_space_create_info = GetXrReferenceSpaceCreateInfo(visualized_space);
    XrSpace space{};
    XrResult res = xrCreateReferenceSpace(session_, &reference_space_create_info, &space);
    if (XR_SUCCEEDED(res)) {
      visualized_spaces_.push_back(space);
    } else {
      utils::logger::Log(utils::logger::Level::WARNING,
                         fmt::format("Failed to create reference space {} with error {}",
                                     visualized_space,
                                     res));
    }
  }
}

void OpenXrProgram::CreateSwapchains() {
  CHECK(session_ != XR_NULL_HANDLE, "session is null")
  CHECK(swapchains_.empty(), "swapchains must be empty")
  CHECK(config_views_.empty(), "config views must be empty")

  LogSystemProperties(instance_, system_id_);

  uint32_t swapchain_format_count = 0;
  CHECK_XRCMD(xrEnumerateSwapchainFormats(session_, 0, &swapchain_format_count, nullptr));
  std::vector<int64_t> swapchain_formats(swapchain_format_count);
  CHECK_XRCMD(xrEnumerateSwapchainFormats(session_,
                                          static_cast<uint32_t>(swapchain_formats.size()),
                                          &swapchain_format_count,
                                          swapchain_formats.data()));
  uint32_t swapchain_color_format = graphics_plugin_->SelectSwapchainFormat(swapchain_formats);

  CHECK(view_config_type_ == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, "only stereo is supported")

  uint32_t view_count = 0;
  CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance_,
                                                system_id_,
                                                view_config_type_,
                                                0,
                                                &view_count,
                                                nullptr));
  config_views_.resize(view_count, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
  CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance_,
                                                system_id_,
                                                view_config_type_,
                                                view_count,
                                                &view_count,
                                                config_views_.data()));

  views_.resize(view_count, {XR_TYPE_VIEW});
  for (const auto &view_config_view:config_views_) {
    utils::logger::Log(utils::logger::Level::INFO,
                       fmt::format(
                           "Creating swapchain with dimensions Width={} Height={} SampleCount={}",
                           view_config_view.recommendedImageRectWidth,
                           view_config_view.recommendedImageRectHeight,
                           view_config_view.recommendedSwapchainSampleCount));

    XrSwapchainCreateInfo swapchain_create_info{};
    swapchain_create_info.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
    swapchain_create_info.arraySize = 1;
    swapchain_create_info.format = swapchain_color_format;
    swapchain_create_info.width = view_config_view.recommendedImageRectWidth;
    swapchain_create_info.height = view_config_view.recommendedImageRectHeight;
    swapchain_create_info.mipCount = 1;
    swapchain_create_info.faceCount = 1;
    swapchain_create_info.sampleCount = view_config_view.recommendedSwapchainSampleCount;
    swapchain_create_info.usageFlags =
        XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
    Swapchain swapchain{};
    swapchain.width = swapchain_create_info.width;
    swapchain.height = swapchain_create_info.height;
    CHECK_XRCMD(xrCreateSwapchain(session_, &swapchain_create_info, &swapchain.handle));

    swapchains_.push_back(swapchain);

    uint32_t image_count;
    CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle, 0, &image_count, nullptr));

    XrSwapchainImageBaseHeader *swapchain_images =
        graphics_plugin_->AllocateSwapchainImageStructs(image_count, swapchain_create_info);
    CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain.handle,
                                           image_count,
                                           &image_count,
                                           swapchain_images));
    graphics_plugin_->SwapchainImageStructsReady(swapchain_images);
    swapchain_images_.insert(std::make_pair(swapchain.handle, swapchain_images));
  }
}

void OpenXrProgram::PollEvents() {
  while (const XrEventDataBaseHeader *event = TryReadNextEvent()) {
    switch (event->type) {
      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
        const auto &instance_loss_pending =
            *reinterpret_cast<const XrEventDataInstanceLossPending *>(&event);
        utils::logger::Log(utils::logger::Level::WARNING,
                           fmt::format("XrEventDataInstanceLossPending by {}",
                                       instance_loss_pending.lossTime));
        return;
      }
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
        auto session_state_changed_event =
            *reinterpret_cast<const XrEventDataSessionStateChanged *>(event);
        HandleSessionStateChangedEvent(session_state_changed_event);
        break;
      }
      case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
        LogActionSourceName(session_, input_.grab_action, "Grab");
        LogActionSourceName(session_, input_.quit_action, "Quit");
        LogActionSourceName(session_, input_.pose_action, "Pose");
        LogActionSourceName(session_, input_.vibrate_action, "Vibrate");
      }
        break;
      case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
      default: {
        utils::logger::Log(utils::logger::Level::VERBOSE,
                           fmt::format("Ignoring event type {}", event->type));
        break;
      }
    }
  }
}

const XrEventDataBaseHeader *OpenXrProgram::TryReadNextEvent() {
  auto base_header = reinterpret_cast<XrEventDataBaseHeader *>(&event_data_buffer_);
  base_header->type = XR_TYPE_EVENT_DATA_BUFFER;
  XrResult result = xrPollEvent(instance_, &event_data_buffer_);
  if (result == XR_SUCCESS) {
    if (base_header->type == XR_TYPE_EVENT_DATA_EVENTS_LOST) {
      auto events_lost = reinterpret_cast<XrEventDataEventsLost *>(base_header);
      utils::logger::Log(utils::logger::Level::WARNING,
                         fmt::format("{} events lost", events_lost->lostEventCount));
    }
    return base_header;
  }
  if (result == XR_EVENT_UNAVAILABLE) {
    return nullptr;
  }
  LOG_FATAL("xr pull event unknown result: {}", result);
}

void OpenXrProgram::HandleSessionStateChangedEvent(const XrEventDataSessionStateChanged &state_changed_event) {

  utils::logger::Log(utils::logger::Level::INFO,
                     fmt::format(
                         "XrEventDataSessionStateChanged: state {}->{} time={}",
                         session_state_,
                         state_changed_event.state,
                         state_changed_event.time));

  if ((state_changed_event.session != XR_NULL_HANDLE)
      && (state_changed_event.session != session_)) {
    LOG_FATAL("XrEventDataSessionStateChanged for unknown session");
    return;
  }
  session_state_ = state_changed_event.state;
  switch (session_state_) {
    case XR_SESSION_STATE_READY: {
      CHECK(session_ != XR_NULL_HANDLE, "session can not be null");
      XrSessionBeginInfo session_begin_info{};
      session_begin_info.type = XR_TYPE_SESSION_BEGIN_INFO;
      session_begin_info.primaryViewConfigurationType = view_config_type_;
      CHECK_XRCMD(xrBeginSession(session_, &session_begin_info));
      session_running_ = true;
      break;
    }
    case XR_SESSION_STATE_STOPPING: {
      CHECK(session_ != XR_NULL_HANDLE, "session can not be null");
      session_running_ = false;
      CHECK_XRCMD(xrEndSession(session_));
      break;
    }
    default:break;
  }
}

bool OpenXrProgram::IsSessionRunning() const {
  return session_running_;
}

void OpenXrProgram::PollActions() {
  input_.hand_active = {XR_FALSE, XR_FALSE};

  const XrActiveActionSet kActiveActionSet{input_.action_set, XR_NULL_PATH};
  XrActionsSyncInfo sync_info{};
  sync_info.type = XR_TYPE_ACTIONS_SYNC_INFO;
  sync_info.countActiveActionSets = 1;
  sync_info.activeActionSets = &kActiveActionSet;
  CHECK_XRCMD(xrSyncActions(session_, &sync_info));

  for (auto hand: {side::LEFT, side::RIGHT}) {
    XrActionStateGetInfo get_info{};
    get_info.type = XR_TYPE_ACTION_STATE_GET_INFO;
    get_info.action = input_.grab_action;
    get_info.subactionPath = input_.hand_subaction_path[hand];

    XrActionStateFloat grab_value{};
    grab_value.type = XR_TYPE_ACTION_STATE_FLOAT;
    CHECK_XRCMD(xrGetActionStateFloat(session_, &get_info, &grab_value));
    if (grab_value.isActive == XR_TRUE) {
      input_.hand_scale[hand] = 1.0f - 0.5f * grab_value.currentState;
      if (grab_value.currentState > 0.9f) {
        XrHapticVibration vibration{};
        vibration.type = XR_TYPE_HAPTIC_VIBRATION;
        vibration.amplitude = 0.5;
        vibration.duration = XR_MIN_HAPTIC_DURATION;
        vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

        XrHapticActionInfo haptic_action_info{};
        haptic_action_info.type = XR_TYPE_HAPTIC_ACTION_INFO;
        haptic_action_info.action = input_.vibrate_action;
        haptic_action_info.subactionPath = input_.hand_subaction_path[hand];
        CHECK_XRCMD(xrApplyHapticFeedback(session_, &haptic_action_info,
                                          (XrHapticBaseHeader *) &vibration));
      }
    }

    get_info.action = input_.pose_action;
    XrActionStatePose pose_state{};
    pose_state.type = XR_TYPE_ACTION_STATE_POSE;
    CHECK_XRCMD(xrGetActionStatePose(session_, &get_info, &pose_state));
    input_.hand_active[hand] = pose_state.isActive;
  }

  XrActionStateGetInfo get_info{};
  get_info.type = XR_TYPE_ACTION_STATE_GET_INFO;
  get_info.next = nullptr;
  get_info.action = input_.quit_action;
  get_info.subactionPath = XR_NULL_PATH;
  XrActionStateBoolean quit_value{};
  quit_value.type = XR_TYPE_ACTION_STATE_BOOLEAN;
  CHECK_XRCMD(xrGetActionStateBoolean(session_, &get_info, &quit_value));
  if ((quit_value.isActive == XR_TRUE) && (quit_value.changedSinceLastSync == XR_TRUE)
      && (quit_value.currentState == XR_TRUE)) {
    CHECK_XRCMD(xrRequestExitSession(session_));
  }
}

void OpenXrProgram::RenderFrame() {
  CHECK(session_ != XR_NULL_HANDLE, "session can not be null");

  XrFrameWaitInfo frame_wait_info{
      .type = XR_TYPE_FRAME_WAIT_INFO,
  };
  XrFrameState frame_state{
      .type = XR_TYPE_FRAME_STATE,
  };
  CHECK_XRCMD(xrWaitFrame(session_, &frame_wait_info, &frame_state));

  XrFrameBeginInfo frame_begin_info{
      .type = XR_TYPE_FRAME_BEGIN_INFO,
  };
  CHECK_XRCMD(xrBeginFrame(session_, &frame_begin_info));

  std::vector<XrCompositionLayerBaseHeader *> layers{};
  XrCompositionLayerProjection layer{
      .type = XR_TYPE_COMPOSITION_LAYER_PROJECTION,
  };
  std::vector<XrCompositionLayerProjectionView> projection_layer_views{};
  if (frame_state.shouldRender == XR_TRUE) {
    if (RenderLayer(frame_state.predictedDisplayTime, projection_layer_views, layer)) {
      layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(&layer));
    }
  }

  XrFrameEndInfo frame_end_info{};
  frame_end_info.type = XR_TYPE_FRAME_END_INFO;
  frame_end_info.displayTime = frame_state.predictedDisplayTime;
  frame_end_info.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  frame_end_info.layerCount = static_cast<uint32_t>(layers.size());
  frame_end_info.layers = layers.data();
  CHECK_XRCMD(xrEndFrame(session_, &frame_end_info));
}

bool OpenXrProgram::RenderLayer(XrTime predicted_display_time,
                                std::vector<XrCompositionLayerProjectionView> &projection_layer_views,
                                XrCompositionLayerProjection &layer) {
  XrViewState view_state{};
  view_state.type = XR_TYPE_VIEW_STATE;

  XrViewLocateInfo view_locate_info{};
  view_locate_info.type = XR_TYPE_VIEW_LOCATE_INFO;
  view_locate_info.viewConfigurationType = view_config_type_;
  view_locate_info.displayTime = predicted_display_time;
  view_locate_info.space = app_space_;

  uint32_t view_count_output = 0;
  CHECK_XRCMD(xrLocateViews(session_,
                            &view_locate_info,
                            &view_state,
                            views_.size(),
                            &view_count_output,
                            views_.data()));
  if ((view_state.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) == 0 ||
      (view_state.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) == 0) {
    return false;  // There is no valid tracking poses for the views.
  }

  CHECK(view_count_output == views_.size(), "idk");
  CHECK(view_count_output == config_views_.size(), "idk");
  CHECK(view_count_output == swapchains_.size(), "idk");

  projection_layer_views.resize(view_count_output);

  // For each locatable space that we want to visualize, render a 25cm cube.
  std::vector<math::Transform> cubes{};

  for (XrSpace visualized_space : visualized_spaces_) {
    XrSpaceLocation space_location{};
    space_location.type = XR_TYPE_SPACE_LOCATION;
    auto res = xrLocateSpace(visualized_space, app_space_, predicted_display_time, &space_location);
    if (XR_UNQUALIFIED_SUCCESS(res)) {
      if ((space_location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
          (space_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
        cubes.push_back(math::Transform{
            math::XrQuaternionFToGlm(space_location.pose.orientation),
            math::XrVector3FToGlm(space_location.pose.position),
            {0.25f, 0.25f, 0.25f}});
      }
    } else {
      utils::logger::Log(utils::logger::Level::VERBOSE,
                         fmt::format(
                             "Unable to locate a visualized reference space in app space: {}",
                             res));
    }
  }

  // Render a 10cm cube scaled by grab_action for each hand. Note renderHand will only be true when the application has focus.
  for (auto hand : {side::LEFT, side::RIGHT}) {
    XrSpaceLocation space_location{};
    space_location.type = XR_TYPE_SPACE_LOCATION;
    auto res =
        xrLocateSpace(input_.hand_space[hand], app_space_, predicted_display_time, &space_location);
    if (XR_UNQUALIFIED_SUCCESS(res)) {
      if ((space_location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
          (space_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
        float scale = 0.1f * input_.hand_scale[hand];
        cubes.push_back(math::Transform{
            math::XrQuaternionFToGlm(space_location.pose.orientation),
            math::XrVector3FToGlm(space_location.pose.position),
            {scale, scale, scale}});
      }
    } else {
      // Tracking loss is expected when the hand is not active so only log a message if the hand is active.
      if (input_.hand_active[hand] == XR_TRUE) {
        const char *hand_name[] = {"left", "right"};
        utils::logger::Log(utils::logger::Level::VERBOSE,
                           fmt::format("Unable to locate {} hand action space in app space: {}",
                                       hand_name[hand],
                                       res));
      }
    }
  }

  // Render view to the appropriate part of the swapchain image.
  for (uint32_t i = 0; i < view_count_output; i++) {
    Swapchain view_swapchain = swapchains_[i];

    XrSwapchainImageAcquireInfo acquire_info{};
    acquire_info.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;

    uint32_t swapchain_image_index = 0;
    CHECK_XRCMD(xrAcquireSwapchainImage(view_swapchain.handle,
                                        &acquire_info,
                                        &swapchain_image_index));

    XrSwapchainImageWaitInfo wait_info{};
    wait_info.type = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
    wait_info.timeout = XR_INFINITE_DURATION;
    CHECK_XRCMD(xrWaitSwapchainImage(view_swapchain.handle, &wait_info));

    projection_layer_views[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
    projection_layer_views[i].pose = views_[i].pose;
    projection_layer_views[i].fov = views_[i].fov;
    projection_layer_views[i].subImage.swapchain = view_swapchain.handle;
    projection_layer_views[i].subImage.imageRect.offset = {0, 0};
    projection_layer_views[i].subImage.imageRect.extent =
        {view_swapchain.width, view_swapchain.height};

    auto swapchain_image = swapchain_images_[view_swapchain.handle];
    graphics_plugin_->RenderView(projection_layer_views[i],
                                 swapchain_image,
                                 swapchain_image_index,
                                 cubes);

    XrSwapchainImageReleaseInfo release_info{};
    release_info.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
    CHECK_XRCMD(xrReleaseSwapchainImage(view_swapchain.handle, &release_info));
  }

  layer.space = app_space_;
  layer.viewCount = static_cast<uint32_t>(projection_layer_views.size());
  layer.views = projection_layer_views.data();
  return true;
}

OpenXrProgram::~OpenXrProgram() {
  if (input_.action_set != XR_NULL_HANDLE) {
    for (auto hand : {side::LEFT, side::RIGHT}) {
      xrDestroySpace(input_.hand_space[hand]);
    }
    xrDestroyActionSet(input_.action_set);
  }
  for (Swapchain swapchain : swapchains_) {
    xrDestroySwapchain(swapchain.handle);
  }
  graphics_plugin_->DeinitDevice();
  for (XrSpace visualized_space : visualized_spaces_) {
    xrDestroySpace(visualized_space);
  }
  if (app_space_ != XR_NULL_HANDLE) {
    xrDestroySpace(app_space_);
  }
  if (session_ != XR_NULL_HANDLE) {
    xrDestroySession(session_);
  }
  if (instance_ != XR_NULL_HANDLE) {
    xrDestroyInstance(instance_);
  }
}

std::shared_ptr<OpenXrProgram> CreateOpenXrProgram(std::shared_ptr<Platform> platform) {
  return std::make_shared<OpenXrProgram>(platform);
}
