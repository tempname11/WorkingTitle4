#pragma once
#include "../mesh.hxx"

namespace engine::loading::mesh {

engine::common::mesh::T06 read_t06_file(const char *filename);

void deinit_t06(engine::common::mesh::T06 *it);

} // namespace
