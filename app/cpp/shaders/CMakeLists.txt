cmake_minimum_required(VERSION 3.18.1)

FUNCTION(CONVERT_SPV result)
    SET(spvs "")
    FOREACH (glsl ${ARGN})
        get_filename_component(name ${glsl} NAME_WE)
        LIST(APPEND spvs "${CMAKE_CURRENT_LIST_DIR}/spv/${name}.spv")
        LIST(APPEND spvs "${CMAKE_CURRENT_LIST_DIR}/spv/.hex/${name}.spv")
    ENDFOREACH (glsl)
    SET(${result} "${spvs}" PARENT_SCOPE)
ENDFUNCTION(CONVERT_SPV)

set(GLSL_FILES
        glsl460/frag.glsl
        glsl460/vert.glsl)

CONVERT_SPV(SPV_FILES ${GLSL_FILES})

add_custom_command(
        OUTPUT ${SPV_FILES}
        COMMAND ${CMAKE_CURRENT_LIST_DIR}/generate-spv.sh
        DEPENDS ${GLSL_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set(SRC shaders.cpp)

add_library(shaders STATIC ${SRC} ${SPV_FILES})

target_include_directories(shaders PRIVATE ${CMAKE_CURRENT_LIST_DIR}/spv/.hex)
target_include_directories(shaders SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(shaders)
