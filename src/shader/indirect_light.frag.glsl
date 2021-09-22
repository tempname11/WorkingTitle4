#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(set = 0, binding = 0) uniform sampler2D probe_light_map;

void main() {
  // @Temporary
  result = texture(
    probe_light_map,
    (0.5 + 0.5 * position) * vec2(1280.0, 720.0) / 2048.0 / 4.0 
  ).rgb;
}