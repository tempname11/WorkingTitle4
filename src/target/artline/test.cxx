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

float cube_sd(glm::vec3 position) {
  return cube(
    position,
    vec3(0.0f),
    1.0f
  );
}

DLL_EXPORT DECL_DESCRIBE_FN(describe) {
  //!!
  for (size_t i = 0; i < 500; i++) { // cube
    auto params = default_params;
    params.grid_size = uvec3(2);

    lib::array::ensure_space(&desc->pieces, 1);
    desc->pieces->data[desc->pieces->count++] = Piece {
      .transform = (
        translation(vec3(0, 0, 1))
      ),
      .mesh {
        .gen0 = {
          .type = PieceMesh::Type::Gen0,
          .signed_distance_fn = cube_sd,
          .texture_uv_fn = triplanar_texture_uv,
          .params = params,
          .signature = 0x1001001,
        },
      },
      .material = materials::placeholder,
    };
  }
}