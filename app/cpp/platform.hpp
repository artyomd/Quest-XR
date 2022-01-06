#pragma once

#include "openxr-include.hpp"

#include <memory>
#include <vector>

class Platform {
 public:
  virtual XrBaseInStructure *GetInstanceCreateExtension() const = 0;

  virtual std::vector<std::string> GetInstanceExtensions() const = 0;

  virtual ~Platform() = default;
};

std::shared_ptr<Platform> CreatePlatform(const std::shared_ptr<struct PlatformData> &data);
