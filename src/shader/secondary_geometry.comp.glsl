#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_buffer_reference2 : enable

layout(binding = 0, rgba16_snorm) uniform image2D gchannel0;
layout(binding = 1, rgba8) uniform image2D gchannel1;
layout(binding = 2, rgba8) uniform image2D gchannel2;
layout(binding = 3, r16) uniform image2D zchannel;
layout(binding = 4) uniform accelerationStructureEXT accel;

layout(buffer_reference) readonly buffer IndexBufferRef {
  u16vec3 data[];
};

struct PerVertex {
  vec3 position;
  vec3 tangent;
  vec3 bitangent;
  vec3 normal;
  vec2 uv;
};

layout(buffer_reference) readonly buffer VertexBufferRef {
  PerVertex data[];
};

struct PerInstance {
  IndexBufferRef indices;
  VertexBufferRef vertices;
};

layout(binding = 5) readonly buffer GeometryRefs {
  PerInstance data[];
} geometry_refs;

// layout(set = 2, binding = 0) uniform texture2D all_albedo_textures[];

// @Cleanup: move to another file
vec3 barycentric_interpolate(vec2 b, vec3 v0, vec3 v1, vec3 v2) {
    return (1.0 - b.x - b.y) * v0 + b.x * v1 + b.y * v2;
}

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
  if (t_intersection == 0.0) {
    return;
  }

  // int custom_index = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, false);
  int instance_index = rayQueryGetIntersectionInstanceIdEXT(ray_query, false);
  int geometry_index = rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, false);
  vec2 bary = rayQueryGetIntersectionBarycentricsEXT  (ray_query, false);

  u16vec3 indices = geometry_refs.data[instance_index].indices.data[geometry_index];

  vec3 n0_object = geometry_refs.data[instance_index].indices.data[int(indices.x)];
  vec3 n1_object = geometry_refs.data[instance_index].indices.data[int(indices.y)];
  vec3 n2_object = geometry_refs.data[instance_index].indices.data[int(indices.z)];
  vec3 n_object = normalize(barycentric_interpolate(bary, n0_object, n1_object, n2_object));

  // @Incomplete: read normal map and apply that.
  // @Incomplete: convert to world space. for now, assume they are same.
  vec3 n_world = n_object;

  ivec2 store_coord = ivec2(gl_WorkGroupID.xy) + ivec2(16 * gl_WorkGroupID.z, 0);
  imageStore(
    zchannel,
    store_coord,
    vec4(t_intersection, 0.0, 0.0, 0.0)
  );

  imageStore(
    gchannel0,
    store_coord,
    vec4(n_world, 0.0)
  );
}
