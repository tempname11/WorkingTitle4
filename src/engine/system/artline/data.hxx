#include <src/lib/base.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/mutex.hxx>
#include <src/lib/u64_table.hxx>
#include "public.hxx"

namespace engine::system::artline {

struct CachedMesh {
  size_t ref_count;
  lib::array_t<lib::GUID> *mesh_ids;
};

struct CachedTexture {
  size_t ref_count;
  lib::GUID texture_id;
};

struct Data {
  struct DLL {
    lib::GUID id;
    lib::cstr_range_t filename; // uses crt allocator
    Status status;
  };

  lib::mutex_t mutex;
  lib::u64_table_t<DLL> *dlls;
  lib::u64_table_t<CachedMesh> *meshes_by_key;
  lib::u64_table_t<CachedTexture> *textures_by_key;
};

} // namespace
