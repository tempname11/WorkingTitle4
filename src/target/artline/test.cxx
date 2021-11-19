#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <src/global.hxx>
#include <src/engine/system/artline/public.hxx>

#define DLL_EXPORT extern "C" __declspec(dllexport)

using namespace engine::system::artline;

glm::mat4 id = glm::mat4(1.0f);

float sphere(glm::vec3 position, glm::vec3 center, float radius) {
  return radius - glm::length(position - center);
}

uint64_t model0_signature = 0x0001002;
float model0_signed_distance(glm::vec3 position) {
  return std::min(
    std::min(
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

glm::vec2 model0_texture_uv(glm::vec3 position, glm::vec3 N) {
  auto s = position;
  auto r = glm::length(glm::vec2(s.x, s.y));
  auto dir = glm::normalize(glm::vec2(s.x, s.y));
  auto t = glm::atan(r, glm::abs(s.z)) / (0.5f * glm::pi<float>());

  return glm::mix(
    0.5f + 0.5f * (dir * t),
    0.5f + 0.5f * (dir * r),
    abs(N.z)
  );
}

glm::mat4 rotation(float turns, glm::vec3 axis) {
  return glm::rotate(id, glm::pi<float>() * 2.0f * turns, axis);
}

glm::mat4 translation(glm::vec3 t) {
  return glm::mat4(
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    t.x, t.y, t.z, 1
  );
}

glm::mat4 scaling(float m) {
  return glm::mat4(
    m, 0, 0, 0,
    0, m, 0, 0,
    0, 0, m, 0,
    0, 0, 0, 1
  );
}

DLL_EXPORT DECL_DESCRIBE_FN(describe) {
  lib::array::ensure_space(&desc->models, 1);
  desc->models->data[desc->models->count++] = Model {
    .transform = (
      translation(glm::vec3(15, 15, 15))
        * scaling(100)
        /*
        * rotation(
          0.25,
          glm::vec3(0, 1, 0)
        )
        */
    ),
    .mesh {
      .gen0 = {
        .type = ModelMesh::Type::Gen0,
        .signed_distance_fn = model0_signed_distance,
        .texture_uv_fn = model0_texture_uv,
        .signature = model0_signature,
      },
    },
    /*
    .file_path_albedo = lib::cstr::from_static("assets/ambientcg/Marble020_4K-PNG/Marble020_4K_Color.png"),
    .file_path_normal = lib::cstr::from_static("assets/ambientcg/Marble020_4K-PNG/Marble020_4K_NormalGL.png"),
    .file_path_romeao = lib::cstr::from_static("assets/ambientcg/Marble020_4K-PNG/Marble020_4K_Roughness.png"),

    .file_path_albedo = lib::cstr::from_static("assets/ambientcg/Terrazzo009_8K-PNG/Terrazzo009_8K_Color.png"),
    .file_path_normal = lib::cstr::from_static("assets/ambientcg/Terrazzo009_8K-PNG/Terrazzo009_8K_NormalGL.png"),
    .file_path_romeao = lib::cstr::from_static("assets/ambientcg/Terrazzo009_8K-PNG/Terrazzo009_8K_Roughness.png"),
    */
    .file_path_albedo = lib::cstr::from_static("assets/ambientcg/Terrazzo009_8K-JPG/Terrazzo009_8K_Color.jpg"),
    .file_path_normal = lib::cstr::from_static("assets/ambientcg/Terrazzo009_8K-JPG/Terrazzo009_8K_NormalGL.jpg"),
    .file_path_romeao = lib::cstr::from_static("assets/ambientcg/Terrazzo009_8K-JPG/Terrazzo009_8K_Roughness.jpg"),
  };

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
      .file_path_albedo = lib::cstr::from_static("assets/texture-1px/albedo.png"),
      .file_path_normal = lib::cstr::from_static("assets/texture-1px/normal.png"),
      .file_path_romeao = lib::cstr::from_static("assets/texture-1px/romeao.png"),
    };
  }
}