set(glslc_exe "${CMAKE_ANDROID_NDK}/shader-tools/${CMAKE_ANDROID_NDK_TOOLCHAIN_HOST_TAG}/glslc${TOOL_OS_SUFFIX}")

#add spirv library
#LIBRARY_NAME - string, name of output library target
#DEBUG -  boolean, enable/disable, generate debuggable spir-v shaders
#WERROR - boolean, enable/disable, treat all warnings as errors.
#TARGET_SPV - string, target spv version
#INCLUDE_PATH - string, include path for shaders
#INPUT_GLSL_FILE - list, absolute path to source files
FUNCTION(add_spirv_library)
    cmake_parse_arguments(PARAM "" "LIBRARY_NAME;DEBUG;WERROR;TARGET_SPV;INCLUDE_PATH" "INPUT_GLSL_FILE" ${ARGN})

    set(EXTRA_FLAGS)
    if (${PARAM_DEBUG})
        set(EXTRA_FLAGS "${EXTRA_FLAGS};-O0;-g")
    else ()
        set(EXTRA_FLAGS "${EXTRA_FLAGS};-O")
    endif ()

    if (${PARAM_WERROR})
        set(EXTRA_FLAGS "${EXTRA_FLAGS};-Werror")
    endif ()

    if (NOT ${TARGET_SPV} STREQUAL "")
        set(EXTRA_FLAGS "${EXTRA_FLAGS};--target-spv=${TARGET_SPV}")
    endif ()

    if (NOT ${INCLUDE_PATH} STREQUAL "")
        set(EXTRA_FLAGS "${EXTRA_FLAGS};-I;${TARGET_SPV}")
    endif ()

    set(SPIR_V_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/spv")

    add_custom_target(CREATE_SPIRV_BUILD_DIR
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SPIR_V_DIRECTORY})

    FOREACH (FILE ${PARAM_INPUT_GLSL_FILE})
        cmake_path(GET FILE STEM LAST_ONLY FILE_NAME)
        set(OUTPUT_FILE "${SPIR_V_DIRECTORY}/${FILE_NAME}.spv")

        add_custom_command(OUTPUT ${OUTPUT_FILE}
                COMMAND ${glslc_exe} ${EXTRA_FLAGS} -mfmt=num -o ${OUTPUT_FILE} ${FILE}
                COMMENT "Building GLSL object ${OUTPUT_FILE}"
                DEPENDS ${glslc_exe} CREATE_SPIRV_BUILD_DIR ${FILE}
                )

        list(APPEND OUTPUT_SPV_FILES ${OUTPUT_FILE})
    ENDFOREACH (FILE)

    add_library(${PARAM_LIBRARY_NAME} INTERFACE ${OUTPUT_SPV_FILES})
    target_include_directories(${PARAM_LIBRARY_NAME} INTERFACE
            ${SPIR_V_DIRECTORY})

ENDFUNCTION(add_spirv_library)

set(GLSL_FILES
        frag.glsl
        vert.glsl)

list(TRANSFORM GLSL_FILES PREPEND "${CMAKE_CURRENT_LIST_DIR}/")

set(SHADERS_DEBUG OFF)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SHADERS_DEBUG ON)
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(SHADERS_DEBUG ON)
endif ()

add_spirv_library(LIBRARY_NAME shaders
        DEBUG ${SHADERS_DEBUG}
        WERROR ${SHADERS_DEBUG}
        INPUT_GLSL_FILE ${GLSL_FILES})
