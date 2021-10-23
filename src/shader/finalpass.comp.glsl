#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"

layout(binding = 0, rgba16) uniform image2D final_image;
// @Think: why is this rgba16, when FINAL_IMAGE_FORMAT = VK_FORMAT_B8G8R8A8_UNORM !?

layout(binding = 1, rgba16) uniform image2D lbuffer;
layout(binding = 2) uniform sampler2D lbuffer_prev;
layout(binding = 3) uniform sampler2D zbuffer;
// layout(binding = 4) uniform sampler2D zbuffer_prev;
layout(binding = 5) uniform Frame { FrameData data; } frame;

layout(
  local_size_x = 8, // :FinalPassWorkGroupSize
  local_size_y = 8,
  local_size_z = 1
) in;

const uint N_MOTION_SAMPLES = 32;

vec3 map(vec3 l) {
  return l / (frame.data.luminance_moving_average + l);
}

vec3 unmap(vec3 p) {
  return p * frame.data.luminance_moving_average / (1.0 - p);
}

void main() {
  if (any(greaterThanEqual(
    gl_GlobalInvocationID.xy,
    imageSize(final_image)
  ))) {
    return;
  }

  vec2 position = (
    (vec2(gl_GlobalInvocationID.xy) + 0.5)
      / imageSize(final_image)
  );

  vec3 m = map(imageLoad(
    lbuffer,
    ivec2(gl_GlobalInvocationID.xy)
  ).rgb);

  float depth = texture(
    zbuffer,
    position
  ).r;

  // @Performance: can fold 4 matrices into one!
  vec4 ndc = vec4(2.0 * position - 1.0, depth, 1.0);
  vec4 pos_world_projective = (
    frame.data.view_inverse
      * frame.data.projection_inverse
      * ndc
  );
  vec4 ndc_prev = frame.data.projection_prev * frame.data.view_prev * pos_world_projective;
  ndc_prev /= ndc_prev.w;
  vec2 position_prev = ndc_prev.xy * 0.5 + 0.5;

  if (!frame.data.flags.disable_TAA && frame.data.is_frame_sequential) {
    vec2 p = position_prev;

    if (true
      && p.x <= 1.0 
      && p.y <= 1.0
      && p.x >= 0.0
      && p.y >= 0.0
    ) {
      vec3 m_t = map(texture(
        lbuffer_prev,
        position_prev
      ).rgb);

      vec3 px = map(imageLoad(
        lbuffer,
        ivec2(gl_GlobalInvocationID.xy) + ivec2(1, 0)
      ).rgb);
      vec3 nx = map(imageLoad(
        lbuffer,
        ivec2(gl_GlobalInvocationID.xy) + ivec2(-1, 0)
      ).rgb);
      vec3 py = map(imageLoad(
        lbuffer,
        ivec2(gl_GlobalInvocationID.xy) + ivec2(0, 1)
      ).rgb);
      vec3 ny = map(imageLoad(
        lbuffer,
        ivec2(gl_GlobalInvocationID.xy) + ivec2(0, -1)
      ).rgb);

      vec3 x_max = max(m, max(px, nx));
      vec3 x_min = min(m, min(px, nx));
      vec3 y_max = max(m, max(py, ny));
      vec3 y_min = min(m, min(py, ny));

      m_t = min(m_t, 0.5 * (x_max + y_max));
      m_t = max(m_t, 0.5 * (x_min + y_min));

      m = (1.0 * m + 31.0 * m_t) / 32.0; // @Cleanup

      imageStore(
        lbuffer,
        ivec2(gl_GlobalInvocationID.xy),
        vec4(unmap(m), 1.0)
      );
    }
  }

  memoryBarrierImage(); // not really sure if this works right.

  if (!frame.data.flags.disable_motion_blur) {
    vec4 sum = vec4(m, 1.0);
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
      sum.rgb += map(imageLoad(
        lbuffer,
        ivec2(p * imageSize(final_image))
      ).rgb);
      sum.w += 1.0;
    }
    m = sum.rgb / sum.w;
  }

  imageStore(
    final_image,
    ivec2(gl_GlobalInvocationID.xy),
    vec4(m, 1.0)
  );
}
