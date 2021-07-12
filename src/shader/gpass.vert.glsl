#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 tangent;
layout(location = 2) in vec3 bitangent;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec2 uv;

layout(location = 0) out vec3 view_space_tangent;
layout(location = 1) out vec3 view_space_bitangent;
layout(location = 2) out vec3 view_space_normal;
layout(location = 3) out vec2 out_uv;

layout(set = 0, binding = 0) uniform Frame {
  mat4 projection;
  mat4 view;
  mat4 projection_inverse;
  mat4 view_inverse;
} frame;

void main() {
  gl_Position = frame.projection * frame.view * vec4(position, 1.0);
  view_space_tangent = (frame.view * vec4(tangent, 0.0)).xyz;
  view_space_bitangent = (frame.view * vec4(bitangent, 0.0)).xyz;
  view_space_normal = (frame.view * vec4(normal, 0.0)).xyz;
  out_uv = uv;
}