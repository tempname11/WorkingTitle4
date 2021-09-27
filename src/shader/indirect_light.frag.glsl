#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"
#include "common/probes.glsl"

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;
layout(set = 0, binding = 4) uniform sampler2D probe_light_map;

layout(binding = 5) uniform Frame { FrameData data; } frame;

void main() {
  vec3 N_view = subpassLoad(gchannel0).rgb;
  vec3 N = (frame.data.view_inverse * vec4(N_view, 0.0)).xyz;

  // @CopyPaste (the whole big section below)
  float depth = subpassLoad(zchannel).r;
  if (depth == 1.0) { discard; }
  float z_near = 0.1; // @Cleanup :MoveToUniforms
  float z_far = 10000.0; // @Cleanup :MoveToUniforms
  float z_linear = z_near * z_far / (z_far + depth * (z_near - z_far));
  vec4 target_view_long = frame.data.projection_inverse * vec4(position, 1.0, 1.0);
  vec3 target_world = normalize((frame.data.view_inverse * target_view_long).xyz);
  float perspective_correction = length(target_view_long.xyz);
  vec3 eye_world = (frame.data.view_inverse * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
  vec3 pos_world = eye_world + target_world * z_linear * perspective_correction;

  vec3 grid_coord_float = clamp(
    (
      (pos_world - frame.data.probe.grid_world_position_zero) /
      frame.data.probe.grid_world_position_delta
    ),
    vec3(0.0),
    vec3(frame.data.probe.grid_size)
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
      uvec2(frame.data.probe.grid_size.x * grid_coord.z, 0)
    );
    vec3 illuminance = texture(
      probe_light_map,
      (
        probe_base_texel_coord +
        (2.0 + 2.0 * octo_encode(N))
      ) / frame.data.probe.light_map_texel_size
    ).rgb;

    vec3 probe_direction = normalize(grid_cube_vertex_coord - grid_cube_coord);

    float weight = trilinear.x * trilinear.y * trilinear.z;

    /*
    if (frame.data.flags.debug_A) {
      // @Incomplete: produces weird grid-like artifacts
      // maybe because of probes-in-the-wall and no visibility info?
      weight *= dot(probe_direction, N) + 1.0; // "smooth backface"
    }
    */

    const float min_weight = 0.01;
    weight = max(min_weight, weight);

    sum += vec4(illuminance * weight, weight);
  }

  result = sum.rgb / sum.a;

  if (frame.data.flags.disable_indirect_lighting) {
    result = vec3(0.0);
  }

  if (frame.data.flags.debug_B) {
    /*
    result += texture(
      probe_light_map,
      (0.5 + 0.5 * position) * vec2(1280.0, 720.0) / 2048.0 / 64.0
    ).rgb;
    */
    /*
    result += grid_coord0 / vec3(32,32,8);
    */
  }
}