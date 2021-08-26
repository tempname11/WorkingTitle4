#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 in_position;
layout(location = 0) out vec2 out_position;

void main() {
  gl_Position = vec4(in_position, 0.0, 1.0);
  out_position = in_position;
}