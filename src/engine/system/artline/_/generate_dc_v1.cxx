#include <glm/glm.hpp>
#include <qef_simd.h>
#include <src/engine/common/mesh.hxx>
#include <src/lib/gfx/utilities.hxx>
#include "../public.hxx"

namespace engine::system::artline {

using common::mesh::T06;
using common::mesh::IndexT06;
using common::mesh::VertexT06;

void add_quad(
  glm::vec3 *positions,
  lib::array_t<VertexT06> **p_all_vertices,
  SignedDistanceFn *signed_distance_fn,
  TextureUvFn *texture_uv_fn,
  DualContouringParams *params
) {
  const auto cell_size = (
    params->grid_max_bounds - params->grid_min_bounds
  ) / glm::vec3(params->grid_size);
  lib::array::ensure_space(p_all_vertices, 6);
  size_t indices[6] = { 0, 1, 2, 2, 1, 3 };
  size_t neighbors[6][2] = {
    { 1, 2 },
    { 2, 0 },
    { 0, 1 },
    { 1, 3 },
    { 3, 2 },
    { 2, 1 },
  };
  for (size_t i = 0; i < 6; i++) {
    auto ix = indices[i];
    auto in = neighbors[i][0];
    auto im = neighbors[i][1];
    auto normal = glm::normalize(glm::cross(
      positions[im] - positions[ix],
      positions[in] - positions[ix]
    ));
    auto tangent = glm::normalize(
      (positions[im] + positions[in]) * 0.5f
        - positions[ix]
    );
    auto bitangent = glm::cross(normal, tangent);
    auto e = params->normal_epsilon_mult;
    auto offset = cell_size * tangent * params->normal_offset_mult;
    float d = signed_distance_fn(positions[ix] + offset);
    float dt = signed_distance_fn(positions[ix] + offset + cell_size * tangent * e);
    float db = signed_distance_fn(positions[ix] + offset + cell_size * bitangent * e);
    float dn = signed_distance_fn(positions[ix] + offset + cell_size * normal * e);
    auto gradient = glm::normalize(glm::vec3(d - dt, d - db, d - dn));
    auto surface_normal = (
      tangent * gradient.x +
      bitangent * gradient.y +
      normal * gradient.z
    );
    if (glm::dot(surface_normal, normal) <= 0.0f) {
      surface_normal = normal; // decline
    }

    auto uv = texture_uv_fn(positions[ix], surface_normal);
    (*p_all_vertices)->data[(*p_all_vertices)->count++] = VertexT06 {
      .position = positions[ix],
      .normal = surface_normal,
      .uv = uv,
    };
  }
}

lib::array_t<T06> *generate_dc_v1(
  lib::allocator_t *misc,
  SignedDistanceFn *signed_distance_fn,
  TextureUvFn *texture_uv_fn,
  DualContouringParams *params
) {
  const auto cell_size = (
    params->grid_max_bounds - params->grid_min_bounds
  ) / glm::vec3(params->grid_size);
  const auto grid_size = params->grid_size;

  auto evals_buffer = (float *) malloc(sizeof(float)
    * (grid_size.x + 1)
    * (grid_size.y + 1)
    * (grid_size.z + 1)
  );
  auto points_buffer = (glm::vec3 *) malloc(sizeof(glm::vec3)
    * grid_size.x
    * grid_size.y
    * grid_size.z
  );

  for (size_t z = 0; z < grid_size.z + 1; z++) {
    for (size_t y = 0; y < grid_size.y + 1; y++) {
      for (size_t x = 0; x < grid_size.x + 1; x++) {
        auto base_position = (
          params->grid_min_bounds + cell_size * glm::vec3(x, y, z)
        );
        auto e = signed_distance_fn(base_position);
        if (e == 0) {
          e = -0.00001f; // @Hack for perfect hits
        }
        evals_buffer[0
          + x
          + y * (grid_size.x + 1)
          + z * (grid_size.x + 1) * (grid_size.y + 1)
        ] = e;
      }
    }
  }

  for (size_t z = 0; z < grid_size.z; z++) {
    for (size_t y = 0; y < grid_size.y; y++) {
      for (size_t x = 0; x < grid_size.x; x++) {
        size_t count = 0;
        glm::vec3 cube_positions[12];
        glm::vec3 normals[12];

        glm::vec3 axes[] = {
          glm::vec3(1, 0, 0),
          glm::vec3(0, 1, 0),
          glm::vec3(0, 0, 1),
        };

        // find all edges with sign change, add their interpolated 0-positions
        // into a list, along with normal.
        for (size_t k = 0; k < 3; k++) {
          for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 2; j++) {
              glm::vec3 p0 = (
                float(i) * axes[(k + 1) % 3] +
                float(j) * axes[(k + 2) % 3]
              );
              glm::vec3 p1 = p0 + axes[k];
              auto d0 = evals_buffer[0
                + (x + int(p0.x))
                + (y + int(p0.y)) * (grid_size.x + 1)
                + (z + int(p0.z)) * (grid_size.x + 1) * (grid_size.y + 1)
              ];
              auto d1 = evals_buffer[0
                + (x + int(p1.x))
                + (y + int(p1.y)) * (grid_size.x + 1)
                + (z + int(p1.z)) * (grid_size.x + 1) * (grid_size.y + 1)
              ];
              if (d0 * d1 <= 0) {
                glm::vec3 p; // on cube
                float d;
                glm::vec3 pos;
                for (size_t r = 0; r < 4; r++) {
                  p = (
                    abs(d1) * p0 +
                    abs(d0) * p1
                  ) / (abs(d0) + abs(d1));
                  pos = (
                    params->grid_min_bounds +
                    cell_size * (glm::vec3(x, y, z) + p)
                  );
                  d = signed_distance_fn(pos);
                  if (d == 0) {
                    break;
                  } else if (d0 * d > 0) {
                    d0 = d;
                    p0 = p;
                  } else {
                    d1 = d;
                    p1 = p;
                  }
                }
                auto e = params->normal_epsilon_mult;
                float dx = signed_distance_fn(pos + cell_size * glm::vec3(e, 0, 0));
                float dy = signed_distance_fn(pos + cell_size * glm::vec3(0, e, 0));
                float dz = signed_distance_fn(pos + cell_size * glm::vec3(0, 0, e));

                cube_positions[count] = p;
                normals[count] = glm::normalize(glm::vec3(d - dx, d - dy, d - dz));
                count++;
              }
            }
          }
        }

        if (count > 0) {
          glm::vec3 solution;
          qef_solve_from_points_3d(
            (float *) cube_positions,
            (float *) normals,
            count,
            (float *) &solution
          );
          if (params->clamp_qef) {
            solution = glm::clamp(solution, glm::vec3(0), glm::vec3(1));
          }
          points_buffer[0
            + x
            + y * grid_size.x
            + z * grid_size.x * grid_size.y
          ] = (
            params->grid_min_bounds +
            cell_size * (glm::vec3(x, y, z) + solution)
          );
        }
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
          + x
          + y * (grid_size.x + 1)
          + z * (grid_size.x + 1) * (grid_size.y + 1)
        );
        float e0 = evals_buffer[i0];
        float ex = evals_buffer[i0 + 1];
        float ey = evals_buffer[i0 + (grid_size.x + 1)];
        float ez = evals_buffer[i0 + (grid_size.x + 1) * (grid_size.y + 1)];
        if (e0 * ex <= 0 && y > 0 && z > 0) {
          auto sign = e0 <= 0 ? -1 : 1;
          glm::vec3 positions[4];
          for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 2; j++) {
              positions[j + 2 * i] = points_buffer[0
                + x
                + (y + (sign > 0 ? i : 1 - i) - 1) * grid_size.x
                + (z + j - 1) * grid_size.x * grid_size.y
              ];
            }
          }
          add_quad(
            positions,
            &all_vertices,
            signed_distance_fn,
            texture_uv_fn, params
          );
        }
        if (e0 * ey <= 0 && z > 0 && x > 0) {
          auto sign = e0 <= 0 ? -1 : 1;
          glm::vec3 positions[4];
          for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 2; j++) {
              positions[j + 2 * i] = points_buffer[0
                + (x + j - 1)
                + y * grid_size.x
                + (z + (sign > 0 ? i : 1 - i) - 1) * grid_size.x * grid_size.y
              ];
            }
          }
          add_quad(
            positions,
            &all_vertices,
            signed_distance_fn,
            texture_uv_fn, params
          );
        }
        if (e0 * ez <= 0 && x > 0 && y > 0) {
          auto sign = e0 <= 0 ? -1 : 1;
          glm::vec3 positions[4];
          for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 2; j++) {
              positions[j + 2 * i] = points_buffer[0
                + (x + (sign > 0 ? i : 1 - i) - 1)
                + (y + j - 1) * grid_size.x
                + z * grid_size.x * grid_size.y
              ];
            }
          }
          add_quad(
            positions,
            &all_vertices,
            signed_distance_fn,
            texture_uv_fn, params
          );
        }
      }
    }
  }

  // 65532 is largest uint16 divisible by 6.
  auto result = lib::array::create<common::mesh::T06>(
    misc,
    (all_vertices->count + 65532 - 1) / 65532
  );

  size_t vertices_processed = 0;
  while (vertices_processed < all_vertices->count) {
    uint32_t vertex_count = lib::min<size_t>(
      65532,
      all_vertices->count - vertices_processed
    );
    assert(vertex_count % 6 == 0); // quads
    uint32_t quad_count = vertex_count / 6;
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
      indices[6 * i + 0] = 6 * i + 0;
      indices[6 * i + 1] = 6 * i + 1;
      indices[6 * i + 2] = 6 * i + 2;
      indices[6 * i + 3] = 6 * i + 3;
      indices[6 * i + 4] = 6 * i + 4;
      indices[6 * i + 5] = 6 * i + 5;
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
  free(points_buffer);

  return result;
}

} // namespace
