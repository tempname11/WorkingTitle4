#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(binding = 0, rgba16_snorm) uniform image2D gchannel0;
layout(binding = 1, rgba8) uniform image2D gchannel1;
layout(binding = 2, rgba8) uniform image2D gchannel2;
layout(binding = 3, r16) uniform image2D zchannel;
layout(binding = 4) uniform accelerationStructureEXT accel;
/*
layout(set = 1, binding = 0) uniform sampler2D xxx[];
layout(set = 2, binding = 0) readonly buffer yyy[];
*/

void main() {
  rayQueryEXT ray_query;
  vec3 origin_world = vec3(1.0) + 2.0 * gl_WorkGroupID;
  vec3 raydir_world = vec3(0.0, 0.0, 1.0);
  rayQueryInitializeEXT(
    ray_query,
    accel,
    0,
    0xFF,
    origin_world,
    0.1, // @Temporary: ray_t_min
    raydir_world,
    1000.0 // @Temporary: ray_t_max
  );
  bool _incomplete = rayQueryProceedEXT(ray_query);
  float t_intersection = rayQueryGetIntersectionTEXT(ray_query, false);

  // @Incomplete
  imageStore(
    zchannel,
    ivec2(gl_WorkGroupID.xy) + ivec2(16 * gl_WorkGroupID.z, 0),
    vec4(t_intersection, 0.0, 0.0, 0.0)
  );
}
