#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 tangent;
layout(location = 2) in vec3 bitangent;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 world_space_tangent;
layout(location = 1) out vec3 world_space_bitangent;
layout(location = 2) out vec3 world_space_normal;
layout(location = 3) out vec2 out_uv;

layout(push_constant) uniform VertexPushConstants {
  mat4 model;
} constants;

layout(binding = 0) uniform Frame { FrameData data; } frame;

void main() {
  // @Performance: precompute matrices.
  gl_Position = frame.data.projection * frame.data.view * constants.model * vec4(position, 1.0);
  world_space_tangent = (constants.model * vec4(tangent, 0.0)).xyz;
  world_space_bitangent = -(constants.model * vec4(bitangent, 0.0)).xyz;
  // @Think: what went wrong that we needed a "-" ?
  world_space_normal = (constants.model * vec4(normal, 0.0)).xyz;
  out_uv = uv;
}