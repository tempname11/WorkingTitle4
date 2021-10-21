#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#include "common/helpers.glsl"
#include "common/frame.glsl"
#include "common/probes.glsl"

layout(
  local_size_x = probe_ray_count_factors.x,
  local_size_y = probe_ray_count_factors.y,
  local_size_z = 1
) in;

layout(scalar, buffer_reference) readonly buffer IndexBufferRef {
  u16vec3 data[]; // :T06IndexType
};

struct PerVertex { // :T06VertexData
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

layout(binding = 0, rgba16_snorm) uniform image2D gchannel0;
layout(binding = 1, rgba8) uniform image2D gchannel1;
layout(binding = 2, rgba8) uniform image2D gchannel2;
layout(binding = 3, r16) uniform image2D zchannel;
layout(binding = 4) uniform accelerationStructureEXT accel;
layout(binding = 5) readonly buffer GeometryRefs { PerInstance data[]; } geometry_refs;
layout(binding = 6) uniform Frame { FrameData data; } frame;
layout(binding = 7) uniform sampler albedo_sampler;
layout(binding = 8, r32ui) uniform uimage2D probe_attention;

layout(set = 1, binding = 0) uniform texture2D albedo_textures[];
layout(set = 2, binding = 0) uniform texture2D romeao_textures[];

layout(push_constant) uniform Cascade {
  vec3 world_position_delta;
  uint level;
} cascade;

void main() {
  uvec3 probe_coord = gl_WorkGroupID;
  uint ray_index = gl_LocalInvocationIndex;

  uvec2 z_subcoord = uvec2(
    probe_coord.z % frame.data.probe.grid_size_z_factors.x,
    probe_coord.z / frame.data.probe.grid_size_z_factors.x
  );

  uvec2 cascade_subcoord = uvec2(
    cascade.level % frame.data.probe.cascade_count_factors.x,
    cascade.level / frame.data.probe.cascade_count_factors.x
  );

  ivec2 store_coord = ivec2(gl_GlobalInvocationID.xy
    + (
      probe_ray_count_factors *
      frame.data.probe.grid_size.xy *
      z_subcoord
    )
    + (
      probe_ray_count_factors *
      frame.data.probe.grid_size.xy *
      frame.data.probe.grid_size_z_factors *
      cascade_subcoord
    )
  );

  if (!frame.data.flags.debug_B) {
    ivec2 combined_coord = ivec2(
      probe_coord.xy +
      frame.data.probe.grid_size.xy * z_subcoord +
      frame.data.probe.grid_size.xy * frame.data.probe.grid_size_z_factors * cascade_subcoord
    );
    
    uint attention = imageLoad(
      probe_attention,
      combined_coord
    ).r;

    if (attention == 0) {
      return;
    }
  }

  rayQueryEXT ray_query;
  vec3 origin_world = (
    frame.data.probe.cascades[cascade.level].world_position_zero +
    cascade.world_position_delta * probe_coord
  );
  vec3 raydir_world = get_probe_ray_direction(
    ray_index,
    probe_ray_count,
    frame.data.probe.random_orientation
  );
  rayQueryInitializeEXT(
    ray_query,
    accel,
    0,
    0xFF,
    origin_world,
    0.01, // @Cleanup :MoveToUniforms ray_t_min
    raydir_world,
    1000.0 // @Cleanup :MoveToUniforms ray_t_max
  );
  rayQueryProceedEXT(ray_query);
  float t_intersection = rayQueryGetIntersectionTEXT(ray_query, false);
  bool front_face = rayQueryGetIntersectionFrontFaceEXT(ray_query, false);
  if (t_intersection == 0.0 || !front_face) {
    // We don't currently clear the G buffer, which should not matter,
    // because we ignore it when Z == 0.0.
    imageStore(
      zchannel,
      store_coord,
      vec4(t_intersection == 0.0 ? -1.0 : 0.0)
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

  PerVertex v0 = vertices.data[int(indices.x)];
  PerVertex v1 = vertices.data[int(indices.y)];
  PerVertex v2 = vertices.data[int(indices.z)];

  vec3 n_object_unnorm = barycentric_interpolate(
    bary,
    v0.normal,
    v1.normal,
    v2.normal
  );

  vec2 uv = barycentric_interpolate(
    bary,
    v0.uv,
    v1.uv,
    v2.uv
  );

  // @Incomplete: LODs are unavailable,
  // because compute shaders have no implicit gradients.
  // (and even they did for some reason, the result would be wrong)

  vec3 albedo = texture(
    sampler2D(
      albedo_textures[instance_index],
      albedo_sampler
    ),
    uv
  ).rgb;

  // @Performance @Think: do we even need this in G2?
  vec3 romeao = texture(
    sampler2D(
      romeao_textures[instance_index],
      albedo_sampler
    ),
    uv
  ).rgb;

  vec3 n_world = normalize(object_to_world * vec4(n_object_unnorm, 0.0));

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
    vec4(albedo, 0.0)
  );

  imageStore(
    gchannel2,
    store_coord,
    vec4(romeao, 0.0)
  );
}
