#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/sky.glsl"
#include "common/frame.glsl"
#include "common/light_model.glsl"

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;

// @Duplicate :UniformFrame
layout(binding = 4) uniform Frame {
  mat4 projection;
  mat4 view;
  mat4 projection_inverse;
  mat4 view_inverse;
  FrameFlags flags;
  uint end_marker;
} frame;

layout(binding = 5) uniform accelerationStructureEXT accel;

// @Duplicate :UniformDirLight
layout(set = 1, binding = 0) uniform DirectionalLight {
  vec3 direction;
  vec3 intensity; // @Terminology
} directional_light;

void main() {
  #ifndef NDEBUG
    if (frame.end_marker != 0xDeadBeef) {
      return;
    }
  #endif

  float depth = subpassLoad(zchannel).r;
  float z_near = 0.1; // @Temporary: move to uniforms
  float z_far = 10000.0; // @Temporary: move to uniforms
  float z_linear = z_near * z_far / (z_far + depth * (z_near - z_far));

  vec4 target_view_long = frame.projection_inverse * vec4(position, 1.0, 1.0);
  vec3 target_world = normalize((frame.view_inverse * target_view_long).xyz);
  vec3 eye_world = (frame.view_inverse * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
  float perspective_correction = length(target_view_long.xyz);
  vec3 V = -normalize(target_view_long.xyz);

  // @Performance: multiply on CPU
  vec3 L = -(frame.view * vec4(directional_light.direction, 0.0)).xyz;

  if (depth == 1.0) {
    if (frame.flags.show_sky) {
      result = sky(target_world, -directional_light.direction);
      return;
    } else {
      discard;
    }
  }

  rayQueryEXT ray_query;
  rayQueryInitializeEXT(
    ray_query,
    accel,
    0,
    0xFF,
    eye_world + target_world * z_linear * perspective_correction,
    0.1, // @Temporary: move ray_t_min to uniforms
    -directional_light.direction,
    1000.0 // @Temporary: move ray_t_max to uniforms
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

  if (frame.flags.show_normals) {
    result = N * 0.5 + vec3(0.5);
    return;
  }

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

  if (frame.flags.disable_direct_lighting) {
    result *= 0.0;
  }
}