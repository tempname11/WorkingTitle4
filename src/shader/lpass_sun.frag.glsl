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
  vec3 intensity; // @Cleanup :ShouldBeIrradiance
} directional_light;

const float Z_NEAR = 0.1;
const float Z_FAR = 10000.0;

const float RAY_T_MIN = 0.1;
const float RAY_T_MAX = 10000.0;

void main() {
  #ifndef NDEBUG
    // @Cleanup: move this sanity check somewhere?
    if (frame.data.end_marker != 0xDeadBeef) {
      result = vec3(1.0, 0.0, 0.0);
      return;
    }
  #endif

  float depth = subpassLoad(zchannel).r;
  float z_linear = Z_NEAR * Z_FAR / (Z_FAR + depth * (Z_NEAR - Z_FAR));
  // @Cleanup :SimplerWorldSpacePos

  vec4 target_view_long = frame.data.projection_inverse * vec4(position, 1.0, 1.0);
  vec3 target_world = normalize((frame.data.view_inverse * target_view_long).xyz);
  vec3 eye_world = (frame.data.view_inverse * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
  float perspective_correction = length(target_view_long.xyz);
  vec3 V = -normalize(target_view_long.xyz);

  // @Performance: can multiply on CPU
  vec3 L = -(frame.data.view * vec4(directional_light.direction, 0.0)).xyz;

  if (depth == 1.0) {
    discard;
  }

  rayQueryEXT ray_query;
  rayQueryInitializeEXT(
    ray_query,
    accel,
    0,
    0xFF,
    // @Cleanup :SimplerWorldSpacePos
    eye_world + target_world * z_linear * perspective_correction,
    RAY_T_MIN,
    -directional_light.direction,
    RAY_T_MAX
  );
  rayQueryProceedEXT(ray_query); // should we use the result here?
  float t_intersection = rayQueryGetIntersectionTEXT(ray_query, false);

  if (t_intersection > 0.0) {
    discard;
  }

  vec3 N = subpassLoad(gchannel0).rgb;
  vec3 H = normalize(V + L);

  float NdotV = max(0.0, dot(N, V));
  float NdotL = max(0.0, dot(N, L));
  float HdotV = max(0.0, dot(H, V));

  vec3 albedo = subpassLoad(gchannel1).rgb;
  vec3 romeao = subpassLoad(gchannel2).rgb;

  result = get_radiance_outgoing(
    albedo,
    romeao,
    NdotV,
    NdotL,
    HdotV,
    N,
    H,
    directional_light.intensity
  );

  if (frame.data.flags.disable_direct_lighting) {
    result *= 0.0;
  }
}