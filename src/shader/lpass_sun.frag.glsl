#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;

layout(binding = 4) uniform Frame {
  mat4 projection;
  mat4 view;
  mat4 projection_inverse;
  mat4 view_inverse;
} frame;

layout(set = 1, binding = 0) uniform DirectionalLight {
  vec3 direction;
  vec3 intensity;
} directional_light;

const vec3 F0_dielectric = vec3(0.04);
const float PI = 3.14159265359;

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
  /*
  float z_near = 0.1;
  float z_far = 100.0;
  float z = z_near + (z_far - z_near) * subpassLoad(zchannel).r;
  */
  vec4 target = frame.projection_inverse * vec4(position, 1.0, 1.0);

  // light direction vectors
  vec3 V = -normalize(target.rgb);
  vec3 N = subpassLoad(gchannel0).rgb;
  vec3 L = -(frame.view * vec4(directional_light.direction, 0.0)).xyz;
  vec3 H = normalize(V + L);
  float NdotV = max(0.0, dot(N, V));
  float NdotL = max(0.0, dot(N, L));
  float HdotV = max(0.0, dot(H, V));

  vec3 albedo = subpassLoad(gchannel1).rgb;
  float metallic = subpassLoad(gchannel2).r;
  float roughness = subpassLoad(gchannel2).g;
  float ao = subpassLoad(gchannel2).b;

  // light model
  vec3 F0 = mix(F0_dielectric, albedo, metallic);
  vec3 F = F_fresnelSchlick(HdotV, F0);
  float D = D_GGX(N, H, roughness);
  float G = G_Smith(NdotV, NdotL, roughness);
  vec3 specular = D * G * F / max(0.001, 4.0 * NdotV * NdotL);
  vec3 kD = (1.0 - F) * (1.0 - metallic);

  vec3 radiance_incoming = directional_light.intensity * max(0.0, dot(N, L));
  vec3 radiance_outgoing = (kD * albedo / PI + specular) * radiance_incoming * NdotL;
  vec3 ambient = 0.03 * albedo * ao;

  result = ambient + radiance_outgoing;
}