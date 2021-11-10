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
  /*
  desc->models.push_back(Model {
    .unique_index = 0,
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
        .density_fn = model0,
        .density_fn_version = 0,
      },
    },
    .filename_albedo = "assets/texture-1px/albedo.png",
    .filename_normal = "assets/texture-1px/normal.png",
    .filename_romeao = "assets/texture-1px/romeao.png",
  });

  desc->models.push_back(Model {
    .unique_index = 1,
    .transform = id,
    .mesh {
      .file = {
        .type = ModelMesh::Type::File,
        .filename = new std::string("assets/gi_test_0.t06"),
      },
    },
    .filename_albedo = "assets/texture-1px/albedo.png",
    .filename_normal = "assets/texture-1px/normal.png",
    .filename_romeao = "assets/texture-1px/romeao.png",
  });
  */
}