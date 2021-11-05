#include <shared_mutex>
#include <unordered_map>
#include <src/lib/guid.hxx>
#include "public.hxx"

namespace engine::system::artline {

struct Data {
  struct DLL {
    std::string filename;
    Status status;
  };

  std::shared_mutex rw_mutex;
  std::unordered_map<lib::GUID, DLL> dlls;
};

} // namespace
