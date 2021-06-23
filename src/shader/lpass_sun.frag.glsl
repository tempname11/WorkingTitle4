#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 position;
layout(location = 0) out vec4 output_channel; 

void main() {
  output_channel = vec4(1.0);
}