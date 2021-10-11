#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/light_model.glsl"
#include "common/probes.glsl"
#include "common/frame.glsl"

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
  uvec2 texel_coord = uvec2(
    (position * 0.5 + 0.5) * frame.data.secondary_gbuffer_texel_size
  ); // truncated, otherwise would need to subtract 0.5 texels.

  // @CopyPaste :L2_ProbeCoord
  // @Performance: could do bit operations here if working with powers of 2.

  // Ray.
  uvec2 ray_subcoord = texel_coord % probe_ray_count_factors;
  uint ray_index = (
    ray_subcoord.x +
    ray_subcoord.y * probe_ray_count_factors.x
  );
  texel_coord /= probe_ray_count_factors;

  // Grid XY.
  uvec2 probe_grid_coord_xy = texel_coord % frame.data.probe.grid_size.xy;
  texel_coord /= frame.data.probe.grid_size.xy;

  // Grid Z.
  uvec2 z_subcoord = texel_coord % frame.data.probe.grid_size_z_factors;
  uvec3 probe_grid_coord = uvec3(
    probe_grid_coord_xy,
    (
      z_subcoord.x +
      z_subcoord.y * frame.data.probe.grid_size_z_factors.x
    )
  );
  texel_coord /= frame.data.probe.grid_size_z_factors;

  // Cascade.
  uvec2 cascade_subcoord = texel_coord;
  uint cascade_level = (
    cascade_subcoord.x +
    cascade_subcoord.y * frame.data.probe.cascade_count_factors.x
  );

  vec3 cascade_world_position_delta = (
    frame.data.probe.grid_world_position_delta_c0 *
    pow(2.0, cascade_level) // bit shift maybe?
  );

  // unless noted otherwise, everything is in world space.
  vec3 probe_origin = (
    frame.data.probe.cascades[cascade_level].world_position_zero +
    cascade_world_position_delta * probe_grid_coord
  );
  vec3 probe_raydir = get_probe_ray_direction(
    ray_index,
    probe_ray_count,
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
    0.01, // @Cleanup :MoveToUniform ray_t_min
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
