add_library(vulkan-wrapper STATIC
        data_type.cpp
        vertex_buffer_layout.cpp
        vulkan_buffer.cpp
        vulkan_rendering_context.cpp
        vulkan_rendering_pipeline.cpp
        vulkan_shader.cpp
        vulkan_utils.cpp
        )

target_link_libraries(vulkan-wrapper
        magic_enum
        spdlog
        spirv-reflect-static
        vulkan
        )
