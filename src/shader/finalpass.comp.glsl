#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"

layout(binding = 0, rgba16) uniform image2D final_image;
layout(binding = 1) uniform sampler2D lbuffer_image;
layout(binding = 2) uniform Frame { FrameData data; } frame;

layout(
  local_size_x = 8, // :FinalPassWorkGroupSize
  local_size_y = 8,
  local_size_z = 1
) in;

void main() {
  if (any(greaterThanEqual(
    gl_GlobalInvocationID.xy,
    frame.data.final_image_texel_size
  ))) {
    return;
  }

  vec3 luminance = texture(
    lbuffer_image,
    (vec2(gl_GlobalInvocationID.xy) + 0.5) / (gl_NumWorkGroups.xy * gl_WorkGroupSize.xy)
  ).rgb;

  float epsilon = 1e-10;
  vec3 reinhard = luminance / (frame.data.luminance_average + luminance + epsilon);

  imageStore(
    final_image,
    ivec2(gl_GlobalInvocationID.xy),
    vec4(reinhard, 1.0)
  );
}
