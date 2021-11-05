#pragma once
#include <glm/glm.hpp>
#include <src/lib/task.hxx>
#include <src/engine/session/public.hxx>

namespace engine::system::artline {

struct Data;

lib::Task *load(
  char const* dll_filename,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
);

void reload_all(
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
);

using TransformFn = glm::mat4 (double time);
using DensityFn = float (glm::vec3 position);

// struct Light {}; // @Incomplete

struct Model {
  size_t unique_index;

  /* @Incomplete
  TransformFn *transform_fn;
  size_t transform_fn_version;
  */
  glm::mat4 transform;

  DensityFn *density_fn;
  size_t density_fn_version;

  std::string filename_albedo;
  std::string filename_normal;
  std::string filename_romeao;
};
struct Description {
  // std::vector<Light> lights;
  std::vector<Model> models;
};

#define DECL_DESCRIBE_FN(name) void name(engine::system::artline::Description *desc)
typedef DECL_DESCRIBE_FN(DescribeFn);

} // namespace