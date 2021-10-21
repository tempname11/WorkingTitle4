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
  //return vec3(0, 0, -1);
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
  bool is_prev,
  sampler2D probe_light_map,
  sampler2D probe_depth_map,
  writeonly uimage2D probe_attention,
  vec3 albedo
) {
  bool out_of_bounds = true;
  uint cascade_level = 0;
  vec3 cascade_world_position_delta = vec3(0.0);
  vec3 grid_coord_float = vec3(0.0);
  ivec3 grid_coord0 = ivec3(0);
  {
    // @Performance: this is written to be obviously correct, but the loop is
    // probably way too slow. Figure out a fast approximation.

    vec3 delta = frame_data.probe.grid_world_position_delta_c0;
    for (uint c = 0; c < frame_data.probe.cascade_count; c++) {
      vec3 world_position_zero = (is_prev
        ? frame_data.probe.cascades[c].world_position_zero_prev
        : frame_data.probe.cascades[c].world_position_zero
      );
      grid_coord_float = (pos_world - world_position_zero) / delta;
      grid_coord0 = ivec3(floor(grid_coord_float));

      out_of_bounds = (false
        || grid_coord0.x < 0
        || grid_coord0.y < 0
        || grid_coord0.z < 0
        || grid_coord0.x > frame_data.probe.grid_size.x - 2
        || grid_coord0.y > frame_data.probe.grid_size.y - 2
        || grid_coord0.z > frame_data.probe.grid_size.z - 2
      );
      
      if (!out_of_bounds) {
        cascade_level = c;
        cascade_world_position_delta = delta;
        break;
      }

      delta *= 2.0;
    }
  }

  /*
  vec3 cascade_world_position_delta = (
    frame_data.probe.grid_world_position_delta_c0 *
    pow(2.0, cascade_level) // bit shift maybe?
  );

  vec3 world_position_zero = (is_prev
    ? frame_data.probe.cascades[cascade_level].world_position_zero_prev
    : frame_data.probe.cascades[cascade_level].world_position_zero
  );
  vec3 grid_coord_float = (
    (pos_world - world_position_zero) /
    cascade_world_position_delta
  );

  ivec3 grid_coord0 = ivec3(floor(grid_coord_float)); // needs to be signed to check bounds
  
  // are vector boolean expressions possible in GLSL? if so, this could use them.
  // @Cleanup: yes, see `lessThan`
  bool out_of_bounds = (false
    || grid_coord0.x < 0
    || grid_coord0.y < 0
    || grid_coord0.z < 0
    || grid_coord0.x > frame_data.probe.grid_size.x - 2
    || grid_coord0.y > frame_data.probe.grid_size.y - 2
    || grid_coord0.z > frame_data.probe.grid_size.z - 2
  );
  */

  vec3 grid_cube_coord = fract(grid_coord_float);

  uvec2 cascade_subcoord = uvec2(
    cascade_level % frame_data.probe.cascade_count_factors.x,
    cascade_level / frame_data.probe.cascade_count_factors.x
  );

  if (out_of_bounds) {
    return vec3(0.0);
  }

  vec4 sum = vec4(0.0);
  for (uint i = 0; i < 8; i++) {
    uvec3 grid_cube_vertex_coord = ivec3(
      (i & 1) == 1 ? 1 : 0,
      (i & 2) == 2 ? 1 : 0,
      (i & 4) == 4 ? 1 : 0
    );

    uvec3 grid_coord = grid_coord0 + grid_cube_vertex_coord;

    vec3 trilinear = mix(
      1.0 - grid_cube_coord,
      grid_cube_coord,
      vec3(grid_cube_vertex_coord)
    ); // choose first or second in each component depending on the cube vertex.

    uvec2 current_z_subcoord = uvec2(
      grid_coord.z % frame_data.probe.grid_size_z_factors.x,
      grid_coord.z / frame_data.probe.grid_size_z_factors.x
    );

    uvec2 combined_texel_coord = (
      grid_coord.xy +
      frame_data.probe.grid_size.xy * (
        current_z_subcoord +
        frame_data.probe.grid_size_z_factors * (
          cascade_subcoord
        )
      )
    );

    uvec2 light_base_texel_coord = octomap_light_texel_size * combined_texel_coord;
    const vec2 unique_texel_size = octomap_light_texel_size - 1.0;
    vec3 illuminance = texture(
      probe_light_map,
      (
        light_base_texel_coord + 0.5 + mod(
          0.5 + unique_texel_size * 0.5 * (1.0 + octo_encode(N)),
          unique_texel_size
        )
      ) / frame_data.probe.light_map_texel_size
    ).rgb;

    float weight = 1.0;

    if (!frame_data.flags.disable_indirect_shadows) {
      // Off by default now. See ENGINE_DEF_ENABLE_PROBE_DEPTH

      // old calculations with border on both sides.
      uint border = 1;
      uvec2 depth_base_texel_coord = border + octomap_depth_texel_size * combined_texel_coord;
      const vec2 depth_hsize = 0.5 * octomap_depth_texel_size - border;

      // Haven't yet seen results that show that the "normal bias" is helpful.
      // But maybe we're doing something else wrong, so leave it be.
      vec3 point_to_probe_biased = (
        cascade_world_position_delta * 0.5 * (
          grid_cube_vertex_coord - grid_cube_coord
        ) + N * frame_data.probe.normal_bias
      );
      float dist = length(point_to_probe_biased);
      point_to_probe_biased /= dist;

      vec2 values = texture(
        probe_depth_map,
        (
          depth_base_texel_coord +
          (depth_hsize * (1.0 + octo_encode(-point_to_probe_biased))) // no +0.5
        ) / frame_data.probe.depth_map_texel_size
      ).rg;

      float mean = values.x;
      float mean2 = values.y;
      float variance = abs(mean * mean - mean2);
      float diff = max(0.0, dist - mean);

      // see https://http.download.nvidia.com/developer/presentations/2006/gdc/2006-GDC-Variance-Shadow-Maps.pdf
      float chebyshev = (
        variance / (variance + diff * diff)
      );

      weight *= clamp(pow(chebyshev, 3.0), 0.0, 1.0); // @Cleanup :MoveToUniform
    }

    // "smooth backface" produced weird grid-like artifacts,
    // so we just use a sane one.
    vec3 probe_direction = normalize(grid_cube_vertex_coord - grid_cube_coord);
    float backface = dot(probe_direction, N) >= 0.0 ? 1.0 : 0.0;
    weight *= backface;

    imageStore(
      probe_attention,
      ivec2(combined_texel_coord),
      uvec4(1, 0, 0, 0)
    );

    const float min_weight = 0.000001; // @Cleanup :MoveToUniform
    weight = max(min_weight, weight);
    weight *= trilinear.x * trilinear.y * trilinear.z;

    sum += vec4(illuminance * weight, weight);
  }

  sum /= sum.a;
  return albedo * sum.rgb;
}

#endif // _COMMON_PROBES_GLSL_
