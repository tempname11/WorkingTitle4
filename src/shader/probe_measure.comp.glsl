#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#include "common/sky.glsl"
#include "common/helpers.glsl"
#include "common/frame.glsl"
#include "common/probes.glsl"
#include "common/light_model.glsl"

layout(
  local_size_x = PROBE_RAY_COUNT_FACTORS.x,
  local_size_y = PROBE_RAY_COUNT_FACTORS.y,
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

layout(binding = 0, rgba16f) uniform writeonly image2D lbuffer; // :LBuffer2_Format
layout(binding = 1) uniform sampler2D probe_irradiance;
layout(binding = 3) uniform writeonly uimage2D probe_attention_write;
layout(binding = 4) readonly buffer ProbeWorkset {
  uvec4 data[];
} probe_worksets[PROBE_CASCADE_COUNT];
layout(binding = 5) uniform accelerationStructureEXT accel;
layout(binding = 6) readonly buffer GeometryRefs { PerInstance data[]; } geometry_refs;
layout(binding = 7) uniform Frame { FrameData data; } frame;
layout(binding = 8) uniform sampler albedo_sampler;
layout(binding = 9) uniform DirectionalLight {
  vec3 direction;
  vec3 irradiance;
} directional_light; // @Incomplete :ManyLights

layout(set = 1, binding = 0) uniform texture2D albedo_textures[];
layout(set = 2, binding = 0) uniform texture2D romeao_textures[];

layout(push_constant) uniform Cascade {
  vec3 world_position_delta;
  uint level;
} cascade;

const float RAY_T_MIN = 0.1;
const float RAY_T_MAX = 10000.0;

void main() {
  uvec3 probe_coord = probe_worksets[cascade.level].data[gl_WorkGroupID.x].xyz;
  uint ray_index = gl_LocalInvocationIndex;

  uvec2 z_subcoord = uvec2(
    probe_coord.z % frame.data.probe.grid_size_z_factors.x,
    probe_coord.z / frame.data.probe.grid_size_z_factors.x
  );

  uvec2 cascade_subcoord = uvec2(
    cascade.level % PROBE_CASCADE_COUNT_FACTORS.x,
    cascade.level / PROBE_CASCADE_COUNT_FACTORS.x
  );

  uvec2 combined_coord = (
    probe_coord.xy +
    frame.data.probe.grid_size.xy * (
      z_subcoord +
      frame.data.probe.grid_size_z_factors * (
        cascade_subcoord
      )
    )
  );

  ivec2 store_coord = ivec2(
    combined_coord * PROBE_RAY_COUNT_FACTORS +
    gl_LocalInvocationID.xy
  );

  rayQueryEXT ray_query;
  ivec3 infinite_grid_min = frame.data.probe.cascades[cascade.level].infinite_grid_min;
  vec3 origin_world = (
    frame.data.probe.grid_world_position_zero +
    cascade.world_position_delta * (0
      + (ivec3(probe_coord) - infinite_grid_min) % ivec3(frame.data.probe.grid_size)
      + infinite_grid_min
    )
  );
  vec3 raydir_world = get_probe_ray_direction(
    ray_index,
    PROBE_RAY_COUNT,
    frame.data.probe.random_orientation
  );
  rayQueryInitializeEXT(
    ray_query,
    accel,
    0,
    0xFF,
    origin_world,
    RAY_T_MIN,
    raydir_world,
    RAY_T_MAX
  );
  rayQueryProceedEXT(ray_query);
  float t_intersection = rayQueryGetIntersectionTEXT(ray_query, false);
  bool front_face = rayQueryGetIntersectionFrontFaceEXT(ray_query, false);

  if (t_intersection == 0.0 || !front_face) {
    vec3 result = vec3(0.0);
    if (true
      && t_intersection == 0.0
      && raydir_world.z > 0.0
      && !frame.data.flags.disable_sky
    ) {
      result = sky(
        raydir_world,
        frame.data.sky_sun_direction,
        frame.data.sky_intensity,
        false /* show_sun */
      );
    }
    imageStore(
      lbuffer,
      store_coord,
      vec4(result, 0.0)
    );
    return;
  }

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

  vec3 romeao = texture(
    sampler2D(
      romeao_textures[instance_index],
      albedo_sampler
    ),
    uv
  ).rgb;

  vec3 N = normalize(object_to_world * vec4(n_object_unnorm, 0.0));
  vec3 L = -directional_light.direction;
  vec3 V = -raydir_world;
  vec3 H = normalize(V + L);
  vec3 pos_world = origin_world + t_intersection * raydir_world;

  vec3 result = get_indirect_radiance(
    pos_world,
    N,
    frame.data,
    true, // prev
    min(PROBE_CASCADE_COUNT - 1, cascade.level + 1),
    probe_irradiance,
    probe_attention_write,
    albedo
  );

  { // directional light
    float NdotV = max(0.0, dot(N, V));
    float NdotL = max(0.0, dot(N, L));
    float HdotV = max(0.0, dot(H, V));

    rayQueryEXT ray_query_shadow;
    rayQueryInitializeEXT(
      ray_query_shadow,
      accel,
      0,
      0xFF,
      pos_world,
      RAY_T_MIN,
      -directional_light.direction,
      RAY_T_MAX
    );
    rayQueryProceedEXT(ray_query_shadow);
    float t_intersection_shadow = rayQueryGetIntersectionTEXT(ray_query_shadow, false);
    if (t_intersection_shadow == 0.0) {
      result += get_radiance_outgoing(
        albedo,
        romeao,
        NdotV,
        NdotL,
        HdotV,
        N,
        H,
        directional_light.irradiance
      );
    }
  }

  imageStore(
    lbuffer,
    store_coord,
    vec4(result, 0.0)
  );
}
