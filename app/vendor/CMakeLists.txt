cmake_minimum_required(VERSION 3.18.1)

add_library(openxr SHARED IMPORTED GLOBAL)
if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set_target_properties(openxr PROPERTIES IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/libs/debug/arm64-v8a/libopenxr_loader.so")
elseif (${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo) #android always generates debug symboles and the gradle plugin does the stripping part
    set_target_properties(openxr PROPERTIES IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/libs/release/arm64-v8a/libopenxr_loader.so")
else ()
    message(FATAL_ERROR "unknown build type")
endif ()

target_include_directories(openxr SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR})
