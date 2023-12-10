#version 460
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

layout(push_constant, std140) uniform UniformBufferObject {
    mat4 mvp;
};

layout(location = 0) out vec4 v_color;

void main() {
    v_color = color;
    gl_Position = mvp * position;
}
