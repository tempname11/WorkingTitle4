#ifndef _COMMON_PROBES_GLSL_
#define _COMMON_PROBES_GLSL_

#include "constants.glsl"
#include "frame.glsl"

uint PROBE_MAX_SKIPS = 8;
const float PROBE_ACCUMULATOR_MAX = 128.0; // ~frames

const uint PROBE_RAY_COUNT = 64; // GI_N_Rays
const uvec2 PROBE_RAY_COUNT_FACTORS = uvec2(8, 8);

const uvec2 OCTOMAP_TEXEL_SIZE = uvec2(8, 8); // :ProbeOctoSize
const float MIN_PROBE_WEIGHT = 0.000001;
const float MIN_INITIAL_WEIGHT = 0.01;

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

vec4 _get_cascade_irradiance(
  uint cascade_level,
  vec3 infinite_grid_coord_float,
  vec3 N,
  sampler2D probe_irradiance,
  usampler2D probe_confidence,
  writeonly uimage2D probe_attention,
  FrameData frame_data
) {
  vec3 grid_cube_coord = fract(infinite_grid_coord_float);
  ivec3 infinite_grid_coord = ivec3(floor(infinite_grid_coord_float));
  uvec3 grid_coord0 = infinite_grid_coord % frame_data.probe.grid_size;

  uvec2 cascade_subcoord = uvec2(
    cascade_level % PROBE_CASCADE_COUNT_FACTORS.x,
    cascade_level / PROBE_CASCADE_COUNT_FACTORS.x
  );

  vec4 sum = vec4(0.0);
  for (uint i = 0; i < 8; i++) {
    uvec3 grid_cube_vertex_coord = ivec3(
      (i & 1) == 1 ? 1 : 0,
      (i & 2) == 2 ? 1 : 0,
      (i & 4) == 4 ? 1 : 0
    );

    uvec3 grid_coord = (grid_coord0 + grid_cube_vertex_coord) % frame_data.probe.grid_size;

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

    uvec2 base_texel_coord = OCTOMAP_TEXEL_SIZE * combined_texel_coord;
    const vec2 unique_texel_size = OCTOMAP_TEXEL_SIZE - 1.0;
    vec3 irradiance = texture(
      probe_irradiance,
      (
        base_texel_coord + 0.5 + mod(
          0.5 + unique_texel_size * 0.5 * (1.0 + octo_encode(N)),
          unique_texel_size
        )
      ) / textureSize(probe_irradiance, 0)
    ).rgb;

    uvec4 confidence_packed = texture(
      probe_confidence,
      (combined_texel_coord + 0.5) / textureSize(probe_confidence, 0)
    ); // :ProbeConfidenceFormat
    float accumulator = confidence_packed.a / 65535.0 * PROBE_ACCUMULATOR_MAX;
    float volatility = confidence_packed.b / 65535.0;
    uint last_update_frame_number = confidence_packed.r * 65536 + confidence_packed.g;
    uint frames_passed = frame_data.number - last_update_frame_number;
    float beta = max(0.0, accumulator - min(frames_passed, PROBE_ACCUMULATOR_MAX));

    // initial weight
    float weight = beta / PROBE_ACCUMULATOR_MAX;

    // "smooth backface" produced weird grid-like artifacts,
    // so we just use a sane one. Should revisit this!
    vec3 probe_direction = normalize(grid_cube_vertex_coord - grid_cube_coord);
    float backface = dot(probe_direction, N) >= 0.0 ? 1.0 : 0.0;
    weight *= backface;

    if (backface > 0.0) {
      imageStore(
        probe_attention,
        ivec2(combined_texel_coord),
        uvec4(1, 0, 0, 0)
      );
    }

    const float min_weight = MIN_PROBE_WEIGHT;
    weight = max(min_weight, weight);
    weight *= trilinear.x * trilinear.y * trilinear.z;
    sum += vec4(irradiance * weight, weight);
  }

  return sum;
}

vec3 get_indirect_radiance(
  vec3 pos_world,
  vec3 N,
  FrameData frame_data,
  uint min_cascade_level,
  sampler2D probe_irradiance,
  usampler2D probe_confidence,
  writeonly uimage2D probe_attention,
  vec3 albedo
) {
  uint level_index[2];
  vec3 level_infinite_grid_coord_float[2];
  uint levels_ready = 0;
  float level0_weight;
  {
    for (uint c = min_cascade_level; c < PROBE_CASCADE_COUNT; c++) {
      // this loop might be slow, but we don't know that for sure.

      vec3 delta = frame_data.probe.cascades[c].world_position_delta;
      ivec3 infinite_grid_min = frame_data.probe.cascades[c].infinite_grid_min;
      vec3 infinite_grid_coord_float = (
        (pos_world - frame_data.probe.grid_world_position_zero)
        / delta
      );

      bool out_of_bounds = (false
        || any(lessThan(
          infinite_grid_coord_float,
          infinite_grid_min + 1
        ))
        || any(greaterThan(
          infinite_grid_coord_float,
          infinite_grid_min + ivec3(frame_data.probe.grid_size) - 2
        ))
      );

      vec3 weights = 1.0 - max(
        vec3(0.0),
        abs(
          (infinite_grid_coord_float - infinite_grid_min)
            / (vec3(frame_data.probe.grid_size) - 1.0)
          - 0.5
        ) - 0.25
      ) * 4.0;

      if (!out_of_bounds) {
        level_index[levels_ready] = c;
        level_infinite_grid_coord_float[levels_ready] = infinite_grid_coord_float;
        levels_ready++;

        if (levels_ready == 1) {
          level0_weight = min(min(weights.x, weights.y), weights.z);
        }

        if (levels_ready == 2) {
          break;
        }
      }
    }
  }

  vec4 sum = vec4(0.0);
  float level_weight[2] = {
    level0_weight,
    (1.0 - level0_weight) // * 0.25 // next level is inherently less accurate
  };
  for (uint i = 0; i < levels_ready; i++) {
    sum += level_weight[i] * _get_cascade_irradiance(
      level_index[i],
      level_infinite_grid_coord_float[i],
      N,
      probe_irradiance,
      probe_confidence,
      probe_attention,
      frame_data
    );
  }

  if (sum.a > 0.0) {
    sum /= sum.a;
  } else {
    sum = vec4(frame_data.luminance_moving_average); // @Hack
  }

  return albedo * sum.rgb;
}

#endif // _COMMON_PROBES_GLSL_
