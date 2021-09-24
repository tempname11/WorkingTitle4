#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"

layout(binding = 0, r11f_g11f_b10f) uniform image2D probe_light_map;
layout(binding = 1) uniform sampler2D lbuffer2_image;
layout(binding = 2) uniform Frame { FrameData data; } frame;

void main() {
  // @Incomplete :ProbePacking
  vec4 value = texture(
    lbuffer2_image,
    (vec2(gl_WorkGroupID.xy) + 0.5) / frame.data.probe.secondary_gbuffer_texel_size
  );
  imageStore(
    probe_light_map,
    ivec2(gl_WorkGroupID.xy),
    value
  );
}