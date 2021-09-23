#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/light_model.glsl"
#include "common/frame.glsl"

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;
layout(binding = 5) uniform accelerationStructureEXT accel;

// @Duplicate :UniformFrame
layout(binding = 4) uniform Frame {
  mat4 projection;
  mat4 view;
  mat4 projection_inverse;
  mat4 view_inverse;
  FrameFlags flags;
  uint end_marker;
} frame;

// @Duplicate :UniformDirLight
layout(set = 1, binding = 0) uniform DirectionalLight {
  vec3 direction;
  vec3 intensity; // @Terminology: should be illuminance
} directional_light;

void main() {
  vec2 coord = (position * 0.5 + 0.5) * vec2(2048.0) - 0.5; // @Cleanup :MoveToUniform
  ivec3 work_group_id = ivec3(
    mod(coord.x, 32),
    mod(coord.y, 32),
    mod(coord.x / 32, 8)
  ); // @Incomplete: probe grid

  // unless noted otherwise, everything is in world space.

  vec3 probe_origin = vec3(0.5) + 1.0 * work_group_id; // @Incomplete: probe grid
  vec3 probe_raydir = vec3(0.0, 0.0, -1.0); // @Incomplete :ManyRays
  float t = subpassLoad(zchannel).r;
  vec3 probe_hit = probe_origin + t * probe_raydir;

  rayQueryEXT ray_query;
  rayQueryInitializeEXT(
    ray_query,
    accel,
    0,
    0xFF,
    probe_hit,
    0.1, // @Cleanup :MoveToUniform ray_t_min
    -directional_light.direction,
    1000.0 // @Cleanup :MoveToUniform ray_t_max
  );
  rayQueryProceedEXT(ray_query);
  float t_intersection = rayQueryGetIntersectionTEXT(ray_query, false);

  if (t_intersection > 0.0) {
    discard;
  }

  vec3 L = -directional_light.direction;
  vec3 V = -probe_raydir;
  vec3 N = subpassLoad(gchannel0).rgb;
  vec3 H = normalize(V + L);

  float NdotV = max(0.0, dot(N, V));
  float NdotL = max(0.0, dot(N, L));
  float HdotV = max(0.0, dot(H, V));

  vec3 albedo = subpassLoad(gchannel1).rgb;
  vec3 romeao = subpassLoad(gchannel2).rgb;

  result = get_luminance_outgoing(
    albedo,
    romeao,
    NdotV,
    NdotL,
    HdotV,
    N,
    H,
    directional_light.intensity
  );
}
