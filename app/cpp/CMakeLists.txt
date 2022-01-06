cmake_minimum_required(VERSION 3.18.1)

add_subdirectory(vulkan)
add_subdirectory(shaders)
add_subdirectory(utils)

set(SRC
        graphics_plugin_vulkan.cpp
        vulkan_swapchain_context.cpp
        openxr_utils.cpp
        openxr_program.cpp
        platform_android.cpp
        main.cpp
        )

add_library(quest-xr SHARED ${SRC})
target_include_directories(quest-xr PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(quest-xr PRIVATE XR_USE_PLATFORM_ANDROID)
target_compile_definitions(quest-xr PRIVATE XR_USE_GRAPHICS_API_VULKAN)
target_compile_definitions(quest-xr PRIVATE VK_USE_PLATFORM_ANDROID_KHR)

add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(quest-xr SYSTEM PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries(
        quest-xr
        vulkan-wrapper
        utils
        glm
        shaders
        openxr
        android
        log
        native_app_glue
        vulkan)
