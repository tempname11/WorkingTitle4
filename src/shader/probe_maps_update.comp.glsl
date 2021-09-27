#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"

layout(local_size_x = 6, local_size_y = 6, local_size_z = 1) in; // :OctomapSize

layout(binding = 0, r11f_g11f_b10f) uniform image2D probe_light_map;
layout(binding = 1) uniform sampler2D lbuffer2_image;
layout(binding = 2) uniform Frame { FrameData data; } frame;

// @Performance :UseComputeLocalSize

void main() {
  // :DDGI_N_Rays 64
  // @Incomplete :ProbePacking

  uvec2 texel_coord_base = 8 * uvec2(
    gl_WorkGroupID.x + gl_WorkGroupID.z * frame.data.probe.grid_size.x,
    gl_WorkGroupID.y
  );
  vec4 value = vec4(0.0);

  for (uint x = 0; x < 8; x++) {
    for (uint y = 0; y < 8; y++) {
      value += texture(
        lbuffer2_image,
        (vec2(texel_coord_base + uvec2(x, y)) + 0.5) / frame.data.probe.secondary_gbuffer_texel_size
      );
    }
  }
  value /= 64.0;
  imageStore(
    probe_light_map,
    ivec2(
      gl_WorkGroupID.x + gl_WorkGroupID.z * frame.data.probe.grid_size.x,
      gl_WorkGroupID.y
    ) * 6 + ivec2(gl_LocalInvocationID.xy),
    value
  );
}