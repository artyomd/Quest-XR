#pragma once

#include "openxr-include.hpp"

#include "logger.hpp"

#define CHECK_XRCMD(cmd) \
  CHECK(!XR_FAILED(cmd), "XrResult [{}] while calling {} in {}:{}", cmd, #cmd, __FILE__, __LINE__)

std::string GetXrVersionString(XrVersion ver);
void LogLayersAndExtensions();
void LogInstanceInfo(XrInstance instance);
void LogViewConfigurations(XrInstance instance, XrSystemId system_id);
void LogReferenceSpaces(XrSession session);
void LogSystemProperties(XrInstance instance, XrSystemId system_id);
void LogActionSourceName(XrSession session, XrAction action, const std::string &action_name);