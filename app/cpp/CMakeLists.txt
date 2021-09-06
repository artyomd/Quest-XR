cmake_minimum_required(VERSION 3.18.1)

add_subdirectory(shaders)
add_subdirectory(logger)

set(SRC
        graphicsplugin_factory.cpp
        graphicsplugin_vulkan.cpp
        main.cpp
        openxr_program.cpp
        platformplugin_android.cpp
        platformplugin_factory.cpp
        )

add_library(quest-demo SHARED ${SRC})
target_include_directories(quest-demo PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_compile_definitions(quest-demo PRIVATE XR_USE_PLATFORM_ANDROID)
target_compile_definitions(quest-demo PRIVATE XR_USE_GRAPHICS_API_VULKAN)
target_compile_definitions(quest-demo PRIVATE VK_USE_PLATFORM_ANDROID_KHR)

add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(quest-demo SYSTEM PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

target_link_libraries(
        quest-demo
        logger
        shaders
        openxr
        android
        log
        native_app_glue
        vulkan)
