#include <vector>
#include <src/engine/common/mesh.hxx>

namespace tools::common::mesh {

struct T06_Builder {
  std::vector<engine::common::mesh::IndexT06> indices;
  std::vector<engine::common::mesh::VertexT06> vertices;
};

void write(const char *out_filename, T06_Builder *mesh);

} // namespace
