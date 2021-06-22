#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 interpolated_normal;
layout (location = 1) in vec3 world_position;

layout (location = 0) out vec4 channel0; 
layout (location = 1) out vec4 channel1; 
layout (location = 2) out vec4 channel2; 

layout(binding = 1) uniform UBO {
  vec3 camera_position;

  // light
  vec3 light_position;
  vec3 light_intensity;

  // material
  vec3 albedo;
  float metallic;
  float roughness;
  float ao;
} ubo;

const vec3 F0_dielectric = vec3(0.04);
const float PI = 3.14159265359;

vec3 linear_to_srgb(vec3 linear) {
  return pow(linear, vec3(1.0 / 2.2));
}

vec3 F_fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(max(0.0, 1.0 - cosTheta), 5.0);
}

float D_GGX(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float dot = max(0.0, dot(H, N));
  float a2 = a * a;
  float dot2 = dot * dot;
  float b = 1.0 + dot2 * (a2 - 1.0);
  return a2 / (PI * b * b);
}

float G_GGX(float NdotV, float roughness) {
  float r = roughness + 1.0;
  float k = r * r / 8.0;
  return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float NdotV, float NdotL, float roughness) {
  float g1 = G_GGX(NdotV, roughness);
  float g2 = G_GGX(NdotL, roughness);
  return g1 * g2;
}

void main() {
  // light direction vectors
  vec3 V = normalize(ubo.camera_position - world_position);
  vec3 N = normalize(interpolated_normal);
  vec3 L = normalize(ubo.light_position - world_position);
  vec3 H = normalize(V + L);
  float NdotV = max(0.0, dot(N, V));
  float NdotL = max(0.0, dot(N, L));
  float HdotV = max(0.0, dot(H, V));

  // light model
  vec3 F0 = mix(F0_dielectric, ubo.albedo, ubo.metallic);
  vec3 F = F_fresnelSchlick(HdotV, F0);
  float D = D_GGX(N, H, ubo.roughness);
  float G = G_Smith(NdotV, NdotL, ubo.roughness);
  vec3 specular = D * G * F / max(0.001, 4.0 * NdotV * NdotL);
  vec3 kD = (1.0 - F) * (1.0 - ubo.metallic);

  float light_distance = length(ubo.light_position - world_position);
  float attenuation = 1.0 / (light_distance * light_distance);
  vec3 radiance_incoming = ubo.light_intensity * attenuation * max(0.0, dot(N, L));
  vec3 radiance_outgoing = (kD * ubo.albedo / PI + specular) * radiance_incoming * NdotL;
  vec3 ambient = 0.03 * ubo.albedo * ubo.ao;

  channel0 = vec4(linear_to_srgb(ambient + radiance_outgoing), 1.0);
  channel1 = vec4(0.0);
  channel2 = vec4(0.0);
}