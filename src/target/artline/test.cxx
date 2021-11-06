#include <glm/glm.hpp>
#include <src/global.hxx>
#include <src/engine/system/artline/public.hxx>

#define DLL_EXPORT extern "C" __declspec(dllexport)

using namespace engine::system::artline;

glm::mat4 identity(double time) {
  return glm::mat4(1.0f);
}

float sphere(glm::vec3 position) {
  return glm::length(position) - 1.0f;
}

DLL_EXPORT DECL_DESCRIBE_FN(describe) {
  desc->models.push_back(Model {
    .unique_index = 0,
    /*
    .transform_fn = identity,
    .transform_fn_version = 0,
    */
    .transform = identity(0.0),
    /*
    .density_fn = sphere,
    .density_fn_version = 0,
    */
    .filename_mesh = "assets/gi_test_0.t06",
    .filename_albedo = "assets/texture-1px/albedo.png",
    .filename_normal = "assets/texture-1px/normal.png",
    .filename_romeao = "assets/texture-1px/romeao.png",
  });
}