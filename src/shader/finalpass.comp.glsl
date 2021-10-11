#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"

layout(binding = 0, rgba16) uniform image2D final_image;
layout(binding = 1) uniform sampler2D lbuffer;
layout(binding = 2) uniform sampler2D lbuffer_prev;
layout(binding = 3) uniform sampler2D zbuffer;
layout(binding = 4) uniform sampler2D zbuffer_prev;
layout(binding = 5) uniform Frame { FrameData data; } frame;

layout(
  local_size_x = 8, // :FinalPassWorkGroupSize
  local_size_y = 8,
  local_size_z = 1
) in;

const uint N_MOTION_SAMPLES = 16;

void main() {
  if (any(greaterThanEqual(
    gl_GlobalInvocationID.xy,
    frame.data.final_image_texel_size
  ))) {
    return;
  }

  vec2 position = (
    (vec2(gl_GlobalInvocationID.xy) + 0.5)
      / (gl_NumWorkGroups.xy * gl_WorkGroupSize.xy)
  );

  float depth = texture(
    zbuffer,
    position
  ).r;

  // @Performance: can fold 4 matrices into one!
  vec4 pos_world_projective = (
    frame.data.view_inverse
      * frame.data.projection_inverse
      * vec4(2.0 * position - 1.0, depth, 1.0)
  );
  vec4 ndc_prev = frame.data.projection * frame.data.view_prev * pos_world_projective;
  ndc_prev /= ndc_prev.w;
  vec2 position_prev = ndc_prev.xy * 0.5 + 0.5;

  vec3 luminance = texture(
    lbuffer,
    position
  ).rgb;

  if (!frame.data.flags.disable_motion_blur) {
    float w = 0.0;
    for (uint i = 1; i <= N_MOTION_SAMPLES; i++) {
      vec2 p = mix(position, position_prev, float(i) / N_MOTION_SAMPLES);
      if (false
        || p.x > 1.0 
        || p.y > 1.0
        || p.x < 0.0
        || p.y < 0.0
      ) {
        break;
      }
      luminance += texture(
        lbuffer,
        mix(position, position_prev, float(i) / N_MOTION_SAMPLES)
      ).rgb;
      w += 1.0;
    }
    luminance /= w;
  }

  float epsilon = 1e-10; // avoid division by zero.
  vec3 reinhard = luminance / (frame.data.luminance_average + luminance + epsilon);

  imageStore(
    final_image,
    ivec2(gl_GlobalInvocationID.xy),
    vec4(reinhard, 1.0)
  );
}
