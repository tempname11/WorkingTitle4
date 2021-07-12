#pragma once
#include "common/mesh.hxx"

namespace engine::mesh {
  engine::common::mesh::T05 read_t05_file(const char *filename);
  void deinit_t05(engine::common::mesh::T05 *it);
}
