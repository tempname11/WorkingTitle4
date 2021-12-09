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

struct ModelPart {
  glm::mat4 transform;
  lib::GUID mesh_id;
  lib::GUID texture_albedo_id;
  lib::GUID texture_normal_id;
  lib::GUID texture_romeao_id;
};

struct Model {
  lib::mutex_t mutex;
  lib::array_t<ModelPart> *parts;
};

struct LoadResult {
  lib::Task *completed;
  Model *model;
};

LoadResult load(
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
using SignedDistanceFn = float (glm::vec3 position);
using TextureUvFn = glm::vec2 (glm::vec3 position, glm::vec3 N);

struct DualContouringParams {
  glm::uvec3 grid_size;
  glm::vec3 grid_min_bounds;
  glm::vec3 grid_max_bounds;
  float normal_offset_mult;
  float normal_epsilon_mult;
  bool clamp_qef;
};

union PieceGeometry {
  enum struct Type {
    File,
    Gen0,
  };

  struct File {
    Type type;
    lib::cstr_range_t path;
  };

  struct Gen0 {
    Type type;
    SignedDistanceFn *signed_distance_fn;
    TextureUvFn *texture_uv_fn;
    DualContouringParams params;
    uint64_t signature;
  };

  Type type;
  File file;
  Gen0 gen0;
};

struct PieceMaterial {
  lib::cstr_range_t file_path_albedo;
  lib::cstr_range_t file_path_normal;
  lib::cstr_range_t file_path_romeao;
};

struct Piece {
  glm::mat4 transform;
  PieceGeometry geometry;
  PieceMaterial material;
};

struct Description {
  lib::array_t<Piece> *pieces;
};

#define DECL_DESCRIBE_FN(name) void name(engine::system::artline::Description *desc)
typedef DECL_DESCRIBE_FN(DescribeFn);

} // namespace