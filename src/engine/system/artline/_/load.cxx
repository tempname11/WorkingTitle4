#include <atomic>
#include <Windows.h>
#include <stb_image.h>
#include <src/lib/lifetime.hxx>
#include <src/lib/defer.hxx>
#include <src/lib/gfx/utilities.hxx>
#include <src/engine/common/texture.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/system/artline/public.hxx>
#include <src/engine/system/grup/mesh/common.hxx>
#include "private.hxx"
#include "../public.hxx"

namespace engine::system::artline {

struct CountdownData {
  std::atomic<size_t> value;
  lib::Task *yarn_zero;
};

struct DensityMeshData {
  lib::GUID mesh_id;
  DensityFn *density_fn;
  engine::common::mesh::T06 t06;
  engine::session::Vulkan::Meshes::Item mesh_item;
  CountdownData *countdown;
};

struct FileMeshData {
  lib::GUID mesh_id;
  std::string path;
  engine::common::mesh::T06 t06;
  engine::session::Vulkan::Meshes::Item mesh_item;
  CountdownData *countdown;
};

void _generate_mesh(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<DensityMeshData> data 
) {
  ZoneScoped;
  data->t06 = generate(data->density_fn);
}

void _read_mesh_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<FileMeshData> data 
) {
  ZoneScoped;
  data->t06 = grup::mesh::read_t06_file(data->path.c_str());
}

void _finish_density_mesh(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<DensityMeshData> data,
  Own<engine::session::Vulkan::Meshes> meshes,
  Ref<engine::session::Data> session
) {
  // @CopyPaste
  ZoneScoped;
  meshes->items.insert({ data->mesh_id, data->mesh_item });
  grup::mesh::deinit_t06(&data->t06);
  auto old_countdown = data->countdown->value.fetch_sub(1);
  assert(old_countdown > 0);
  if (old_countdown == 1) {
    lib::task::signal(ctx->runner, data->countdown->yarn_zero);
    delete data->countdown;
  }
  delete data.ptr;
}

void _finish_file_mesh(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<FileMeshData> data,
  Own<engine::session::Vulkan::Meshes> meshes,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  meshes->items.insert({ data->mesh_id, data->mesh_item });
  grup::mesh::deinit_t06(&data->t06);
  auto old_countdown = data->countdown->value.fetch_sub(1);
  assert(old_countdown > 0);
  if (old_countdown == 1) {
    lib::task::signal(ctx->runner, data->countdown->yarn_zero);
    delete data->countdown;
  }
  delete data.ptr;
}

void _add_density_mesh_tasks(
  lib::GUID mesh_id,
  ModelMesh::Density *density,
  CountdownData *countdown,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  auto data = new DensityMeshData {
    .mesh_id = mesh_id,
    .density_fn = *density->density_fn,
    .countdown = countdown,
  };

  auto task_generate = lib::task::create(
    _generate_mesh,
    data
  );

  // @CopyPaste
  auto signal_init_buffer = lib::task::create_external_signal();
  auto task_init_buffer = lib::defer(
    lib::task::create(
      _upload_mesh_init_buffer,
      &data->t06,
      &data->mesh_item,
      signal_init_buffer,
      &session->vulkan.queue_work,
      session.ptr
    )
  );
  auto signal_init_blas = lib::task::create_external_signal();
  auto task_init_blas = lib::defer(
    lib::task::create(
      _upload_mesh_init_blas,
      &data->mesh_item,
      signal_init_blas,
      &session->vulkan.queue_work,
      session.ptr
    )
  );

  auto task_finish_mesh = lib::defer(
    lib::task::create(
      _finish_density_mesh,
      data,
      &session->vulkan.meshes,
      session.ptr
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_generate,
    task_init_buffer.first,
    task_init_blas.first,
    task_finish_mesh.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_generate, task_init_buffer.first },
    { signal_init_buffer, task_init_blas.first },
    { signal_init_blas, task_finish_mesh.first },
  });

  countdown->value++;
}

void _add_file_mesh_tasks(
  lib::GUID mesh_id,
  ModelMesh::File *file,
  CountdownData *countdown,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  auto data = new FileMeshData {
    .mesh_id = mesh_id,
    .path = *file->filename,
    .countdown = countdown,
  };
  auto task_read_file = lib::task::create(
    _read_mesh_file,
    data
  );

  auto signal_init_buffer = lib::task::create_external_signal();
  auto task_init_buffer = lib::defer(
    lib::task::create(
      _upload_mesh_init_buffer,
      &data->t06,
      &data->mesh_item,
      signal_init_buffer,
      &session->vulkan.queue_work,
      session.ptr
    )
  );
  auto signal_init_blas = lib::task::create_external_signal();
  auto task_init_blas = lib::defer(
    lib::task::create(
      _upload_mesh_init_blas,
      &data->mesh_item,
      signal_init_blas,
      &session->vulkan.queue_work,
      session.ptr
    )
  );

  auto task_finish_mesh = lib::defer(
    lib::task::create(
      _finish_file_mesh,
      data,
      &session->vulkan.meshes,
      session.ptr
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_read_file,
    task_init_buffer.first,
    task_init_blas.first,
    task_finish_mesh.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_file, task_init_buffer.first },
    { signal_init_buffer, task_init_blas.first },
    { signal_init_blas, task_finish_mesh.first },
  });

  countdown->value++;
}

struct LoadTextureData {
  lib::GUID texture_id;
  std::string path;
  VkFormat format;
  engine::common::texture::Data<uint8_t> data;
  engine::session::Vulkan::Textures::Item texture_item;
  CountdownData *countdown;
};

void _read_texture_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadTextureData> data 
) {
  ZoneScoped;
  int width, height, _channels = 4;
  auto loaded = stbi_load(data->path.c_str(), &width, &height, &_channels, 4);
  if (loaded == nullptr) {
    return;
  };
  data->data = engine::common::texture::Data<uint8_t> {
    .data = loaded,
    .width = width,
    .height = height,
    .channels = 4,
    .computed_mip_levels = lib::gfx::utilities::mip_levels(width, height),
  };
}

void _finish_texture(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadTextureData> data,
  Own<engine::session::Vulkan::Textures> textures,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  textures->items.insert({ data->texture_id, data->texture_item });
  stbi_image_free(data->data.data);
  auto old_countdown = data->countdown->value.fetch_sub(1);
  assert(old_countdown > 0);
  if (old_countdown == 1) {
    lib::task::signal(ctx->runner, data->countdown->yarn_zero);
    delete data->countdown;
  }
  delete data.ptr;
}

struct FinishData {
  HINSTANCE h_lib;
  std::vector<session::Data::Scene::Item> items_to_add;
};

void _finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadData> data,
  Ref<FinishData> finish_data,
  Own<engine::session::Data::Scene> scene,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  {
    auto it = &session->artline;
    std::unique_lock lock(it->rw_mutex);
    auto item = &it->dlls[data->dll_id];
    item->status = Status::Ready;
  }

  scene->items.insert(
    scene->items.end(),
    finish_data->items_to_add.begin(),
    finish_data->items_to_add.end()
  );

  if (data->yarn_end != nullptr) {
    lib::task::signal(ctx->runner, data->yarn_end);
  }

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  {
    auto result = FreeLibrary(finish_data->h_lib);
    assert(result != 0);
  }

  delete data.ptr;
  delete finish_data.ptr;
}

void _add_texture_tasks(
  lib::GUID texture_id,
  VkFormat format,
  std::string *path,
  CountdownData *countdown,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  auto data = new LoadTextureData {
    .texture_id = texture_id,
    .path = *path,
    .format = format,
    .countdown = countdown,
  };
  auto task_read_file = lib::task::create( // @Tmp
    _read_texture_file,
    data
  );

  auto signal_init_buffer = lib::task::create_external_signal();
  auto task_init_buffer = lib::defer(
    lib::task::create(
      _upload_texture,
      &data->format,
      &data->data,
      &data->texture_item,
      signal_init_buffer,
      &session->vulkan.queue_work,
      session.ptr
    )
  );
  auto task_finish_texture = lib::defer(
    lib::task::create(
      _finish_texture,
      data,
      &session->vulkan.textures,
      session.ptr
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_read_file,
    task_init_buffer.first,
    task_finish_texture.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_file, task_init_buffer.first },
    { signal_init_buffer, task_finish_texture.first },
  });

  countdown->value++;
}

void _load(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadData> data,
  Ref<engine::session::Data> session
) {
  ZoneScoped;

  auto finish_data = new FinishData {};
  engine::system::artline::Description desc;
  {
    HINSTANCE h_lib = LoadLibrary(data->dll_filename.c_str());
    assert(h_lib != nullptr);

    auto describe_fn = (DescribeFn *) GetProcAddress(h_lib, "describe");
    assert(describe_fn != nullptr);

    describe_fn(&desc);

    finish_data->h_lib = h_lib;
  }

  auto countdown = new CountdownData {
    .yarn_zero = lib::task::create_yarn_signal(),
  };

  {
    auto it = &session->artline;
    std::unique_lock lock(it->rw_mutex);

    for (auto &model : desc.models) {
      lib::GUID mesh_id = 0;
      switch(model.mesh.type) {
        case ModelMesh::Type::File: {
          ModelMesh::File *file = &model.mesh.file;
          auto key = *file->filename;
          if (it->meshes_by_key.contains(key)) {
            mesh_id = it->meshes_by_key.at(key);
            it->meshes[mesh_id].ref_count++;
          } else {
            mesh_id = lib::guid::next(&session->guid_counter);
            it->meshes_by_key[model.filename_albedo] = mesh_id;
            it->meshes[mesh_id] = MeshInfo {
              .ref_count = 1,
              .key = key,
            };

            _add_file_mesh_tasks(
              mesh_id,
              &model.mesh.file,
              countdown,
              session,
              ctx
            );
          }
          break;
        }
        case ModelMesh::Type::Density: {
          ModelMesh::Density *density = &model.mesh.density;
          std::string key = "tmp"; // @Tmp
          if (it->meshes_by_key.contains(key)) {
            mesh_id = it->meshes_by_key.at(key);
            it->meshes[mesh_id].ref_count++;
          } else {
            mesh_id = lib::guid::next(&session->guid_counter);
            it->meshes_by_key[model.filename_albedo] = mesh_id;
            it->meshes[mesh_id] = MeshInfo {
              .ref_count = 1,
              .key = key,
            };

            _add_density_mesh_tasks(
              mesh_id,
              &model.mesh.density,
              countdown,
              session,
              ctx
            );
          }
          break;
        }
      }
      assert(mesh_id != 0);

      lib::GUID texture_albedo_id = 0;
      if (it->textures_by_key.contains(model.filename_albedo)) {
        texture_albedo_id = it->textures_by_key.at(model.filename_albedo);
        it->textures[texture_albedo_id].ref_count++;
      } else {
        texture_albedo_id = lib::guid::next(&session->guid_counter);
        it->textures_by_key[model.filename_albedo] = texture_albedo_id;
        it->textures[texture_albedo_id] = TextureInfo {
          .ref_count = 1,
          .key = model.filename_albedo,
        };

        _add_texture_tasks(
          texture_albedo_id,
          engine::common::texture::ALBEDO_TEXTURE_FORMAT,
          &model.filename_albedo,
          countdown,
          session,
          ctx
        );
      }
      assert(texture_albedo_id != 0);

      lib::GUID texture_normal_id = 0;
      if (it->textures_by_key.contains(model.filename_normal)) {
        texture_normal_id = it->textures_by_key.at(model.filename_normal);
        it->textures[texture_normal_id].ref_count++;
      } else {
        texture_normal_id = lib::guid::next(&session->guid_counter);
        it->textures_by_key[model.filename_normal] = texture_normal_id;
        it->textures[texture_normal_id] = TextureInfo {
          .ref_count = 1,
          .key = model.filename_normal,
        };

        _add_texture_tasks(
          texture_normal_id,
          engine::common::texture::ALBEDO_TEXTURE_FORMAT,
          &model.filename_normal,
          countdown,
          session,
          ctx
        );
      }
      assert(texture_normal_id != 0);

      lib::GUID texture_romeao_id = 0;
      if (it->textures_by_key.contains(model.filename_romeao)) {
        texture_romeao_id = it->textures_by_key.at(model.filename_romeao);
        it->textures[texture_romeao_id].ref_count++;
      } else {
        texture_romeao_id = lib::guid::next(&session->guid_counter);
        it->textures_by_key[model.filename_romeao] = texture_romeao_id;
        it->textures[texture_romeao_id] = TextureInfo {
          .ref_count = 1,
          .key = model.filename_romeao,
        };

        _add_texture_tasks(
          texture_romeao_id,
          engine::common::texture::ALBEDO_TEXTURE_FORMAT,
          &model.filename_romeao,
          countdown,
          session,
          ctx
        );
      }
      assert(texture_romeao_id != 0);

      finish_data->items_to_add.push_back(session::Data::Scene::Item {
        .owner_id = data->dll_id,
        .transform = model.transform,
        .mesh_id = mesh_id,
        .texture_albedo_id = texture_albedo_id,
        .texture_normal_id = texture_normal_id,
        .texture_romeao_id = texture_romeao_id,
      });
    }
  }

  auto task_finish = lib::defer(
    lib::task::create(
      _finish,
      data.ptr,
      finish_data,
      &session->scene,
      session.ptr
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_finish.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { countdown->yarn_zero, task_finish.first },
  });
}

} // namespace
