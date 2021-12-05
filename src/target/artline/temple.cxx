#include <src/engine/system/artline/public.hxx>
#include <src/engine/system/artline/helpers.hxx>
#include <src/engine/system/artline/materials.hxx>

using namespace engine::system::artline;
using namespace glm;

DualContouringParams default_params = {
  .grid_size = uvec3(1), // override this
  .grid_min_bounds = vec3(-1.0f),
  .grid_max_bounds = vec3(1.0f),
  .normal_offset_mult = 0.1f,
  .normal_epsilon_mult = 0.01f,
};

void do_ground(Description *desc, glm::mat4 *transform) {
  auto params = default_params;
  params.grid_size = uvec3(2);

  lib::array::ensure_space(&desc->models, 1);
  desc->models->data[desc->models->count++] = Model {
    .transform = *transform,
    .mesh {
      .gen0 = {
        .type = ModelMesh::Type::Gen0,
        .signed_distance_fn = [](vec3 p) {
          return intersect(
            1.0 - abs(p.x),
            1.0 - abs(p.y),
            1.0 - abs(p.z)
          );
        },
        .texture_uv_fn = [](vec3 p, vec3 N) {
          return vec2(0.0);
        },
        .params = params,
        .signature = 0x1001001,
      },
    },
    .material = materials::placeholder,
  };
}

void do_wall(Description *desc, glm::mat4 *transform) {
  auto params = default_params;
  params.grid_size = uvec3(2);

  lib::array::ensure_space(&desc->models, 1);
  desc->models->data[desc->models->count++] = Model {
    .transform = *transform,
    .mesh {
      .gen0 = {
        .type = ModelMesh::Type::Gen0,
        .signed_distance_fn = [](vec3 p) {
          return intersect(
            1.0 - abs(p.x),
            1.0 - abs(p.y),
            1.0 - abs(p.z)
          );
        },
        .texture_uv_fn = [](vec3 p, vec3 N) {
          return triplanar_texture_uv(p * 2.5f, N);
        },
        .params = params,
        .signature = 0x1009001,
      },
    },
    .material = materials::ambientcg::Bricks059,
  };
}

void do_steps(Description *desc, glm::mat4 *transform) {
  auto params = default_params;
  params.grid_size = uvec3(85);
  params.grid_min_bounds.z = 0.0f;
  params.grid_max_bounds.z = 0.5f;

  lib::array::ensure_space(&desc->models, 1);
  desc->models->data[desc->models->count++] = Model {
    .transform = *transform,
    .mesh {
      .gen0 = {
        .type = ModelMesh::Type::Gen0,
        .signed_distance_fn = [](vec3 p) {
          auto r = -1e9f;
          const size_t N = 5;
          for (size_t i = 0; i < N; i++) {
            r = unite(
              r,
              intersect(
                (0.8f + 0.2f * float(i) / (N - 1)) - abs(p.x),
                (0.8f + 0.2f * float(i) / (N - 1)) - abs(p.y),
                (0.2f - 0.15f * float(i) / (N - 1)) - abs(p.z),
                p.z
              )
            );
          }
          return r;
        },
        .texture_uv_fn = [](vec3 p, vec3 N) {
          return triplanar_texture_uv(p * 2.5f, N);
        },
        .params = params,
        .signature = 0x1002001,
      },
    },
    .material = materials::ambientcg::PavingStones107,
  };
}

void do_column_base(Description *desc, glm::mat4 *transform) {
  auto params = default_params;
  params.grid_size = uvec3(64, 64, 2);
  params.grid_min_bounds.z = 0.0f;
  params.grid_max_bounds.z = 1.0f;

  lib::array::ensure_space(&desc->models, 1);
  desc->models->data[desc->models->count++] = Model {
    .transform = *transform,
    .mesh {
      .gen0 = {
        .type = ModelMesh::Type::Gen0,
        .signed_distance_fn = [](vec3 p) {
          return intersect(
            1.0f - glm::length(vec2(p.x, p.y)),
            p.z,
            1.0f - p.z
          );
        },
        .texture_uv_fn = [](vec3 p, vec3 N){
          return vec2(
            atan2(p.y, p.x) / (2 * lib::PI),
            p.z / (2 * lib::PI)
          );
        },
        .params = params,
        .signature = 0x1003001,
      },
    },
    .material = materials::ambientcg::Concrete034,
  };
}

void do_column(Description *desc, glm::mat4 *transform) {
  auto params = default_params;
  params.grid_size = uvec3(64, 64, 2);
  params.grid_min_bounds.z = 0.0f;
  params.grid_max_bounds.z = 8.0f;

  lib::array::ensure_space(&desc->models, 1);
  desc->models->data[desc->models->count++] = Model {
    .transform = *transform,
    .mesh {
      .gen0 = {
        .type = ModelMesh::Type::Gen0,
        .signed_distance_fn = [](vec3 p) {
          auto t = lib::min(0.0f, sin(atan2(p.y, p.x) * 20.0f) * 0.07f);
          return intersect(
            0.9f - glm::length(vec2(p.x, p.y)) + t,
            p.z,
            8.0f - p.z
          );
        },
        .texture_uv_fn = [](vec3 p, vec3 N){
          return vec2(
            atan2(p.y, p.x) / (2 * lib::PI),
            p.z / (2 * lib::PI)
          );
        },
        .params = params,
        .signature = 0x1004001,
      },
    },
    .material = materials::ambientcg::Concrete034,
  };
}

void do_dome_ring(Description *desc, glm::mat4 *transform) {
  auto params = default_params;
  params.grid_size = uvec3(33, 33, 2);
  params.grid_min_bounds.z = 0.0f;
  params.grid_max_bounds.z = 0.1f;

  lib::array::ensure_space(&desc->models, 1);
  desc->models->data[desc->models->count++] = Model {
    .transform = *transform,
    .mesh {
      .gen0 = {
        .type = ModelMesh::Type::Gen0,
        .signed_distance_fn = [](vec3 p) {
          return intersect(
            1.0f - length(vec2(p.x, p.y)),
            -(0.66f - length(vec2(p.x, p.y))),
            p.z,
            0.1f - p.z
          );
        },
        .texture_uv_fn = ad_hoc_sphere_texture_uv,
        .params = params,
        .signature = 0x1005002,
      },
    },
    .material = materials::ambientcg::Concrete034,
  };
}

void do_dome(Description *desc, glm::mat4 *transform) {
  auto params = default_params;
  params.grid_size = uvec3(41);

  lib::array::ensure_space(&desc->models, 1);
  desc->models->data[desc->models->count++] = Model {
    .transform = *transform,
    .mesh {
      .gen0 = {
        .type = ModelMesh::Type::Gen0,
        .signed_distance_fn = [](vec3 p) {
          return intersect(
            sphere(
              p,
              vec3(0.0f),
              1.0f
            ),
            -sphere(
              p,
              vec3(0.0f),
              0.9f
            ),
            p.z
          );
        },
        .texture_uv_fn = ad_hoc_sphere_texture_uv,
        .params = params,
        .signature = 0x1006001,
      },
    },
    .material = materials::ambientcg::Terrazzo009,
  };
}

void do_podium(Description *desc, glm::mat4 *transform) {
  auto params = default_params;
  params.grid_size = uvec3(64);
  params.grid_min_bounds.z = 0.0f;

  lib::array::ensure_space(&desc->models, 1);
  desc->models->data[desc->models->count++] = Model {
    .transform = *transform,
    .mesh {
      .gen0 = {
        .type = ModelMesh::Type::Gen0,
        .signed_distance_fn = [](vec3 p) {
          return intersect(
            1.0f - p.z,
            0.5f + p.z * p.z * 0.5f - length(vec2(p.x, p.y)),
            p.z
          );
        },
        .texture_uv_fn = [](vec3 p, vec3 N) {
          // return triplanar_texture_uv(p, N);
          return vec2(0.1);
        },
        .params = params,
        .signature = 0x1007004,
      },
    },
    // .material = materials::ambientcg::Metal035, @Incomplete: no metallicness yet
    .material = materials::ambientcg::Bricks059,
  };
}

void do_artifact(Description *desc, glm::mat4 *transform) {
  auto params = default_params;
  params.grid_size = uvec3(128);

  lib::array::ensure_space(&desc->models, 1);
  desc->models->data[desc->models->count++] = Model {
    .transform = *transform,
    .mesh {
      .gen0 = {
        .type = ModelMesh::Type::Gen0,
        .signed_distance_fn = [](vec3 p) {
          const float k = 6.0;
          float c = cos(k * p.z);
          float s = sin(k * p.z);
          mat2 m = mat2(c, -s, s, c);
          vec3 q = vec3(m * vec2(p.x, p.y), p.z);

          vec2 v = vec2(length(vec2(q.x, q.z)) - 0.84f, q.y);
          return 0.15f - length(v);
        },
        .texture_uv_fn = triplanar_texture_uv,
        .params = params,
        .signature = 0x1008001,
      },
    },
    .material = materials::placeholder,
  };
}

DLL_EXPORT DECL_DESCRIBE_FN(describe) {
  float z = 0;

  {
    auto transform = translation(vec3(0, 0, -1000)) * scaling(1000);
    do_ground(desc, &transform);
  }

  for (int8_t i = -1; i <= 1; i++) {
    auto transform = translation(vec3(i * 60, 60, 20)) * scaling(30);
    do_wall(desc, &transform);
  }

  {
    auto transform = translation(vec3(0, 0, 0)) * scaling(20);
    do_steps(desc, &transform);
  }
  z += 4.0f;

  {
    auto transform = (id
      * translation(vec3(0, 0, z))
      * scaling(3.0f)
    );
    do_podium(desc, &transform);
  }

  {
    auto transform = (id
      * translation(vec3(0, 0, z + 5.0f))
      * scaling(2.0f)
    );
    do_artifact(desc, &transform);
  }

  const size_t NUM_COLUMNS = 6;
  for (size_t i = 0; i < NUM_COLUMNS; i++) {
    {
      auto transform = (id
        * rotation(float(i) / NUM_COLUMNS, vec3(0, 0, 1))
        * translation(vec3(9.5f, 0, z))
        * scaling(1)
      );
      do_column_base(desc, &transform);
    }

    {
      auto transform = (id
        * rotation(float(i) / NUM_COLUMNS, vec3(0, 0, 1))
        * translation(vec3(9.5f, 0, z + 1.0f))
        * scaling(1)
      );
      do_column(desc, &transform);
    }

    {
      auto transform = (id
        * rotation(float(i) / NUM_COLUMNS, vec3(0, 0, 1))
        * translation(vec3(9.5f, 0, z + 9.0f))
        * scaling(1)
      );
      do_column_base(desc, &transform);
    }
  }
  z += 10.0f;

  {
    auto transform = translation(vec3(0, 0, z)) * scaling(11.5);
    do_dome_ring(desc, &transform);
  }
  z += 1.15f;

  {
    auto transform = translation(vec3(0, 0, z)) * scaling(10);
    do_dome(desc, &transform);
  }
}
