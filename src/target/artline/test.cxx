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

uint64_t model0_signature = 0x00010000;
float model0(glm::vec3 position) {
  // return sphere(position, glm::vec3(0.0f), 0.9f);
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
    position.x
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
        * scaling(10)
        * rotation(
          0.25,
          glm::vec3(0, 1, 0)
        )
    ),
    .mesh {
      .density = {
        .type = ModelMesh::Type::Density,
        .fn = model0,
        .signature = model0_signature,
      },
    },
    .file_path_albedo = lib::cstr::from_static("assets/texture-1px/albedo.png"),
    .file_path_normal = lib::cstr::from_static("assets/texture-1px/normal.png"),
    .file_path_romeao = lib::cstr::from_static("assets/texture-1px/romeao.png"),
  };

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