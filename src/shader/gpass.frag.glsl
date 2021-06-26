#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 view_space_normal;

layout(location = 0) out vec4 gchannel0; 
layout(location = 1) out vec4 gchannel1; 
layout(location = 2) out vec4 gchannel2; 

layout(binding = 1) uniform Material {
  // material
  vec3 albedo;
  float metallic;
  float roughness;
  float ao;
} material;

void main() {
  vec3 N = normalize(view_space_normal);
  gchannel0 = vec4(N, 0.0);
  gchannel1 = vec4(material.albedo, 0.0);
  gchannel2 = vec4(material.metallic, material.roughness, material.ao, 0.0);
}