#version 460
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 v_color;

layout(location = 0) out vec4 color;

void main(){
    color = v_color;
}