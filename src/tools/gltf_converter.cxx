#include <cassert>
#include <tiny_gltf.h>
#include <src/global.hxx>

namespace tools {

void gltf_converter(
  char const *path_gltf,
  char const *path_t06
) {
  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  bool ok = loader.LoadASCIIFromFile(&model, &err, &warn, path_gltf);
  assert(ok);

  if (!warn.empty()) {
    DBG("gltf_converter warning: {}", warn.c_str());
  }

  if (!err.empty()) {
    DBG("gltf_converter error: {}", err.c_str());
  }

  assert(0 && "Not implemented");
}

} // namespace