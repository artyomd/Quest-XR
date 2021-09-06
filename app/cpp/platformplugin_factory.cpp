// Copyright (c) 2017-2021, The Khronos Group Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "platformplugin.h"

std::shared_ptr<IPlatformPlugin> CreatePlatformPlugin_Android(const std::shared_ptr<Options> &,
                                                              const std::shared_ptr<PlatformData> &);

std::shared_ptr<IPlatformPlugin> CreatePlatformPlugin(const std::shared_ptr<Options> &options,
                                                      const std::shared_ptr<PlatformData> &data) {
  return CreatePlatformPlugin_Android(options, data);
}
