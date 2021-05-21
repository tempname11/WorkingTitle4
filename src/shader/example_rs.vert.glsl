#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 interpolated_normal;
layout(location = 1) out vec3 world_position;

layout(binding = 0) uniform UBO {
  mat4 projection;
  mat4 view;
} ubo;

void main() {
  gl_Position = ubo.projection * ubo.view * vec4(position, 1.0);
  world_position = position;
  interpolated_normal = normal;
}