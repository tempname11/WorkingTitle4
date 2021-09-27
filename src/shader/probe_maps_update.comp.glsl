#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"
#include "common/probes.glsl"

const uint ray_count = 64; // :DDGI_N_Rays 64

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

  vec3 map_direction = octo_decode(mod(gl_LocalInvocationID.xy - 1.0, 4.0) / 2.0 - 1.0);

  vec4 value = vec4(0.0);
  for (uint x = 0; x < 8; x++) {
    for (uint y = 0; y < 8; y++) {
      // :DDGI_N_Rays 64
      uint ray_index = x + y * 8;
      vec3 ray_direction = -spherical_fibonacci(ray_index, ray_count);

      vec3 ray_luminance = texture(
        lbuffer2_image,
        (vec2(texel_coord_base + uvec2(x, y)) + 0.5) / frame.data.probe.secondary_gbuffer_texel_size
      ).rgb;
      float weight = max(0.0, dot(map_direction, ray_direction));
      value += vec4(ray_luminance * weight, weight);
    }
  }

  imageStore(
    probe_light_map,
    ivec2(
      gl_WorkGroupID.x + gl_WorkGroupID.z * frame.data.probe.grid_size.x,
      gl_WorkGroupID.y
    ) * 6 + ivec2(gl_LocalInvocationID.xy),
    value.rgba / value.a
  );
}