#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <tiny_gltf.h>
#include <stb_image_write.h>
#include <src/global.hxx>
#include <src/lib/io.hxx>
#include <src/lib/mkdir_p.hxx>
#include "common/mesh.hxx"

const uint8_t GRUP_VERSION = 1;

namespace tools {

void traverse_nodes(
  tinygltf::Model *model,
  glm::mat4 *transform,
  FILE *file,
  char const *path_folder,
  uint32_t *inout_item_count,
  std::vector<int> *nodes
) {
  for (auto n : *nodes) {
    auto node = model->nodes[n];

    assert(node.matrix.size() == 0);
    glm::mat4 transform_node(1.0f);
    if (node.translation.size() == 3) {
      transform_node *= glm::translate(
        glm::mat4(1.0f),
        -glm::vec3(
          *(glm::dvec3 *) node.translation.data()
        )
      );
    }
    if (node.rotation.size() == 4) {
      transform_node *= glm::mat4_cast(
        glm::quat(
          *(glm::dquat *) node.rotation.data()
        )
      );
    }
    if (node.scale.size() == 3) {
      assert(node.scale[0] > 0.0);
      assert(node.scale[1] > 0.0);
      assert(node.scale[2] > 0.0);
      transform_node *= glm::scale(
        glm::mat4(1.0f),
        glm::vec3(
          *(glm::dvec3 *) node.scale.data()
        )
      );
    }

    glm::mat4 transform_inner = (*transform) * transform_node; 

    if (node.children.size() > 0) {
      traverse_nodes(
        model, &transform_inner, file, path_folder,
        inout_item_count,
        &node.children
      );
    }

    if (node.mesh != -1) {
      char buf[1024];
      auto mesh = &model->meshes[node.mesh];
      for (size_t p = 0; p < mesh->primitives.size(); p++) {
        (*inout_item_count)++;

        auto primitive = &mesh->primitives[p];
        
        fwrite(&transform_inner, 1, sizeof(glm::mat4), file);
        {
          auto buf_len = snprintf(
            buf,
            sizeof(buf),
            "%s/m%02zu_p%03zu.t06",
            path_folder,
            size_t(node.mesh),
            p
          );
          assert(buf_len >= 0);
          assert(buf_len < sizeof(buf));
          lib::io::string::write_c(file, buf, buf_len);
        }

        if (primitive->material == -1) {
          lib::io::string::write_c(file, "assets/texture-1px/albedo.png");
          lib::io::string::write_c(file, "assets/texture-1px/normal.png");
          lib::io::string::write_c(file, "assets/texture-1px/romeao.png");
        } else {
          auto material = &model->materials[primitive->material];
          { // albedo
            // @Incomplete: baseColorFactor

            auto texture_index = material->pbrMetallicRoughness.baseColorTexture.index;
            if (texture_index == -1) {
              lib::io::string::write_c(file, "assets/texture-1px/albedo.png");
            } else {
              assert(material->pbrMetallicRoughness.baseColorTexture.texCoord == 0);
              auto buf_len = snprintf(
                buf,
                sizeof(buf),
                "%s/%02zu.png",
                path_folder,
                size_t(model->textures[texture_index].source)
              );
              assert(buf_len >= 0);
              assert(buf_len < sizeof(buf));
              lib::io::string::write_c(file, buf, buf_len);
            }
          }
          
          { // normal
            auto texture_index = material->normalTexture.index;
            if (texture_index == -1) {
              lib::io::string::write_c(file, "assets/texture-1px/normal.png");
            } else {
              assert(material->normalTexture.texCoord == 0);
              auto buf_len = snprintf(
                buf,
                sizeof(buf),
                "%s/%02zu.png",
                path_folder,
                size_t(model->textures[texture_index].source)
              );
              assert(buf_len >= 0);
              assert(buf_len < sizeof(buf));
              lib::io::string::write_c(file, buf, buf_len);
            }
          }

          { // romeao
            // @Incomplete: roughnessFactor, metallicFactor

            auto texture_index = material->pbrMetallicRoughness.metallicRoughnessTexture.index;
            if (texture_index == -1) {
              lib::io::string::write_c(file, "assets/texture-1px/romeao.png");
            } else {
              assert(material->pbrMetallicRoughness.metallicRoughnessTexture.texCoord == 0);
              auto buf_len = snprintf(
                buf,
                sizeof(buf),
                "%s/%02zu.png",
                path_folder,
                size_t(model->textures[texture_index].source)
              );
              assert(buf_len >= 0);
              assert(buf_len < sizeof(buf));
              lib::io::string::write_c(file, buf, buf_len);
            }
          }
        }
      }
    }
  }
}

void gltf_converter(
  char const *path_gltf,
  char const *path_folder
) {
  ZoneScoped;

  tinygltf::Model model;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;
  char buf[1024];

  bool ok = loader.LoadASCIIFromFile(&model, &err, &warn, path_gltf);
  assert(ok);

  if (!warn.empty()) {
    LOG("gltf_converter warning: {}", warn.c_str());
  }

  if (!err.empty()) {
    LOG("gltf_converter error: {}", err.c_str());
  }

  std::unordered_set<size_t> mero_set;

  assert(model.meshes.size() > 0);
  size_t mesh_index = 0;
  for (auto &mesh : model.meshes) {
    size_t primitive_index = 0;
    for (auto &primitive : mesh.primitives) {
      common::mesh::T06_Builder builder;
      assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);

      if (primitive.material != -1) {
        auto material = &model.materials[primitive.material];
        auto texture_index = material->pbrMetallicRoughness.metallicRoughnessTexture.index;
        if (texture_index != -1) {
          mero_set.insert(
            model.textures[texture_index].source
          );
        }
      }

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
        auto accessor_texcoord = &model.accessors[primitive.attributes["TEXCOORD_0"]];
        auto accessor_normal = &model.accessors[primitive.attributes["NORMAL"]];
        auto accessor_tangent = (primitive.attributes.contains("TANGENT")
          ? &model.accessors[primitive.attributes["TANGENT"]]
          : &model.accessors[primitive.attributes["NORMAL"]] // ????? @Think
        );

        auto count = accessor_position->count;
        assert(accessor_position->count == accessor_texcoord->count);
        assert(accessor_position->count == accessor_normal->count);
        assert(accessor_position->count == accessor_tangent->count);

        assert(accessor_position->type == TINYGLTF_TYPE_VEC3);
        assert(accessor_position->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        auto buffer_view_position = &model.bufferViews[accessor_position->bufferView];
        auto buffer_position = &model.buffers[buffer_view_position->buffer];

        assert(accessor_texcoord->type == TINYGLTF_TYPE_VEC2);
        assert(accessor_texcoord->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        auto buffer_view_texcoord = &model.bufferViews[accessor_texcoord->bufferView];
        auto buffer_texcoord = &model.buffers[buffer_view_texcoord->buffer];

        assert(accessor_normal->type == TINYGLTF_TYPE_VEC3);
        assert(accessor_normal->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        auto buffer_view_normal = &model.bufferViews[accessor_normal->bufferView];
        auto buffer_normal = &model.buffers[buffer_view_normal->buffer];

        assert(0
          || accessor_tangent->type == TINYGLTF_TYPE_VEC3
          || accessor_tangent->type == TINYGLTF_TYPE_VEC4
        );
        assert(accessor_tangent->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        auto buffer_view_tangent = &model.bufferViews[accessor_tangent->bufferView];
        auto buffer_tangent = &model.buffers[buffer_view_tangent->buffer];
        
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

          auto stride_texcoord = (
            buffer_view_texcoord->byteStride > 0
              ? buffer_view_texcoord->byteStride
              : sizeof(glm::vec2)
          );
          auto offset_texcoord = (0
            + accessor_texcoord->byteOffset
            + buffer_view_texcoord->byteOffset
            + i * stride_texcoord
          );
          assert(offset_texcoord + sizeof(glm::vec2) <= buffer_texcoord->data.size());

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

          auto stride_tangent = (
            buffer_view_tangent->byteStride > 0
              ? buffer_view_tangent->byteStride
              : accessor_tangent->type == TINYGLTF_TYPE_VEC3
                ? sizeof(glm::vec3)
                : sizeof(glm::vec4)
          );
          auto offset_tangent = (0
            + accessor_tangent->byteOffset
            + buffer_view_tangent->byteOffset
            + i * stride_tangent
          );
          assert(offset_tangent + sizeof(glm::vec3) <= buffer_tangent->data.size());

          auto normal = *((glm::vec3 *) (&buffer_normal->data[offset_normal]));
          auto tangent = *((glm::vec3 *) (&buffer_tangent->data[offset_tangent]));

          engine::common::mesh::VertexT06 vertex = {
            .position = *((glm::vec3 *) (&buffer_position->data[offset_position])),
            .tangent = tangent,
            .bitangent = glm::cross(normal, tangent),
            .normal = normal,
            .uv = *((glm::vec2 *) (&buffer_texcoord->data[offset_texcoord])),
          };
          float epsilon = 1e-3;
          assert(vertex.position.x + epsilon >= accessor_position->minValues[0]);
          assert(vertex.position.y + epsilon >= accessor_position->minValues[1]);
          assert(vertex.position.z + epsilon >= accessor_position->minValues[2]);
          assert(vertex.position.x - epsilon <= accessor_position->maxValues[0]);
          assert(vertex.position.y - epsilon <= accessor_position->maxValues[1]);
          assert(vertex.position.z - epsilon <= accessor_position->maxValues[2]);

          vertex.position.z = -vertex.position.z;
          // @Hack: this is ugly. This converter had a `flip` scale matrix
          // applied in the final transform before, presumably because the
          // handedness of the GLTF coordinate system differed from ours.
          // That caused problems with Vulkan RT determining triangle facing in
          // object space, so we just flip the vertices here instead.

          builder.vertices.push_back(vertex);
        }

        assert(max_index == builder.vertices.size() - 1);
      }

      auto buf_len = snprintf(
        buf,
        sizeof(buf),
        "%s/m%02zu_p%03zu.t06",
        path_folder,
        mesh_index,
        primitive_index
      );
      assert(buf_len >= 0);
      assert(buf_len < sizeof(buf));
      common::mesh::write(buf, &builder);

      primitive_index++;
    }
    mesh_index++;
  }

  size_t image_index = 0;
  for (auto &image_ : model.images) {
    auto image = &image_;
    assert(image->component == 4);
    assert(image->bits == 8);
    assert(image->pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);

    if (mero_set.contains(image_index)) {
      // mutate in-place
      for (size_t i = 0; i < image->width * image->height; i++) {
        auto ix0 = i * image->component;
        image->image[ix0 + 0] = image->image[ix0 + 1];
        image->image[ix0 + 1] = image->image[ix0 + 2];
        image->image[ix0 + 2] = 0;
        image->image[ix0 + 3] = 255;
      }
    }

    auto buf_len = snprintf(buf, sizeof(buf), "%s/%02zu.png", path_folder, image_index);
    assert(buf_len >= 0);
    assert(buf_len < sizeof(buf));
    auto result = stbi_write_png(
      buf,
      image->width,
      image->height,
      image->component,
      image->image.data(),
      0
    );
    assert(result != 0);

    image_index++;
  }

  { ZoneScopedN("directory");
    auto result = mkdir_p(path_folder);
    assert(result == 0);
  }

  // @Incomplete: scale constant should be set externally.
  float scale = 1.0;

  { ZoneScopedN("group");
    auto buf_len = snprintf(buf, sizeof(buf), "%s/index.grup", path_folder);
    assert(buf_len >= 0);
    assert(buf_len < sizeof(buf));

    auto file = fopen(buf, "wb");
    assert(file != nullptr);
    lib::io::magic::write(file, "GRUP");
    fwrite(&GRUP_VERSION, 1, sizeof(GRUP_VERSION), file);
    lib::io::string::write_c(file, path_folder);
       
    auto item_count_pos = ftell(file);
    uint32_t item_count = 0; // modified and re-written later
    fwrite(&item_count, 1, sizeof(item_count), file);

    assert(model.scenes.size() == 1);
    auto nodes = &model.scenes[0].nodes;
    glm::mat4 transform_global = glm::rotate(
      glm::scale(
        glm::mat4(1.0f),
        glm::vec3(1.0f, 1.0f, 1.0f) * scale
      ),
      glm::radians(90.0f),
      glm::vec3(1.0f, 0.0f, 0.0f)
    );
    traverse_nodes(
      &model, &transform_global, file, path_folder,
      &item_count,
      nodes
    );

    fseek(file, item_count_pos, SEEK_SET);
    fwrite(&item_count, 1, sizeof(item_count), file);
    assert(ferror(file) == 0);
    fclose(file);
  }
}

} // namespace