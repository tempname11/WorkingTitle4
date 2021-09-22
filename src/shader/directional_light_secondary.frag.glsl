#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/light_model.glsl"
#include "common/frame.glsl"

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;

// @Duplicate in lpass_sun.flag.glsl
layout(binding = 4) uniform Frame {
  mat4 projection;
  mat4 view;
  mat4 projection_inverse;
  mat4 view_inverse;
  FrameFlags flags;
  uint end_marker;
} frame;

// @Duplicate in lpass_sun.flag.glsl
layout(set = 1, binding = 0) uniform DirectionalLight {
  vec3 direction;
  vec3 intensity; // @Terminology
} directional_light;

void main() {
  vec2 coord = (position * 0.5 + 0.5) * vec2(2048.0); // @Incomplete: use uniform for image size
  ivec3 work_group_id = ivec3(mod(coord.x, 16), coord.y, coord.x / 16); // @Incomplete: 16

  // unless noted otherwise, everything is in world space.
  vec3 eye = frame.view[3].xyz; // @Incomplete: a guess, need to check this

  vec3 origin = vec3(1.0) + 2.0 * work_group_id;
  vec3 raydir = vec3(0.0, 0.0, 1.0); // @Incomplete many rays
  float d = subpassLoad(zchannel).r;
  vec3 here = origin + d * raydir;

  vec3 L = -directional_light.direction;
  vec3 V = -raydir;
  vec3 N = subpassLoad(gchannel0).rgb;
  vec3 H = normalize(V + L);

  float NdotV = max(0.0, dot(N, V));
  float NdotL = max(0.0, dot(N, L));
  float HdotV = max(0.0, dot(H, V));

  vec3 albedo = subpassLoad(gchannel1).rgb;
  vec3 romeao = subpassLoad(gchannel2).rgb;

  // result = subpassLoad(zchannel).rgb;
  result = get_luminance_outgoing(
    albedo,
    romeao,
    NdotV,
    NdotL,
    HdotV,
    N,
    H,
    directional_light.intensity
  );
}
