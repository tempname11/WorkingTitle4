#include <src/lib/base.hxx>
#include <src/lib/guid.hxx>
#include <src/lib/mutex.hxx>
#include "public.hxx"

namespace engine::system::artline {

struct MeshInfo {
  size_t ref_count;
  std::string key;
};

struct TextureInfo {
  size_t ref_count;
  std::string key;
};

struct Data {
  struct DLL {
    lib::GUID id;
    lib::cstr_range_t filename; // uses crt_allocator
    Status status;
  };

  lib::mutex_t mutex;
  lib::array_t<DLL> *dlls;

  /*
  std::unordered_map<lib::GUID, DLL> dlls;
  std::unordered_map<lib::GUID, MeshInfo> meshes;
  std::unordered_map<lib::GUID, TextureInfo> textures;
  std::unordered_map<std::string, lib::GUID> meshes_by_key;
  std::unordered_map<std::string, lib::GUID> textures_by_key;
  */
};

} // namespace
