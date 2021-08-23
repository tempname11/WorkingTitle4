#pragma once
#include "../mesh.hxx"

namespace engine::loading::mesh {

engine::common::mesh::T05 read_t05_file(const char *filename);

void deinit_t05(engine::common::mesh::T05 *it);

void _unload_item(
  SessionData::Vulkan::Meshes::Item *item,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core
);

} // namespace
