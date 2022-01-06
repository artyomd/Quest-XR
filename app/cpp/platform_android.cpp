#include "platform.hpp"
#include "platform_data.hpp"

#include <string>

class AndroidPlatform : public Platform {
 public:
  explicit AndroidPlatform(const std::shared_ptr<PlatformData> &data) {
    PFN_xrInitializeLoaderKHR initialize_loader = nullptr;

    if (XR_SUCCEEDED(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                           (PFN_xrVoidFunction *) (&initialize_loader)))) {
      XrLoaderInitInfoAndroidKHR loader_init_info_android;
      loader_init_info_android.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
      loader_init_info_android.next = nullptr;
      loader_init_info_android.applicationVM = data->application_vm;
      loader_init_info_android.applicationContext = data->application_activity;
      initialize_loader(reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR *>(&loader_init_info_android));
    }

    instance_create_info_android_ = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
    instance_create_info_android_.applicationVM = data->application_vm;
    instance_create_info_android_.applicationActivity = data->application_activity;
  }

  [[nodiscard]] std::vector<std::string> GetInstanceExtensions() const override {
    return {XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME};
  }

  [[nodiscard]] XrBaseInStructure *
  GetInstanceCreateExtension() const override { return (XrBaseInStructure *) (&instance_create_info_android_); }

 private:
  XrInstanceCreateInfoAndroidKHR instance_create_info_android_{};
};

std::shared_ptr<Platform>
CreatePlatform(const std::shared_ptr<PlatformData> &data) {
  return std::make_shared<AndroidPlatform>(data);
}
