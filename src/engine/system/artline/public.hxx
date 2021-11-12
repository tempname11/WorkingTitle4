#pragma once
#include <glm/glm.hpp>
#include <src/lib/task.hxx>
#include <src/lib/guid.hxx>
#include <src/engine/common/mesh.hxx>
#include <src/engine/session/public.hxx>

namespace engine::system::artline {

struct Data;

enum struct Status {
  Loading,
  Reloading,
  Ready,
  Unloading,
};

void init(Data *out);
void deinit(Data *it);

lib::Task *load(
  lib::cstr_range_t dll_filename,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
);

void unload(
  lib::GUID dll_id,
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

union ModelMesh {
  enum struct Type {
    File,
    Density,
  };

  struct File {
    Type type;
    lib::cstr_range_t path;
  };

  struct Density {
    Type type;
    DensityFn *density_fn;
    size_t density_fn_version;
  };

  Type type;
  File file;
  Density density;
};

struct Model {
  size_t unique_index;

  /* @Incomplete
  TransformFn *transform_fn;
  size_t transform_fn_version;
  */
  glm::mat4 transform;

  ModelMesh mesh;

  lib::cstr_range_t file_path_albedo;
  lib::cstr_range_t file_path_normal;
  lib::cstr_range_t file_path_romeao;
};

struct Description {
  lib::array_t<Model> *models;
};

#define DECL_DESCRIBE_FN(name) void name(engine::system::artline::Description *desc)
typedef DECL_DESCRIBE_FN(DescribeFn);

lib::array_t<common::mesh::T06> *generate(
  lib::allocator_t *misc,
  DensityFn *density_fn
);

} // namespace