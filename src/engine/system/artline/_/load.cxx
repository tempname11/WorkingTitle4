#include <atomic>
#include <Windows.h>
#include <stb_image.h>
#include <src/global.hxx>
#include <src/lib/easy_allocator.hxx>
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

constexpr size_t NUM_TEXTURE_TYPES = 3; // albedo, normal, romeao

struct PerLoadImpl {
  lib::Lifetime models_complete;
  HINSTANCE h_lib;
  Description desc;
};

struct PerMesh {
  session::Vulkan::Meshes::Item mesh_item;
};

struct PerTexture {
  lib::cstr_range_t file_path;
  VkFormat format;

  engine::common::texture::Data<uint8_t> data;
  session::Vulkan::Textures::Item texture_item;
  lib::GUID texture_id;
};

struct PerModel {
  lib::Lifetime complete;
  lib::u64_table::hash_t mesh_key;
  lib::array_t<lib::GUID> *mesh_ids;
  lib::array_t<PerMesh> *meshes;
  lib::array_t<PerTexture> *textures;
};

void _read_texture_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerTexture> texture 
) {
  ZoneScoped;
  int width, height, _channels = 4;
  auto loaded = stbi_load(texture->file_path.start, &width, &height, &_channels, 4);
  if (loaded == nullptr) {
    return;
  };
  texture->data = engine::common::texture::Data<uint8_t> {
    .data = loaded,
    .width = width,
    .height = height,
    .channels = 4,
    .computed_mip_levels = lib::gfx::utilities::mip_levels(width, height),
  };
}

void _finish_texture(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerTexture> texture,
  Ref<PerModel> model,
  Own<engine::session::Vulkan::Textures> textures,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  texture->texture_id = lib::guid::next(&session->guid_counter);
  textures->items.insert({ texture->texture_id, texture->texture_item });
  stbi_image_free(texture->data.data);
  lib::lifetime::deref(&model->complete, ctx->runner);
}

void _finish_mesh(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<common::mesh::T06> t06,
  Ref<PerMesh> mesh,
  Ref<lib::GUID> p_mesh_id,
  Ref<PerModel> model,
  Own<engine::session::Vulkan::Meshes> meshes,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  *p_mesh_id = lib::guid::next(&session->guid_counter);
  meshes->items.insert({ *p_mesh_id, mesh->mesh_item });
  grup::mesh::deinit_t06(t06.ptr);
  lib::lifetime::deref(&model->complete, ctx->runner);
}

void _model_end(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<Model> desc_model,
  Ref<PerModel> model,
  Ref<PerLoad> load,
  Ref<session::Data> session
) {
  ZoneScoped;

  {
    lib::mutex::lock(&load->ready.mutex);

    auto count = model->meshes->count;
    lib::array::ensure_space(&load->ready.scene_items, count);
    for (size_t i = 0; i < count; i++) {
      load->ready.scene_items->data[
        load->ready.scene_items->count++
      ] = session::Data::Scene::Item {
        .owner_id = load->dll_id,
        .transform = desc_model->transform,
        .mesh_id = model->mesh_ids->data[i],
        .texture_albedo_id = model->textures->data[0].texture_id,
        .texture_normal_id = model->textures->data[1].texture_id,
        .texture_romeao_id = model->textures->data[2].texture_id,
      };
    }

    lib::mutex::unlock(&load->ready.mutex);
  }

  if (model->mesh_key.as_number != 0) {
    auto a = &session->artline;
    lib::mutex::lock(&a->mutex);
    // insert the new mesh ids into the cache.
    lib::u64_table::insert(
      &a->meshes_by_key,
      model->mesh_key,
      CachedMesh {
        .ref_count = 1, 
        .mesh_ids = model->mesh_ids,
      }
    );
    lib::mutex::unlock(&a->mutex);
  }

  lib::lifetime::deref(&load->impl->models_complete, ctx->runner);
}

void _generate_texture(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerTexture> texture,
  Ref<PerModel> model,
  Ref<session::Data> session
) {
  ZoneScoped;
  auto a = &session->artline;
  auto key = lib::cstr::fnv_1a(texture->file_path);

  lib::GUID cached_texture_id = 0;
  {
    lib::mutex::lock(&a->mutex);
    auto hit = lib::u64_table::lookup(a->textures_by_key, key);
    if (hit == nullptr) {
      // @Bug :ConcurrentLoad
    } else {
      hit->ref_count++;
      cached_texture_id = hit->texture_id;
    }
    lib::mutex::unlock(&a->mutex);
  }

  if (cached_texture_id == 0) {
    auto task_read_file = lib::task::create(
      _read_texture_file,
      texture.ptr
    );

    auto signal_init_image = lib::task::create_external_signal();
    auto task_init_image = lib::defer(
      lib::task::create(
        _upload_texture,
        &texture->format,
        &texture->data,
        &texture->texture_item,
        signal_init_image,
        &session->vulkan.queue_work,
        session.ptr
      )
    );

    auto task_finish_texture = lib::defer(
      lib::task::create(
        _finish_texture,
        texture.ptr,
        model.ptr,
        &session->vulkan.textures,
        session.ptr
      )
    );

    ctx->new_tasks.insert(ctx->new_tasks.end(), {
      task_read_file,
      task_init_image.first,
      task_finish_texture.first,
    });

    ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
      { task_read_file, task_init_image.first },
      { signal_init_image, task_finish_texture.first },
    });
  } else {
    texture->texture_id = cached_texture_id;
    lib::lifetime::deref(&model->complete, ctx->runner);
  }
}

void _generate_meshes(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<Model> desc_model,
  Ref<PerModel> model,
  Ref<PerLoad> load,
  Ref<session::Data> session
) {
  lib::array_t<lib::GUID> *cached_mesh_ids = nullptr;
  lib::array_t<common::mesh::T06> *t06_meshes = nullptr;
  auto a = &session->artline;
  switch (desc_model->mesh.type) {
    case ModelMesh::Type::File: {
      auto it = &desc_model->mesh.file;
      auto key = lib::cstr::fnv_1a(it->path);

      {
        lib::mutex::lock(&a->mutex);
        auto hit = lib::u64_table::lookup(a->meshes_by_key, key);
        if (hit == nullptr) {
          // @Bug :ConcurrentLoad
        } else {
          hit->ref_count++;
          cached_mesh_ids = hit->mesh_ids;
        }
        lib::mutex::unlock(&a->mutex);
      }

      if (cached_mesh_ids == nullptr) {
        model->mesh_key = key;
        t06_meshes = lib::array::create<common::mesh::T06>(load->misc, 1);
        t06_meshes->data[t06_meshes->count++] = (
          grup::mesh::read_t06_file(it->path.start)
        );
      }
      break;
    }
    case ModelMesh::Type::Density: {
      auto it = &desc_model->mesh.density;
      auto key = lib::u64_table::from_u64(it->signature); // @Incomplete: should also use `dll_id`.

      {
        lib::mutex::lock(&a->mutex);
        auto hit = lib::u64_table::lookup(a->meshes_by_key, key);
        if (hit == nullptr) {
          // @Bug :ConcurrentLoad
        } else {
          hit->ref_count++;
          cached_mesh_ids = hit->mesh_ids;
        }
        lib::mutex::unlock(&a->mutex);
      }
      
      if (cached_mesh_ids == nullptr) {
        model->mesh_key = key;
        t06_meshes = generate(load->misc, it->fn);
      }

      break;
    }
    default: {
      assert("Unknown mesh type" && false);
    }
  }
    
  if (cached_mesh_ids != nullptr) {
    model->mesh_ids = cached_mesh_ids; // ref_count will keep it alive for us
  }

  if (t06_meshes != nullptr) {
    model->mesh_ids = lib::array::create<lib::GUID>(
      // CRT allocator is important, because this array
      // will go into the cache and needs long-term lifetime.
      lib::allocator::crt,
      t06_meshes->count
    );
    model->mesh_ids->count = model->mesh_ids->capacity;

    assert(model->meshes->count == 0);
    lib::array::ensure_space(&model->meshes, t06_meshes->count);
    model->meshes->count = model->meshes->capacity;

    for (size_t i = 0; i < t06_meshes->count; i++) {
      lib::lifetime::ref(&model->complete);
      auto signal_init_buffer = lib::task::create_external_signal();
      auto task_init_buffer = lib::defer(
        lib::task::create(
          _upload_mesh_init_buffer,
          &t06_meshes->data[i],
          &model->meshes->data[i].mesh_item,
          signal_init_buffer,
          &session->vulkan.queue_work,
          session.ptr
        )
      );
      auto signal_init_blas = lib::task::create_external_signal();
      auto task_init_blas = lib::defer(
        lib::task::create(
          _upload_mesh_init_blas,
          &model->meshes->data[i].mesh_item,
          signal_init_blas,
          &session->vulkan.queue_work,
          session.ptr
        )
      );

      auto task_finish_mesh = lib::defer(
        lib::task::create(
          _finish_mesh,
          &t06_meshes->data[i],
          &model->meshes->data[i],
          &model->mesh_ids->data[i],
          model.ptr,
          &session->vulkan.meshes,
          session.ptr
        )
      );

      ctx->new_tasks.insert(ctx->new_tasks.end(), {
        task_init_buffer.first,
        task_init_blas.first,
      });

      ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
        { signal_init_buffer, task_init_blas.first },
        { signal_init_blas, task_finish_mesh.first },
      });
    }
  }

  lib::lifetime::deref(&model->complete, ctx->runner);
}

void _begin_model(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<Model> desc_model,
  Ref<PerLoad> load,
  Ref<session::Data> session
) {
  ZoneScoped;

  auto model = lib::allocator::make<PerModel>(load->misc);
  *model = {
    .meshes = lib::array::create<PerMesh>(
      load->misc,
      0
    ),
    .textures = lib::array::create<PerTexture>(
      load->misc,
      NUM_TEXTURE_TYPES
    ),
  };
  lib::lifetime::init(&model->complete, 1 + 3); // meshes + 3 textures

  model->textures->count = model->textures->capacity;

  auto task_generate_meshes = lib::task::create(
    _generate_meshes,
    desc_model.ptr,
    model,
    load.ptr,
    session.ptr
  );

  auto task_model_end = lib::task::create(
    _model_end,
    desc_model.ptr,
    model,
    load.ptr,
    session.ptr
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_generate_meshes,
    task_model_end,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { model->complete.yarn, task_model_end },
  });

  model->textures->data[0] = {
    .file_path = desc_model->file_path_albedo,
    .format = engine::common::texture::ALBEDO_TEXTURE_FORMAT,
  };
  model->textures->data[1] = {
    .file_path = desc_model->file_path_normal,
    .format = engine::common::texture::NORMAL_TEXTURE_FORMAT,
  };
  model->textures->data[2] = {
    .file_path = desc_model->file_path_romeao,
    .format = engine::common::texture::ROMEAO_TEXTURE_FORMAT,
  };

  for (size_t i = 0; i < NUM_TEXTURE_TYPES; i++) {
    auto task_generate_texture = lib::task::create(
      _generate_texture,
      &model->textures->data[i],
      model,
      session.ptr
    );

    ctx->new_tasks.insert(ctx->new_tasks.end(), {
      task_generate_texture,
    });
  }
}

void _finish_dll(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerLoad> load,
  Own<session::Data::Scene> scene,
  Ref<session::Data> session
) {
  ZoneScoped;

  {
    auto result = FreeLibrary(load->impl->h_lib);
    assert(result != 0);
  }

  if (load->yarn_done != nullptr) {
    lib::task::signal(ctx->runner, load->yarn_done);
  }
}

void _load_dll(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerLoad> load,
  Ref<session::Data> session
) {
  ZoneScoped;

  auto impl = lib::allocator::make<PerLoadImpl>(load->misc);
  *impl = {};
  lib::lifetime::init(&impl->models_complete, 0);
  impl->desc.models = lib::array::create<Model>(load->misc, 0);
  
  load->impl = impl;

  {
    HINSTANCE h_lib = LoadLibrary(load->dll_filename.start);
    assert(h_lib != nullptr);

    auto describe_fn = (DescribeFn *) GetProcAddress(h_lib, "describe");
    assert(describe_fn != nullptr);
    describe_fn(&impl->desc);

    impl->h_lib = h_lib;
  }

  for (size_t i = 0; i < impl->desc.models->count; i++) {
    lib::lifetime::ref(&impl->models_complete);
    auto model = &impl->desc.models->data[i];

    auto task_begin = lib::task::create(
      _begin_model,
      model,
      load.ptr,
      session.ptr
    );

    ctx->new_tasks.insert(ctx->new_tasks.end(), {
      task_begin,
    });
  }

  auto task_finish = lib::defer(
    lib::task::create(
      _finish_dll,
      load.ptr,
      &session->scene,
      session.ptr
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_finish.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { impl->models_complete.yarn, task_finish.first },
  });
}

} // namespace
