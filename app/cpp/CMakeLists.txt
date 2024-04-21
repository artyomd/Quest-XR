add_subdirectory(vulkan)
add_subdirectory(shaders)

add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

add_library(quest-xr SHARED
        graphics_plugin_vulkan.cpp
        main.cpp
        openxr_program.cpp
        openxr_utils.cpp
        platform_android.cpp
        vulkan_swapchain_context.cpp
        )

target_compile_definitions(quest-xr PRIVATE XR_USE_PLATFORM_ANDROID
        XR_USE_GRAPHICS_API_VULKAN
        VK_USE_PLATFORM_ANDROID_KHR)

target_link_libraries(
        quest-xr
        android
        glm
        native_app_glue
        meta_quest_openxr_loader
        OpenXR::headers
        shaders
        spdlog
        vulkan-wrapper
)
