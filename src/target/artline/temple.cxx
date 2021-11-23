#include <src/engine/system/artline/public.hxx>
#include <src/engine/system/artline/helpers.hxx>
#include <src/engine/system/artline/materials.hxx>

using namespace engine::system::artline;
using namespace glm;

float unite(float a) {
  return a;
}

float unite(float a, float b) {
  return lib::max(a, b);
}

template<typename... Args>
float unite(float a, float b, Args... xs) {
  return lib::max(lib::max(a, b), unite(xs...));
}

float intersect(float a) {
  return a;
}

float intersect(float a, float b) {
  return lib::min(a, b);
}

template<typename... Args>
float intersect(float a, float b, Args... xs) {
  return lib::min(lib::min(a, b), intersect(xs...));
}

DualContouringParams default_params = {
  .grid_size = uvec3(1), // override this
  .grid_min_bounds = vec3(-1.0f),
  .grid_max_bounds = vec3(1.0f),
  .normal_offset_mult = 0.1f,
  .normal_epsilon_mult = 0.01f,
};

DLL_EXPORT DECL_DESCRIBE_FN(describe) {
  float z = 0;

  if (1) { // Plane.
    auto params = default_params;
    params.grid_size = uvec3(2);

    lib::array::ensure_space(&desc->models, 1);
    desc->models->data[desc->models->count++] = Model {
      .transform = scaling(1000),
      .mesh {
        .gen0 = {
          .type = ModelMesh::Type::Gen0,
          .signed_distance_fn = [](vec3 p) {
            return -p.z;
          },
          .texture_uv_fn = [](vec3 p, vec3 N) {
            return vec2(0.0);
          },
          .params = params,
          .signature = 0x1001004,
        },
      },
      .material = materials::placeholder,
    };
  }

  if (1) { // Steps.
    auto params = default_params;
    params.grid_size = uvec3(85);
    params.grid_min_bounds.z = 0.0f;
    params.grid_max_bounds.z = 0.5f;

    lib::array::ensure_space(&desc->models, 1);
    desc->models->data[desc->models->count++] = Model {
      .transform = translation(vec3(0, 0, 0)) * scaling(20),
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
          .signature = 0x1004002,
        },
      },
      .material = materials::ambientcg::PavingStones107,
    };
  }
  z += 4.0f;

  const size_t NUM_COLUMNS = 6;
  for (size_t i = 0; i < NUM_COLUMNS; i++) { // Columns.
    { // Base.
      auto params = default_params;
      params.grid_size = uvec3(64, 64, 2);
      params.grid_min_bounds.z = 0.0f;
      params.grid_max_bounds.z = 1.0f;

      lib::array::ensure_space(&desc->models, 1);
      desc->models->data[desc->models->count++] = Model {
        .transform = (id
          * rotation(float(i) / NUM_COLUMNS, vec3(0, 0, 1))
          * translation(vec3(9.5f, 0, z))
          * scaling(1)
        ),
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
            .signature = 0x1007005,
          },
        },
        .material = materials::ambientcg::Concrete034,
      };
    }

    {
      auto params = default_params;
      params.grid_size = uvec3(64, 64, 2);
      params.grid_min_bounds.z = 0.0f;
      params.grid_max_bounds.z = 8.0f;

      lib::array::ensure_space(&desc->models, 1);
      desc->models->data[desc->models->count++] = Model {
        .transform = (id
          * rotation(float(i) / NUM_COLUMNS, vec3(0, 0, 1))
          * translation(vec3(9.5f, 0, z + 1.0f))
          * scaling(1)
        ),
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
            .signature = 0x1008004,
          },
        },
        .material = materials::ambientcg::Concrete034,
      };
    }

    { // Base.
      auto params = default_params;
      params.grid_size = uvec3(64, 64, 2);
      params.grid_min_bounds.z = 0.0f;
      params.grid_max_bounds.z = 1.0f;

      lib::array::ensure_space(&desc->models, 1);
      desc->models->data[desc->models->count++] = Model {
        .transform = (id
          * rotation(float(i) / NUM_COLUMNS, vec3(0, 0, 1))
          * translation(vec3(9.5f, 0, z + 9.0f))
          * scaling(1)
        ),
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
            .signature = 0x1007005,
          },
        },
        .material = materials::ambientcg::Concrete034,
      };
    }
  }
  z += 10.0f;

  if (1) { // Dome ring.
    auto params = default_params;
    params.grid_size = uvec3(33, 33, 2);
    params.grid_min_bounds.z = 0.0f;
    params.grid_max_bounds.z = 0.1f;

    lib::array::ensure_space(&desc->models, 1);
    desc->models->data[desc->models->count++] = Model {
      .transform = translation(vec3(0, 0, z)) * scaling(10.5),
      .mesh {
        .gen0 = {
          .type = ModelMesh::Type::Gen0,
          .signed_distance_fn = [](vec3 p) {
            return intersect(
              1.0f - length(vec2(p.x, p.y)),
              -(0.8f - length(vec2(p.x, p.y))),
              p.z,
              0.1f - p.z
            );
          },
          .texture_uv_fn = ad_hoc_sphere_texture_uv,
          .params = params,
          .signature = 0x1005007,
        },
      },
      .material = materials::ambientcg::Concrete034,
    };
  }
  z += 1.05f;

  if (1) { // Dome.
    auto params = default_params;
    params.grid_size = uvec3(41);

    lib::array::ensure_space(&desc->models, 1);
    desc->models->data[desc->models->count++] = Model {
      .transform = translation(vec3(0, 0, z)) * scaling(10),
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
          .signature = 0x1002003,
        },
      },
      .material = materials::ambientcg::Terrazzo009,
    };
  }
}
