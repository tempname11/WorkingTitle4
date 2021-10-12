#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/sky.glsl"
#include "common/frame.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 tangent;
layout(location = 2) in vec3 bitangent;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 view_space_tangent;
layout(location = 1) out vec3 view_space_bitangent;
layout(location = 2) out vec3 view_space_normal;
layout(location = 3) out vec2 out_uv;

layout(push_constant) uniform VertexPushConstants {
  mat4 model;
} constants;

layout(binding = 0) uniform Frame { FrameData data; } frame;

void main() {
  // @Performance: combined `viewmodel` would obviously be faster
  gl_Position = frame.data.projection * frame.data.view * constants.model * vec4(position, 1.0);
  view_space_tangent = (frame.data.view * constants.model * vec4(tangent, 0.0)).xyz;
  view_space_bitangent = (frame.data.view * constants.model * vec4(bitangent, 0.0)).xyz;
  view_space_normal = (frame.data.view * constants.model * vec4(normal, 0.0)).xyz;
  out_uv = uv;
}