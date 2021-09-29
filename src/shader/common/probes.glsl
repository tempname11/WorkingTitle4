#ifndef _COMMON_PROBES_GLSL_
#define _COMMON_PROBES_GLSL_

#include "constants.glsl"
#include "frame.glsl"

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
  sampler2D probe_light_map
) {
  vec3 grid_coord_float = clamp(
    (
      (pos_world - frame_data.probe.grid_world_position_zero) /
      frame_data.probe.grid_world_position_delta
    ),
    vec3(0.0),
    vec3(frame_data.probe.grid_size)
  );
  uvec3 grid_coord0 = uvec3(grid_coord_float);
  vec3 grid_cube_coord = fract(grid_coord_float);
  
  vec4 sum = vec4(0.0);
  for (uint i = 0; i < 8; i++) {
    uvec3 grid_cube_vertex_coord = uvec3(
      (i & 1) == 1 ? 1 : 0,
      (i & 2) == 2 ? 1 : 0,
      (i & 4) == 4 ? 1 : 0
    );
    uvec3 grid_coord = grid_coord0 + grid_cube_vertex_coord;

    vec3 trilinear = mix( // choose first or second in each component depending on the vertex.
      vec3(1.0) - grid_cube_coord,
      grid_cube_coord,
      grid_cube_vertex_coord
    );

    // @Incomplete :ProbePacking
    uvec2 probe_base_texel_coord = 1 + 6 * (
      grid_coord.xy +
      uvec2(frame_data.probe.grid_size.x * grid_coord.z, 0)
    );
    vec3 illuminance = texture(
      probe_light_map,
      (
        probe_base_texel_coord +
        (2.0 + 2.0 * octo_encode(N))
      ) / frame_data.probe.light_map_texel_size
    ).rgb;

    vec3 probe_direction = normalize(grid_cube_vertex_coord - grid_cube_coord);

    float weight = trilinear.x * trilinear.y * trilinear.z;

    /*
    if (frame_data.flags.debug_A) {
      // @Incomplete: produces weird grid-like artifacts
      // maybe because of probes-in-the-wall and no visibility info?
      weight *= dot(probe_direction, N) + 1.0; // "smooth backface"
    }
    */

    const float min_weight = 0.01;
    weight = max(min_weight, weight);

    sum += vec4(illuminance * weight, weight);
  }

  // @Incomplete: material diffuse properties should be considered here!
  // :DDGI_Textures

  return sum.rgb / sum.a;
}

#endif // _COMMON_PROBES_GLSL_
