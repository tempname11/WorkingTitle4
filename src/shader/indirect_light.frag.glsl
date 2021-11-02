#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/sky.glsl"
#include "common/frame.glsl"
#include "common/probes.glsl"

const float Z_NEAR = 0.1;
const float Z_FAR = 10000.0;

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;
layout(binding = 4) uniform sampler2D probe_irradiance;
layout(binding = 5) uniform usampler2D probe_confidence;
layout(binding = 6) uniform Frame { FrameData data; } frame;
layout(binding = 7) uniform writeonly uimage2D probe_attention_write;
layout(binding = 8) uniform sampler2D probe_offsets;

void main() {
  // Sanity check for frame data.
  #ifndef NDEBUG
    if (frame.data.end_marker != 0xDeadBeef) {
      result = vec3(1.0, 0.0, 0.0);
      return;
    }
  #endif

  float depth = subpassLoad(zchannel).r;
  vec3 N = subpassLoad(gchannel0).rgb;
  vec3 albedo = subpassLoad(gchannel1).rgb;

  vec4 ndc = vec4(position, depth, 1.0);
  vec4 pos_world_projective = ( // @Performance: precompute matrices
    frame.data.view_inverse
      * frame.data.projection_inverse
      * ndc
  );

  if (depth == 1.0) {
    result = sky(
      normalize(pos_world_projective.xyz),
      frame.data.sky_sun_direction,
      frame.data.sky_intensity,
      true /* show_sun */
    );
    if (frame.data.flags.disable_sky) {
      result = vec3(0.0);
    }
    return;
  }

  vec3 pos_world = pos_world_projective.xyz / pos_world_projective.w;

  result = get_indirect_radiance(
    pos_world,
    N,
    frame.data,
    0,
    probe_irradiance,
    probe_confidence,
    probe_offsets,
    probe_attention_write,
    albedo
  );

  if (frame.data.flags.disable_indirect_lighting) {
    result = vec3(0.0);
  }
}