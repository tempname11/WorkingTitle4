#ifndef _COMMON_LIGHT_MODEL_GLSL_
#define _COMMON_LIGHT_MODEL_GLSL_

#include "constants.glsl"

// need to remember where this came from.
const vec3 F0_dielectric = vec3(0.04);

vec3 F_fresnelSchlick(float cos_theta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(max(0.0, 1.0 - cos_theta), 5.0);
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

float G_Smith(float N_dot_V, float N_dot_L, float roughness) {
  float g1 = G_GGX(N_dot_V, roughness);
  float g2 = G_GGX(N_dot_L, roughness);
  return g1 * g2;
}

vec3 get_luminance_outgoing(
  vec3 albedo,
  vec3 romeao,
  float NdotV,
  float NdotL,
  float HdotV,
  vec3 N,
  vec3 H,
  vec3 luminance_incoming
) {
  float roughness = romeao.r;
  float metallic = romeao.g;
  float ao = romeao.b; // unused?

  // light model
  vec3 F0 = mix(F0_dielectric, albedo, metallic);
  vec3 F = F_fresnelSchlick(HdotV, F0);
  float D = D_GGX(N, H, roughness);
  float G = G_Smith(NdotV, NdotL, roughness);
  vec3 specular = D * G * F / max(0.001, 4.0 * NdotV * NdotL);
  vec3 kD = (1.0 - F) * (1.0 - metallic);

  // @Terminology: is luminance_incoming even correct?
  vec3 luminance_outgoing = (kD * albedo / PI + specular) * luminance_incoming * NdotL;

  return luminance_outgoing;
}

#endif // _COMMON_LIGHT_MODEL_GLSL_
