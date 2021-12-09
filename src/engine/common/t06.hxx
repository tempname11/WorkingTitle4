#pragma once
#include "mesh.hxx"

namespace engine::common::t06 {

engine::common::mesh::T06 read_file(const char *filename);

void destroy(engine::common::mesh::T06 *it);

} // namespace
