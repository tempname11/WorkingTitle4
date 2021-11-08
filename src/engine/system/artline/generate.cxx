#include <glm/glm.hpp>
#include <src/engine/common/mesh.hxx>
#include <src/lib/gfx/utilities.hxx>
#include <src/lib/gfx/marching_cubes.hxx>
#include "public.hxx"

namespace engine::system::artline {

using common::mesh::T06;
using common::mesh::IndexT06;
using common::mesh::VertexT06;

// @Incomplete: move to parameters
const auto grid_size = glm::uvec3(32);
const auto grid_min_bounds = glm::vec3(-1.0f);
const auto grid_max_bounds = glm::vec3(1.0f);
const float normal_epsilon_mult = 0.01f;
const size_t SOLVE_ITERATIONS_MAX = 4;
const float SOLVE_EPSILON = 0.001;

glm::uvec3 mc_vertices[8] = {
  glm::uvec3(0, 0, 0),
  glm::uvec3(1, 0, 0),
  glm::uvec3(1, 1, 0),
  glm::uvec3(0, 1, 0),
  glm::uvec3(0, 0, 1),
  glm::uvec3(1, 0, 1),
  glm::uvec3(1, 1, 1),
  glm::uvec3(0, 1, 1),
};

glm::vec3 mc_edge_middles[12] = {
  // bottom
  {0.5f, 0.0f, 0.0f},
  {1.0f, 0.5f, 0.0f},
  {0.5f, 1.0f, 0.0f},
  {0.0f, 0.5f, 0.0f},
  // top
  {0.5f, 0.0f, 1.0f},
  {1.0f, 0.5f, 1.0f},
  {0.5f, 1.0f, 1.0f},
  {0.0f, 0.5f, 1.0f},
  // middle
  {0.0f, 0.0f, 0.5f},
  {1.0f, 0.0f, 0.5f},
  {1.0f, 1.0f, 0.5f},
  {0.0f, 1.0f, 0.5f},
};

uint8_t mc_edge_connects[12][2] = {
  // bottom
  {0, 1},
  {1, 2},
  {2, 3},
  {3, 0},
  // top
  {4, 5},
  {5, 6},
  {6, 7},
  {7, 4},
  // middle
  {0, 4},
  {1, 5},
  {2, 6},
  {3, 7},
};


lib::Array<T06> *generate(
  DensityFn *density_fn
) {
  auto cell_size = (grid_max_bounds - grid_min_bounds) / glm::vec3(grid_size);

  auto densities_buffer = (float *) malloc(sizeof(float)
    * (grid_size.x + 1)
    * (grid_size.y + 1)
    * (grid_size.z + 1)
  );
  auto variants_buffer = (uint8_t *) malloc(sizeof(uint8_t)
    * grid_size.x
    * grid_size.y
    * grid_size.z
  );

  for (size_t z = 0; z < grid_size.z + 1; z++) {
    for (size_t y = 0; y < grid_size.y + 1; y++) {
      for (size_t x = 0; x < grid_size.x + 1; x++) {
        auto base_position = (
          grid_min_bounds + cell_size * glm::vec3(x, y, z)
        );
        densities_buffer[0
          + x
          + y * (grid_size.x + 1)
          + z * (grid_size.x + 1) * (grid_size.y + 1)
        ] = density_fn(base_position);
      }
    }
  }

  size_t total_triangles = 0;
  for (size_t z = 0; z < grid_size.z; z++) {
    for (size_t y = 0; y < grid_size.y; y++) {
      for (size_t x = 0; x < grid_size.x; x++) {
        uint8_t variant = 0;
        for (size_t i = 0; i < 8; i++) {
          glm::uvec3 vertex = mc_vertices[i];
          float density = densities_buffer[0
            + (x + vertex.x)
            + (y + vertex.y) * (grid_size.x + 1)
            + (z + vertex.z) * (grid_size.x + 1) * (grid_size.y + 1)
          ];
          if (density > 0) {
            variant += 1 << i;
          }
        }
        variants_buffer[0
          + x
          + y * grid_size.x
          + z * grid_size.x * grid_size.y
        ] = variant;
        total_triangles += lib::gfx::marching_cubes::triangle_count_table[variant];
      }
    }
  } 

  uint32_t vertex_count = checked_integer_cast<uint32_t>(total_triangles * 3);
  uint32_t index_count = vertex_count;

  assert(vertex_count < 65536); // @Incomplete: many meshes

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
    indices[i] = i;
  }

  size_t current_vertex = 0;
  for (size_t z = 0; z < grid_size.z; z++) {
    for (size_t y = 0; y < grid_size.y; y++) {
      for (size_t x = 0; x < grid_size.x; x++) {
        auto base_position = (
          grid_min_bounds + cell_size * glm::vec3(x, y, z)
        );
        uint8_t variant = variants_buffer[0
          + x
          + y * grid_size.x
          + z * grid_size.x * grid_size.y
        ];
        auto tris = lib::gfx::marching_cubes::triangle_count_table[variant];
        for (size_t i = 0; i < 3 * tris; i++) {
          auto edge_index = lib::gfx::marching_cubes::edge_table[variant][i];
          assert(edge_index < 12);

          glm::vec3 v;
          glm::vec3 position;
          float d;
          {
            auto v0 = mc_vertices[mc_edge_connects[edge_index][0]];
            auto v1 = mc_vertices[mc_edge_connects[edge_index][1]];
            float d0 = densities_buffer[
              + (x + v0.x)
              + (y + v0.y) * (grid_size.x + 1)
              + (z + v0.z) * (grid_size.x + 1) * (grid_size.y + 1)
            ];
            float d1 = densities_buffer[
              + (x + v1.x)
              + (y + v1.y) * (grid_size.x + 1)
              + (z + v1.z) * (grid_size.x + 1) * (grid_size.y + 1)
            ];
            for (size_t r = 0; r < SOLVE_ITERATIONS_MAX; r++) {
              v = (
                (abs(d1) * glm::vec3(v0) + abs(d0) * glm::vec3(v1))
                  / (abs(d0) + abs(d1))
              );
              position = base_position + v * cell_size;
              d = density_fn(position);
              if (abs(d) < SOLVE_EPSILON) {
                break;
              }
              if (d0 * d > 0) {
                d0 = d;
                v0 = v;
              } else {
                d1 = d;
                v1 = v;
              }
            }
          }

          float dx = density_fn(position + cell_size * glm::vec3(normal_epsilon_mult, 0.0f, 0.0f));
          float dy = density_fn(position + cell_size * glm::vec3(0.0f, normal_epsilon_mult, 0.0f));
          float dz = density_fn(position + cell_size * glm::vec3(0.0f, 0.0f, normal_epsilon_mult));
          glm::vec3 N = glm::normalize(glm::vec3(d - dx, d - dy, d - dz));

          vertices[current_vertex] = VertexT06 {
            .position = position, 
            .normal = N,
          };
          current_vertex++;
        }
      }
    }
  } 

  free(densities_buffer);
  free(variants_buffer);

  auto result = lib::array::create<common::mesh::T06>(lib::allocator::crt, 1);
  result->data[0] = common::mesh::T06 {
    .buffer = buffer,
    .index_count = index_count,
    .vertex_count = vertex_count,
    .indices = indices,
    .vertices = vertices,
  };
  result->count = 1;
  return result;
}

} // namespace
