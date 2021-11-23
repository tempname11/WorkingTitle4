#include <src/engine/system/artline/public.hxx>
#include <src/engine/system/artline/helpers.hxx>
#include <src/engine/system/artline/materials.hxx>

using namespace engine::system::artline;

DualContouringParams default_params = {
  .grid_size = glm::uvec3(1), // override this
  .grid_min_bounds = glm::vec3(-1.0f),
  .grid_max_bounds = glm::vec3(1.0f),
  .normal_offset_mult = 0.1f,
  .normal_epsilon_mult = 0.01f,
};

glm::vec3 cork(glm::vec3 position) {
  auto f = 4.0f;
  auto p = f * position.z;
  return glm::vec3(
    position.x * sin(p) + position.y * cos(p),
    position.y * sin(p) - position.x * cos(p),
    position.z
  );
}

float cube_sd(glm::vec3 position) {
  return cube(
    position,
    glm::vec3(0.0f),
    1.0f
  );
}

float model1_sd(glm::vec3 position) {
  return cube(
    cork(position),
    glm::vec3(0.0f),
    0.5f
  );
}

float model2_sd(glm::vec3 position) {
  return lib::min(
    lib::min(
      sphere(
        position,
        glm::vec3(0.0f),
        0.9
      ),
      -sphere(
        position,
        glm::vec3(0.0f),
        0.8
      )
    ),
    position.z
  );
}

DLL_EXPORT DECL_DESCRIBE_FN(describe) {
  if (1) {
    auto params = default_params;
    params.grid_size = glm::uvec3(64);

    lib::array::ensure_space(&desc->models, 1);
    desc->models->data[desc->models->count++] = Model {
      .transform = (
        translation(glm::vec3(15, 15, 15))
          * scaling(10)
          * rotation(
            0.5,
            glm::vec3(0, 1, 0)
          )
      ),
      .mesh {
        .gen0 = {
          .type = ModelMesh::Type::Gen0,
          .signed_distance_fn = model1_sd,
          .texture_uv_fn = triplanar_texture_uv,
          .params = params,
          .signature = 0x1001003,
        },
      },
      .material = materials::ambientcg::Terrazzo009,
    };
  }

  if (1) {
    auto params = default_params;
    params.grid_size = glm::uvec3(64);

    lib::array::ensure_space(&desc->models, 1);
    desc->models->data[desc->models->count++] = Model {
      .transform = (
        translation(glm::vec3(15, 15, 15))
          * scaling(100)
          * rotation(
            0.5,
            glm::vec3(0, 1, 0)
          )
      ),
      .mesh {
        .gen0 = {
          .type = ModelMesh::Type::Gen0,
          .signed_distance_fn = model2_sd,
          .texture_uv_fn = ad_hoc_sphere_texture_uv,
          .params = params,
          .signature = 0x1002004,
        },
      },
      .material = materials::ambientcg::Marble020,
    };
  }

  if (1) {
    lib::array::ensure_space(&desc->models, 1);
    desc->models->data[desc->models->count++] = Model {
      .transform = id,
      .mesh {
        .file = {
          .type = ModelMesh::Type::File,
          .path = lib::cstr::from_static("assets/gi_test_0.t06"),
        },
      },
      .material = materials::placeholder,
    };
  }
}