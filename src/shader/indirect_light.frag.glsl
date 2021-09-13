#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

void main() {
  result = vec3(0.5); // @Incomplete
}