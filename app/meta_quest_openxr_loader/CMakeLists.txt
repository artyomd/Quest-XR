add_library(meta_quest_openxr_loader SHARED IMPORTED GLOBAL)
if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set_target_properties(meta_quest_openxr_loader PROPERTIES IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/libs/debug/arm64-v8a/libopenxr_loader.so")
elseif (${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo) #android always generates debug symbols and the gradle plugin does the stripping part
    set_target_properties(meta_quest_openxr_loader PROPERTIES IMPORTED_LOCATION "${PROJECT_SOURCE_DIR}/libs/release/arm64-v8a/libopenxr_loader.so")
else ()
    message(FATAL_ERROR "unknown build type")
endif ()
