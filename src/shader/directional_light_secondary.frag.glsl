#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/light_model.glsl"
#include "common/probes.glsl"
#include "common/frame.glsl"

const uint ray_count = 64; // :DDGI_N_Rays 64

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;
layout(binding = 4) uniform Frame { FrameData data; } frame;
layout(binding = 5) uniform accelerationStructureEXT accel;

// @Duplicate :UniformDirLight
layout(set = 1, binding = 0) uniform DirectionalLight {
  vec3 direction;
  vec3 intensity; // @Cleanup: should be named illuminance here and everywhere else
} directional_light;

void main() {
  uvec2 texel_coord = uvec2((position * 0.5 + 0.5) * frame.data.probe.secondary_gbuffer_texel_size);
  // truncated, otherwise would need to subtract 0.5.

  // @Performance: could do bit operations here if working with powers of 2.

  // :DDGI_N_Rays 64
  uvec2 tmp_ray_coord = uvec2(mod(texel_coord, 8));
  uint ray_index = tmp_ray_coord.x + tmp_ray_coord.y * 8;
  uvec3 probe_grid_coord = uvec3(
    mod(texel_coord.x / 8, frame.data.probe.grid_size.x),
    texel_coord.y / 8,
    texel_coord.x / 8 / frame.data.probe.grid_size.x
  );

  // unless noted otherwise, everything is in world space.
  vec3 probe_origin = (
    frame.data.probe.grid_world_position_zero +
    frame.data.probe.grid_world_position_delta * probe_grid_coord
  );
  vec3 probe_raydir = get_probe_ray_direction(
    ray_index,
    ray_count,
    frame.data.probe.random_orientation
  );
  float t = subpassLoad(zchannel).r;
  if (t == 0.0) { discard; }
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
  if (t_intersection > 0.0) { discard; }

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
