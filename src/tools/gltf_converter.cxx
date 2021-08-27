#include <algorithm>
#include <cassert>
#include <glm/glm.hpp>
#include <tiny_gltf.h>
#include <src/global.hxx>
#include <src/lib/io.hxx>
#include <src/lib/mkdir_p.hxx>
#include <src/engine/loading/group/common.hxx>
#include "common/mesh.hxx"

namespace tools {

void gltf_converter(
  char const *path_gltf,
  char const *path_folder
) {
  ZoneScoped;

  { ZoneScopedN("directory");
    auto result = mkdir_p(path_folder);
    assert(result == 0);
  }

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

  assert(model.meshes.size() > 0);
  size_t mesh_index = 0;
  for (auto &mesh : model.meshes) {
    size_t primitive_index = 0;
    for (auto &primitive : mesh.primitives) {
      common::mesh::T06_Builder builder;
    
      assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);
      uint16_t max_index = 0;
      { ZoneScopedN("indices");
        assert(primitive.indices >= 0); // @Incomplete
        auto accessor = &model.accessors[primitive.indices];
        assert(accessor->type == TINYGLTF_TYPE_SCALAR);
        assert(accessor->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT); // @Incomplete
        auto buffer_view = &model.bufferViews[accessor->bufferView];
        auto buffer = &model.buffers[buffer_view->buffer];

        for (size_t i = 0; i < accessor->count; i++) {
          auto stride = (
            buffer_view->byteStride > 0
              ? buffer_view->byteStride
              : sizeof(uint16_t)
          );
          auto offset = (0
            + accessor->byteOffset
            + buffer_view->byteOffset
            + i * stride
          );
          assert(offset + sizeof(uint16_t) <= buffer->data.size());

          auto ptr = (uint16_t *) &buffer->data[offset];
          builder.indices.push_back(*ptr);
          max_index = std::max(max_index, *ptr);
        }
      }

      { ZoneScopedN("vertices");
        assert(primitive.attributes.contains("POSITION"));
        assert(primitive.attributes.contains("NORMAL"));
        assert(primitive.attributes.contains("TEXCOORD_0"));

        auto accessor_position = &model.accessors[primitive.attributes["POSITION"]];
        auto accessor_normal = &model.accessors[primitive.attributes["NORMAL"]];
        auto accessor_texcoord = &model.accessors[primitive.attributes["TEXCOORD_0"]];

        auto count = accessor_position->count;
        assert(accessor_position->count == accessor_normal->count);
        assert(accessor_position->count == accessor_texcoord->count);

        assert(accessor_position->type == TINYGLTF_TYPE_VEC3);
        assert(accessor_position->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        auto buffer_view_position = &model.bufferViews[accessor_position->bufferView];
        auto buffer_position = &model.buffers[buffer_view_position->buffer];

        assert(accessor_normal->type == TINYGLTF_TYPE_VEC3);
        assert(accessor_normal->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        auto buffer_view_normal = &model.bufferViews[accessor_normal->bufferView];
        auto buffer_normal = &model.buffers[buffer_view_normal->buffer];
        
        for (size_t i = 0; i < count; i++) {
          auto stride_position = (
            buffer_view_position->byteStride > 0
              ? buffer_view_position->byteStride
              : sizeof(glm::vec3)
          );
          auto offset_position = (0
            + accessor_position->byteOffset
            + buffer_view_position->byteOffset
            + i * stride_position
          );
          assert(offset_position + sizeof(glm::vec3) <= buffer_position->data.size());

          auto stride_normal = (
            buffer_view_normal->byteStride > 0
              ? buffer_view_normal->byteStride
              : sizeof(glm::vec3)
          );
          auto offset_normal = (0
            + accessor_normal->byteOffset
            + buffer_view_normal->byteOffset
            + i * stride_normal
          );
          assert(offset_normal + sizeof(glm::vec3) <= buffer_normal->data.size());

          engine::common::mesh::VertexT06 vertex = {
            .position = *((glm::vec3 *) (&buffer_position->data[offset_position])),
            .normal = *((glm::vec3 *) (&buffer_normal->data[offset_normal])),
            /*
            .tangent = _,
            .bitangent = _,
            .uv = _,
            */
          };
          float epsilon = 1e-3;
          assert(vertex.position.x + epsilon >= accessor_position->minValues[0]);
          assert(vertex.position.y + epsilon >= accessor_position->minValues[1]);
          assert(vertex.position.z + epsilon >= accessor_position->minValues[2]);
          assert(vertex.position.x - epsilon <= accessor_position->maxValues[0]);
          assert(vertex.position.y - epsilon <= accessor_position->maxValues[1]);
          assert(vertex.position.z - epsilon <= accessor_position->maxValues[2]);
          builder.vertices.push_back(vertex);
        }

        assert(max_index == builder.vertices.size() - 1);
      }

      char buf[1024];
      auto buf_len = snprintf(
        buf,
        1024,
        "%s/m%02zu_p%03zu.t06",
        path_folder,
        mesh_index,
        primitive_index
      );
      assert(buf_len >= 0);
      assert(buf_len < 1024);
      common::mesh::write(buf, &builder);

      primitive_index++;
    }
    mesh_index++;
  }

  { ZoneScopedN("group");
    char buf[1024];
    auto buf_len = snprintf(buf, 1024, "%s/index.grup", path_folder);
    assert(buf_len >= 0);
    assert(buf_len < 1024);

    auto file = fopen(buf, "wb");
    assert(file != nullptr);
    lib::io::magic::write(file, "GRUP");
    fwrite(&GRUP_VERSION, 1, sizeof(GRUP_VERSION), file);
    lib::io::string::write_c(file, path_folder);
       
    uint32_t item_count = 0;
    for (auto &mesh : model.meshes) {
      item_count += mesh.primitives.size();
    }
    fwrite(&item_count, 1, sizeof(item_count), file);

    for (size_t m = 0; m < model.meshes.size(); m++) {
      auto mesh = &model.meshes[m];
      for (size_t p = 0; p < mesh->primitives.size(); p++) {
        char buf[1024];
        auto buf_len = snprintf(
          buf,
          1024,
          "%s/m%02zu_p%03zu.t06",
          path_folder,
          m,
          p
        );
        assert(buf_len >= 0);
        assert(buf_len < 1024);
        lib::io::string::write_c(file, buf, buf_len);
        /*
        lib::io::string::write_c(file, nullptr); // albedo
        lib::io::string::write_c(file, nullptr); // normal
        lib::io::string::write_c(file, nullptr); // romeao
        */
        lib::io::string::write_c(file, "assets/texture-1px/albedo.png"); // albedo
        lib::io::string::write_c(file, "assets/texture-1px/normal.png"); // normal
        lib::io::string::write_c(file, "assets/texture-1px/romeao.png"); // romeao
      }
    }

    assert(ferror(file) == 0);
    fclose(file);
  }
}

} // namespace