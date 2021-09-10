#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "sky.glsl"
#include "frame.glsl"

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
  FrameFlags flags;
  uint end_marker;
} frame;
layout(binding = 5) uniform accelerationStructureEXT accel;

layout(set = 1, binding = 0) uniform DirectionalLight {
  vec3 direction;
  vec3 intensity;
} directional_light;

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

void main() {
  #ifndef NDEBUG
    if (frame.end_marker != 0xDeadBeef) {
      return;
    }
  #endif

  float depth = subpassLoad(zchannel).r;
  float z_near = 0.1; // @Temporary: z_near
  float z_far = 10000.0; // @Temporary: z_far
  float z_linear = z_near * z_far / (z_far + depth * (z_near - z_far));

  vec4 target_view_long = frame.projection_inverse * vec4(position, 1.0, 1.0);
  vec3 target_world = normalize((frame.view_inverse * target_view_long).xyz);
  float perspective_correction = length(target_view_long.xyz);
  vec3 V = -normalize(target_view_long.xyz);
  vec3 L = -(frame.view * vec4(directional_light.direction, 0.0)).xyz;

  if (depth == 1.0) {
    if (frame.flags.show_sky) {
      result = sky(target_world, -directional_light.direction);
    } else {
      result = target_world;
    }
    return;
  }

  rayQueryEXT ray_query;
  vec3 eye_world = (frame.view_inverse * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
  rayQueryInitializeEXT(
    ray_query,
    accel,
    0,
    0xFF,
    eye_world + target_world * z_linear * perspective_correction,
    0.1, // @Temporary: ray_t_min
    -directional_light.direction,
    1000.0 // @Temporary: ray_t_max
  );
  bool incomplete = rayQueryProceedEXT(ray_query);
  float t_intersection = rayQueryGetIntersectionTEXT(ray_query, false);

  if (t_intersection > 0.0) {
    result = vec3(0.0, 0.0, 0.0);
    return;
  }

  vec3 N = subpassLoad(gchannel0).rgb;
  vec3 H = normalize(V + L);
  float NdotV = max(0.0, dot(N, V));
  float NdotL = max(0.0, dot(N, L));
  float HdotV = max(0.0, dot(H, V));

  vec3 albedo = subpassLoad(gchannel1).rgb;
  vec3 romeao = subpassLoad(gchannel2).rgb;
  float roughness = romeao.r;
  float metallic = romeao.g;
  float ao = romeao.b; // unused?

  if (frame.flags.show_normals) {
    result = N * 0.5 + vec3(0.5);
    return;
  }

  // light model
  vec3 F0 = mix(F0_dielectric, albedo, metallic);
  vec3 F = F_fresnelSchlick(HdotV, F0);
  float D = D_GGX(N, H, roughness);
  float G = G_Smith(NdotV, NdotL, roughness);
  vec3 specular = D * G * F / max(0.001, 4.0 * NdotV * NdotL);
  vec3 kD = (1.0 - F) * (1.0 - metallic);

  vec3 radiance_incoming = directional_light.intensity * max(0.0, dot(N, L));
  vec3 radiance_outgoing = (kD * albedo / PI + specular) * radiance_incoming * NdotL;

  result = radiance_outgoing;
}