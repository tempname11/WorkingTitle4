#ifndef _COMMON_PROBES_GLSL_
#define _COMMON_PROBES_GLSL_

#include "constants.glsl"
#include "frame.glsl"

const uint probe_ray_count = 64; // GI_N_Rays
const uvec2 probe_ray_count_factors = uvec2(8, 8);
const uvec2 octomap_light_texel_size = uvec2(8, 8); // :ProbeLightOctoSize
const uvec2 octomap_depth_texel_size = uvec2(16, 16); // :ProbeDepthOctoSize

float madfrac(float a, float b) {
  return a * b - floor(a * b);
}

// originally from DDGI sample code.
// see http://jcgt.org/published/0008/02/01/
vec3 spherical_fibonacci(float i, float n) {
  float phi = 2.0 * PI * madfrac(i, PHI - 1.0);
  float cos_theta = 1.0 - (2.0 * i + 1.0) / n;
  float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
  // there was a `saturate` in the original, but I think it's not needed.

  return vec3(
    cos(phi) * sin_theta,
    sin(phi) * sin_theta,
    cos_theta
  );
}

// @Performance: maybe precompute N vectors instead?
vec3 get_probe_ray_direction(float i, float n, mat3 random_orientation) {
  //return -spherical_fibonacci(i, n);
  return random_orientation * spherical_fibonacci(i, n);
}

float sign_not_zero(float k) {
  return k >= 0.0 ? 1.0 : -1.0;
}

vec2 sign_not_zero(vec2 v) {
  return vec2(
    sign_not_zero(v.x),
    sign_not_zero(v.y)
  );
}

// see http://jcgt.org/published/0003/02/01/
vec2 octo_encode(vec3 v) {
  float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
  vec2 result = v.xy / l1norm;
  if (v.z < 0.0) {
    result = (1.0 - abs(result.yx)) * sign_not_zero(result.xy);
  }
  return result;
}

// see http://jcgt.org/published/0003/02/01/
vec3 octo_decode(vec2 o) {
  vec3 v = vec3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));
  if (v.z < 0.0) {
    v.xy = (1.0 - abs(v.yx)) * sign_not_zero(v.xy);
  }
  return normalize(v);
}

vec3 get_indirect_luminance(
  vec3 pos_world,
  vec3 N,
  FrameData frame_data,
  vec3 grid_world_position_zero,
  vec3 grid_world_position_delta,
  sampler2D probe_light_map,
  sampler2D probe_depth_map,
  vec3 albedo
) {
  vec3 grid_coord_float = (
    (pos_world - grid_world_position_zero) /
    grid_world_position_delta
  ); // may be out of bounds

  ivec3 grid_coord0 = ivec3(grid_coord_float);
  vec3 grid_cube_coord = fract(grid_coord_float);
  
  // are vector boolean expressions possible in GLSL? if so, this could use them.
  bool out_of_bounds = (false
    || grid_coord0.x < 0
    || grid_coord0.y < 0
    || grid_coord0.z < 0
    || grid_coord0.x + 1 >= frame_data.probe.grid_size.x
    || grid_coord0.y + 1 >= frame_data.probe.grid_size.y
    || grid_coord0.z + 1 >= frame_data.probe.grid_size.z
  );

  if (out_of_bounds) {
    return vec3(0.0);
  }

  vec4 sum = vec4(0.0);
  for (uint i = 0; i < 8; i++) {
    ivec3 grid_cube_vertex_coord = ivec3(
      (i & 1) == 1 ? 1 : 0,
      (i & 2) == 2 ? 1 : 0,
      (i & 4) == 4 ? 1 : 0
    );

    ivec3 grid_coord = grid_coord0 + grid_cube_vertex_coord; // may be out of bounds

    vec3 trilinear = mix(
      1.0 - grid_cube_coord,
      grid_cube_coord,
      vec3(grid_cube_vertex_coord)
    ); // choose first or second in each component depending on the cube vertex.

    uvec2 current_z_subcoord = uvec2(
      grid_coord.z % frame_data.probe.grid_size_z_factors.x,
      grid_coord.z / frame_data.probe.grid_size_z_factors.x
    );

    uvec2 light_base_texel_coord = 1 /* border */ + octomap_light_texel_size * (
      grid_coord.xy +
      frame_data.probe.grid_size.xy * current_z_subcoord
    );

    uvec2 depth_base_texel_coord = 1 /* border */ + octomap_depth_texel_size * (
      grid_coord.xy +
      frame_data.probe.grid_size.xy * current_z_subcoord
    );

    const float border = 1.0;
    const vec2 light_hsize = 0.5 * octomap_light_texel_size - border;
    const vec2 depth_hsize = 0.5 * octomap_depth_texel_size - border;

    vec3 illuminance = texture(
      probe_light_map,
      (
        light_base_texel_coord +
        (light_hsize * (1.0 + octo_encode(N)))
        // @Think: not 100% sure, is +0.5 really not needed here?
      ) / frame_data.probe.light_map_texel_size
    ).rgb;

    vec2 d_d2 = texture(
      probe_depth_map,
      (
        depth_base_texel_coord +
        (depth_hsize * (1.0 + octo_encode(N)))
        // @Think: not 100% sure, is +0.5 really not needed here?
      ) / frame_data.probe.depth_map_texel_size
    ).rg;

    float weight = trilinear.x * trilinear.y * trilinear.z;

    if (frame_data.flags.debug_A) {
      float mean = d_d2.x;
      float variance = abs(d_d2.x * d_d2.x - d_d2.y);
      float dist = length(grid_cube_coord - grid_cube_vertex_coord);
      float diff = max(0.0, dist - mean);

      // see https://http.download.nvidia.com/developer/presentations/2006/gdc/2006-GDC-Variance-Shadow-Maps.pdf
      float chebyshev = (
        variance / (variance + diff * diff)
      );
      illuminance *= pow(chebyshev, 3.0); // @Cleanup :MoveToUniform
    }

    if (frame_data.flags.debug_B) {
      // @Incomplete: produces weird grid-like artifacts
      // maybe because of probes-in-the-wall and no "bias"?
      vec3 probe_direction = normalize(grid_cube_vertex_coord - grid_cube_coord);

      float smooth_backface = dot(probe_direction, N) + 1.0;
      weight *= smooth_backface;
      /*
      float backface = max(0.0, dot(probe_direction, N));
      weight *= backface;
      */
    }

    const float min_weight = 0.000001;
    weight = max(min_weight, weight);
    sum += vec4(illuminance * weight, weight);
  }

  sum /= sum.a;
  return albedo * sum.rgb;
}

#endif // _COMMON_PROBES_GLSL_
