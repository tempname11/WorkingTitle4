#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;
layout(set = 0, binding = 4) uniform sampler2D probe_light_map;

layout(binding = 5) uniform Frame { FrameData data; } frame;

void main() {
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

  uvec3 grid_coord = uvec3(
    clamp(
      (
        (pos_world - frame.data.probe.grid_world_position_zero) /
        frame.data.probe.grid_world_position_delta
      ),
      vec3(0.0),
      vec3(frame.data.probe.grid_size)
    )
  );
  // :ManyRays
  uvec2 packed_probe_coord = (
    grid_coord.xy +
    uvec2(frame.data.probe.grid_size.x * grid_coord.z, 0)
  );

  // @Incomplete :ProbePacking :ProbeEquation
  result = texture(
    probe_light_map,
    (packed_probe_coord + 0.5) / frame.data.probe.light_map_texel_size
  ).rgb;

  if (frame.data.flags.disable_indirect_lighting) {
    result = vec3(0.0);
  }

  /* debug stuff
  result += grid_coord / vec3(32,32,8);
  result += texture(
    probe_light_map,
    (0.5 + 0.5 * position) * vec2(1280.0, 720.0) / 2048.0 / 4.0 
  ).rgb;
  */
}