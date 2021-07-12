#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 view_space_tangent;
layout(location = 1) in vec3 view_space_bitangent;
layout(location = 2) in vec3 view_space_normal;
layout(location = 3) in vec2 uv;
layout(location = 0) out vec4 gchannel0; 
layout(location = 1) out vec4 gchannel1; 
layout(location = 2) out vec4 gchannel2; 

layout(set = 1, binding = 0) uniform sampler2D albedo_image;
layout(set = 1, binding = 1) uniform sampler2D normal_image;
/*
layout(set = 1, binding = 1) uniform Material {
  // material
  vec3 albedo;
  float metallic;
  float roughness;
  float ao;
} material;
*/

void main() {
  vec3 albedo = texture(albedo_image, uv).rgb;
  vec3 normalmap = texture(normal_image, uv).rgb;
  float material_metallic = 0.0;
  float material_roughness = 0.5;
  float material_ao = 1.0;
  vec3 N = normalize(0
	+ view_space_tangent * normalmap.x
	+ view_space_bitangent * normalmap.y
	+ view_space_normal * normalmap.z
  );
  gchannel0 = vec4(N, 0.0);
  gchannel1 = vec4(albedo, 0.0);
  gchannel2 = vec4(material_metallic, material_roughness, material_ao, 0.0);
}