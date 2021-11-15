#include <src/lib/base.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/mutex.hxx>
#include <src/lib/u64_table.hxx>
#include "public.hxx"

namespace engine::system::artline {

struct CachedMesh {
  size_t ref_count;
  lib::Task *pending;
  lib::array_t<lib::GUID> *mesh_ids;
};

struct CachedTexture {
  size_t ref_count;
  lib::Task *pending;
  lib::GUID texture_id;
};

struct Data {
  struct DLL {
    lib::GUID id;
    Status status;

    // Everything here uses the CRT allocator.
    lib::cstr_range_t file_path;
    lib::array_t<lib::hash64_t> *mesh_keys;
    lib::array_t<lib::hash64_t> *texture_keys;
  };

  lib::mutex_t mutex;
  lib::array_t<DLL> *dlls;
  lib::u64_table_t<size_t> *dll_indices;
  lib::u64_table_t<CachedMesh> *meshes_by_key;
  lib::u64_table_t<CachedTexture> *textures_by_key;
};

} // namespace
