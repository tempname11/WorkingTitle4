#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/sky.glsl"
#include "common/frame.glsl"
#include "common/probes.glsl"

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;
layout(binding = 4) uniform sampler2D probe_light_map;
layout(binding = 5) uniform sampler2D probe_depth_map;
layout(binding = 6) uniform Frame { FrameData data; } frame;

void main() {
  vec3 N_view = subpassLoad(gchannel0).rgb;
  vec3 albedo = subpassLoad(gchannel1).rgb;
  vec3 N = (frame.data.view_inverse * vec4(N_view, 0.0)).xyz;

  // @Cleanup share this logic with other L1 passes
  float depth = subpassLoad(zchannel).r;
  float z_near = 0.1; // @Cleanup :MoveToUniforms
  float z_far = 10000.0; // @Cleanup :MoveToUniforms
  float z_linear = z_near * z_far / (z_far + depth * (z_near - z_far));
  vec4 target_view_long = frame.data.projection_inverse * vec4(position, 1.0, 1.0);
  vec3 target_world = normalize((frame.data.view_inverse * target_view_long).xyz);
  if (depth == 1.0) {
    result = sky(
      target_world,
      frame.data.sky_sun_direction,
      frame.data.sky_intensity
    );
    if (frame.data.flags.disable_sky) {
      result *= 0.0;
    }
    return;
  }
  float perspective_correction = length(target_view_long.xyz);
  vec3 eye_world = (frame.data.view_inverse * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
  vec3 pos_world = eye_world + target_world * z_linear * perspective_correction;

  result = get_indirect_luminance(
    pos_world,
    N,
    frame.data,
    frame.data.probe.grid_world_position_zero,
    frame.data.probe.grid_world_position_delta,
    probe_light_map,
    probe_depth_map,
    albedo
  ); // :GI_Equations @Think

  if (frame.data.flags.disable_indirect_lighting) {
    result = vec3(0.0);
  }

  if (frame.data.flags.debug_B) {
    /*
    vec2 lbuffer_size = vec2(1280.0, 720.0); // @Cleanup :MoveToUniform

    if (frame.data.flags.debug_C) {
      result += texture(
        probe_light_map,
        clamp((
          (0.5 + 0.5 * vec2(position.x, -position.y))
            * lbuffer_size
            / frame.data.probe.light_map_texel_size
        ), 0.0, 1.0)
      ).rgb;
    } else {
      result += texture(
        probe_depth_map,
        clamp((
          (0.5 + 0.5 * vec2(position.x, -position.y))
            * lbuffer_size
            / frame.data.probe.depth_map_texel_size
        ), 0.0, 1.0)
      ).rgb;
    }
    */

    /*
    vec3 grid_coord = (
      (pos_world - frame.data.probe.grid_world_position_zero) /
      frame.data.probe.grid_world_position_delta
    );
    if (true
      && grid_coord.x >= 0
      && grid_coord.y >= 0
      && grid_coord.z >= 0
      && grid_coord.x <= frame.data.probe.grid_size.x - 1
      && grid_coord.y <= frame.data.probe.grid_size.y - 1
      && grid_coord.z <= frame.data.probe.grid_size.z - 1
    ) {
      result += floor(grid_coord) / (frame.data.probe.grid_size - 1);
      // result += pow(abs(fract(grid_coord) * 2.0 - 1.0), vec3(10.0));
    }
    */
  }
}