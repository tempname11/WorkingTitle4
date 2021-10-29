#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"
#include "common/light_model.glsl"

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;

layout(binding = 4) uniform Frame { FrameData data; } frame;
layout(binding = 5) uniform accelerationStructureEXT accel;

// @Duplicate :UniformDirLight
layout(set = 1, binding = 0) uniform DirectionalLight {
  vec3 direction;
  vec3 irradiance;
} directional_light;

const float Z_NEAR = 0.1;
const float Z_FAR = 10000.0;

const float RAY_T_MIN_MULT = 0.01;
const float RAY_T_MAX = 10000.0;

void main() {
  float depth = subpassLoad(zchannel).r;
  vec3 N = subpassLoad(gchannel0).rgb;
  vec3 albedo = subpassLoad(gchannel1).rgb;
  vec3 romeao = subpassLoad(gchannel2).rgb;

  if (depth == 1.0) {
    discard;
  }

  vec4 ndc = vec4(position, depth, 1.0);
  vec4 pos_view_projective = frame.data.projection_inverse * ndc;
  vec4 pos_world_projective = frame.data.view_inverse * pos_view_projective;
  vec3 eye_world = frame.data.view_inverse[3].xyz;
  vec3 pos_view = pos_view_projective.xyz / pos_view_projective.w;
  vec3 pos_world = pos_world_projective.xyz / pos_world_projective.w;

  if (!frame.data.flags.debug_C) { // @Tmp check that shadows are also slow
    rayQueryEXT ray_query;
    rayQueryInitializeEXT(
      ray_query,
      accel,
      0,
      0xFF,
      pos_world,
      RAY_T_MIN_MULT * length(pos_view), // try to compensate for Z precision.
      -directional_light.direction,
      RAY_T_MAX
    );
    rayQueryProceedEXT(ray_query);
    float t_intersection = rayQueryGetIntersectionTEXT(ray_query, false);

    if (t_intersection > 0.0) {
      discard;
    }
  }

  vec3 L = -directional_light.direction;
  vec3 V = normalize(eye_world - pos_world);
  vec3 H = normalize(V + L);

  float NdotV = max(0.0, dot(N, V));
  float NdotL = max(0.0, dot(N, L));
  float HdotV = max(0.0, dot(H, V));

  result = get_radiance_outgoing(
    albedo,
    romeao,
    NdotV,
    NdotL,
    HdotV,
    N,
    H,
    directional_light.irradiance
  );

  if (frame.data.flags.disable_direct_lighting) {
    result *= 0.0;
  }
}