cmake_minimum_required(VERSION 3.18.1)

set(SRC
        data_type.cpp
        vertex_buffer_layout.cpp
        vulkan_buffer.cpp
        vulkan_rendering_context.cpp
        vulkan_rendering_pipeline.cpp
        vulkan_shader.cpp
        vulkan_utils.cpp
        )

add_library(vulkan-wrapper STATIC ${SRC})

target_link_libraries(vulkan-wrapper spirv-reflect utils vulkan utils fmt)
