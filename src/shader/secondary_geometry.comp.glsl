#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable
#include "common/helpers.glsl"
#include "common/frame.glsl"
#include "common/probes.glsl"

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in; // :DDGI_N_Rays 64
const uint ray_count = gl_WorkGroupSize.x * gl_WorkGroupSize.y;

layout(binding = 0, rgba16_snorm) uniform image2D gchannel0;
layout(binding = 1, rgba8) uniform image2D gchannel1;
layout(binding = 2, rgba8) uniform image2D gchannel2;
layout(binding = 3, r16) uniform image2D zchannel;
layout(binding = 4) uniform accelerationStructureEXT accel;

layout(scalar, buffer_reference) readonly buffer IndexBufferRef {
  u16vec3 data[]; // @See :T06IndexType
};

struct PerVertex { // @See :T06VertexData
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

layout(binding = 6) uniform Frame { FrameData data; } frame;

// layout(set = 2, binding = 0) uniform texture2D all_albedo_textures[];
// @Incomplete :Textures

void main() {
  ivec2 store_coord = ( // :SecondaryCoordEncoding
    ivec2(gl_GlobalInvocationID.xy) +
    ivec2(frame.data.probe.grid_size.x * 8 * gl_GlobalInvocationID.z, 0)
  );

  rayQueryEXT ray_query;
  uint ray_index = gl_LocalInvocationIndex;
  uvec3 probe_coord = gl_WorkGroupID;
  vec3 origin_world = (
    frame.data.probe.grid_world_position_zero +
    frame.data.probe.grid_world_position_delta * probe_coord
  );
  vec3 raydir_world = -spherical_fibonacci(ray_index, ray_count);
  rayQueryInitializeEXT(
    ray_query,
    accel,
    0,
    0xFF,
    origin_world,
    0.1, // @Cleanup :MoveToUniforms ray_t_min
    raydir_world,
    1000.0 // @Cleanup :MoveToUniforms ray_t_max
  );
  rayQueryProceedEXT(ray_query);
  float t_intersection = rayQueryGetIntersectionTEXT(ray_query, false);
  bool front_face = rayQueryGetIntersectionFrontFaceEXT(ray_query, false);
  if (t_intersection == 0.0 || !front_face) {
    // We don't clear the G buffer, which should not matter.
    imageStore(
      zchannel,
      store_coord,
      vec4(0.0)
    );
    return;
  }
  // int custom_index = rayQueryGetIntersectionInstanceCustomIndexEXT(ray_query, false);
  int instance_index = rayQueryGetIntersectionInstanceIdEXT(ray_query, false);
  int geometry_index = rayQueryGetIntersectionPrimitiveIndexEXT(ray_query, false);
  vec2 bary = rayQueryGetIntersectionBarycentricsEXT(ray_query, false);
  mat4x3 object_to_world = rayQueryGetIntersectionObjectToWorldEXT(ray_query, false);

  u16vec3 indices = geometry_refs.data[instance_index].indices.data[geometry_index];
  VertexBufferRef vertices = geometry_refs.data[instance_index].vertices;
  vec3 n0_object = vertices.data[int(indices.x)].normal;
  vec3 n1_object = vertices.data[int(indices.y)].normal;
  vec3 n2_object = vertices.data[int(indices.z)].normal;
  vec3 n_object = normalize(barycentric_interpolate(bary, n0_object, n1_object, n2_object));

  // @Incomplete :Textures read normal map and apply that.
  vec3 n_world = frame.data.flags.debug_C ? n_object : (
    object_to_world * vec4(n_object, 0.0) 
  );

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

  imageStore(
    gchannel1,
    store_coord,
    vec4(1.0) // @Incomplete :Textures
  );

  imageStore(
    gchannel2,
    store_coord,
    vec4(1.0) // @Incomplete :Textures
  );
}
