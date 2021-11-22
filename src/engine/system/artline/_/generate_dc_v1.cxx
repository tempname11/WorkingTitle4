#include <glm/glm.hpp>
#include <src/engine/common/mesh.hxx>
#include <src/lib/gfx/utilities.hxx>
#include "../public.hxx"

namespace engine::system::artline {

using common::mesh::T06;
using common::mesh::IndexT06;
using common::mesh::VertexT06;

// @Incomplete: move to parameters
const auto grid_size = glm::uvec3(64);
const auto grid_min_bounds = glm::vec3(-1.0f);
const auto grid_max_bounds = glm::vec3(1.0f);

lib::array_t<T06> *generate_dc_v1(
  lib::allocator_t *misc,
  SignedDistanceFn *signed_distance_fn,
  TextureUvFn *texture_uv_fn
) {
  auto cell_size = (grid_max_bounds - grid_min_bounds) / glm::vec3(grid_size);

  auto evals_buffer = (float *) malloc(sizeof(float)
    * (grid_size.x + 1)
    * (grid_size.y + 1)
    * (grid_size.z + 1)
  );

  for (size_t z = 0; z < grid_size.z + 1; z++) {
    for (size_t y = 0; y < grid_size.y + 1; y++) {
      for (size_t x = 0; x < grid_size.x + 1; x++) {
        auto base_position = (
          grid_min_bounds + cell_size * glm::vec3(x, y, z)
        );
        evals_buffer[0
          + x
          + y * (grid_size.x + 1)
          + z * (grid_size.x + 1) * (grid_size.y + 1)
        ] = signed_distance_fn(base_position);
      }
    }
  }

  auto all_vertices = lib::array::create<VertexT06>(
    lib::allocator::crt,
    0
  );

  for (size_t z = 0; z < grid_size.z; z++) {
    for (size_t y = 0; y < grid_size.y; y++) {
      for (size_t x = 0; x < grid_size.x; x++) {
        auto i0 = (0
          + x * 1
          + y * (grid_size.x + 1)
          + z * (grid_size.x + 1) * (grid_size.y + 1)
        );
        float e0 = evals_buffer[i0];
        float ex = evals_buffer[i0 + 1];
        float ey = evals_buffer[i0 + (grid_size.x + 1)];
        float ez = evals_buffer[i0 + (grid_size.x + 1) * (grid_size.y + 1)];
        if (e0 * ex <= 0) {
          lib::array::ensure_space(&all_vertices, 4);
          auto sign = e0 >= 0 ? 1 : -1;
          for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 2; j++) {
              auto position = grid_min_bounds + cell_size * glm::vec3(
                x + 0.5,
                y + sign * (i - 0.5),
                z + (j - 0.5)
              );
              auto normal = glm::vec3(sign, 0, 0);
              auto uv = texture_uv_fn(position, normal);
              all_vertices->data[all_vertices->count++] = VertexT06 {
                .position = position,
                .normal = normal,
                .uv = uv,
              };
            }
          }
        }
        if (e0 * ey <= 0) {
          lib::array::ensure_space(&all_vertices, 4);
          auto sign = e0 >= 0 ? 1 : -1;
          for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 2; j++) {
              auto position = grid_min_bounds + cell_size * glm::vec3(
                x + (j - 0.5),
                y + 0.5,
                z + sign * (i - 0.5)
              );
              auto normal = glm::vec3(0, sign, 0);
              auto uv = texture_uv_fn(position, normal);
              all_vertices->data[all_vertices->count++] = VertexT06 {
                .position = position,
                .normal = normal,
                .uv = uv,
              };
            }
          }
        }
        if (e0 * ez <= 0) {
          lib::array::ensure_space(&all_vertices, 4);
          auto sign = e0 >= 0 ? 1 : -1;
          for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 2; j++) {
              auto position = grid_min_bounds + cell_size * glm::vec3(
                x + sign * (i - 0.5),
                y + (j - 0.5),
                z + 0.5
              );
              auto normal = glm::vec3(0, 0, sign);
              auto uv = texture_uv_fn(position, normal);
              all_vertices->data[all_vertices->count++] = VertexT06 {
                .position = position,
                .normal = normal,
                .uv = uv,
              };
            }
          }
        }
      }
    }
  }

  auto result = lib::array::create<common::mesh::T06>(
    misc,
    (all_vertices->count + 65536 - 1) / 65536
  );

  size_t vertices_processed = 0;
  while (vertices_processed < all_vertices->count) {
    uint32_t vertex_count = lib::min<size_t>(
      65536,
      all_vertices->count - vertices_processed
    );
    assert(vertex_count % 4 == 0); // quads
    uint32_t quad_count = vertex_count / 4;
    uint32_t index_count = quad_count * 6;

    auto indices_size = index_count * sizeof(IndexT06);
    auto vertices_size = vertex_count * sizeof(VertexT06);
    auto aligned_size = lib::gfx::utilities::aligned_size(
      indices_size,
      sizeof(VertexT06)
    );
    size_t buffer_size = aligned_size + vertices_size;
    auto buffer = (uint8_t *) malloc(buffer_size);
    auto indices = (IndexT06 *) buffer;
    auto vertices = (VertexT06 *) (buffer + aligned_size);

    for (size_t i = 0; i < vertex_count; i++) {
      vertices[i] = all_vertices->data[vertices_processed + i];
    }
    for (size_t i = 0; i < quad_count; i++) {
      indices[6 * i + 0] = 4 * i + 0;
      indices[6 * i + 1] = 4 * i + 1;
      indices[6 * i + 2] = 4 * i + 2;
      indices[6 * i + 3] = 4 * i + 2;
      indices[6 * i + 4] = 4 * i + 1;
      indices[6 * i + 5] = 4 * i + 3;
    }

    result->data[result->count++] = T06 {
      .buffer = buffer,
      .index_count = index_count,
      .vertex_count = vertex_count,
      .indices = indices,
      .vertices = vertices,
    };
    vertices_processed += vertex_count;
  }

  lib::array::destroy(all_vertices);

  free(evals_buffer);

  return result;
}

} // namespace
