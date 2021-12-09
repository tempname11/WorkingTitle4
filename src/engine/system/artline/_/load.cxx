#include <atomic>
#include <Windows.h>
#include <stb_image.h>
#include <src/global.hxx>
#include <src/lib/easy_allocator.hxx>
#include <src/lib/lifetime.hxx>
#include <src/lib/defer.hxx>
#include <src/lib/gfx/utilities.hxx>
#include <src/engine/common/t06.hxx>
#include <src/engine/common/texture.hxx>
#include <src/engine/session/data.hxx>
#include <src/engine/session/data/vulkan.hxx>
#include <src/engine/system/artline/public.hxx>
#include "private.hxx"
#include "../public.hxx"

namespace engine::system::artline {

constexpr size_t NUM_TEXTURE_TYPES = 3; // albedo, normal, romeao

struct PerLoadImpl {
  lib::Lifetime pieces_complete;
  HINSTANCE h_lib;
  Description desc;
};

struct PerMesh {
  session::VulkanData::Meshes::Item mesh_item;
};

struct PerTexture {
  lib::cstr_range_t file_path;
  VkFormat format;

  lib::GUID texture_id;
  engine::common::texture::Data<uint8_t> data;
  session::VulkanData::Textures::Item texture_item;
  lib::hash64_t key;
};

struct PerPiece {
  lib::Lifetime complete;
  lib::array_t<lib::GUID> *mesh_ids;
  lib::array_t<PerMesh> *meshes;
  lib::array_t<PerTexture> *textures;
  lib::hash64_t mesh_key;
};

void _read_texture_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerTexture> texture 
) {
  ZoneScoped;
  int width, height, _channels = 4;
  auto loaded = stbi_load(texture->file_path.start, &width, &height, &_channels, 4);
  if (loaded == nullptr) {
    texture->data = {};
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
  Ref<PerPiece> piece,
  Own<engine::session::VulkanData::Textures> textures,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  textures->items.insert({ texture->texture_id, texture->texture_item });

  {
    auto a = &session->artline;
    lib::mutex::lock(&a->mutex);
    auto cached = lib::u64_table::lookup(
      a->textures_by_key,
      texture->key
    );
    assert(cached != nullptr);
    cached->pending = nullptr;
    lib::mutex::unlock(&a->mutex);
  }

  lib::lifetime::deref(&piece->complete, ctx->runner);
  stbi_image_free(texture->data.data);
}

void _finish_mesh(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<common::mesh::T06> t06,
  Ref<PerMesh> mesh,
  Ref<lib::GUID> p_mesh_id,
  Ref<PerPiece> piece,
  Own<engine::session::VulkanData::Meshes> meshes,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  *p_mesh_id = lib::guid::next(session->guid_counter);
  meshes->items.insert({ *p_mesh_id, mesh->mesh_item });
  common::t06::destroy(t06.ptr);
}

void _piece_end(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<Piece> desc_piece,
  Ref<PerPiece> piece,
  Ref<PerLoad> load,
  Ref<session::Data> session
) {
  ZoneScoped;

  {
    lib::mutex::lock(&load->ready.mutex);

    auto count = piece->mesh_ids->count;
    lib::array::ensure_space(&load->ready.model_parts, count);
    for (size_t i = 0; i < count; i++) {
      load->ready.model_parts->data[
        load->ready.model_parts->count++
      ] = ModelPart {
        .transform = desc_piece->transform,
        .mesh_id = piece->mesh_ids->data[i],
        .texture_albedo_id = piece->textures->data[0].texture_id,
        .texture_normal_id = piece->textures->data[1].texture_id,
        .texture_romeao_id = piece->textures->data[2].texture_id,
      };
    }

    lib::mutex::unlock(&load->ready.mutex);
  }

  {
    auto a = &session->artline;
    lib::mutex::lock(&a->mutex);

    auto cached = lib::u64_table::lookup(
      a->meshes_by_key,
      piece->mesh_key
    );
    assert(cached != nullptr);
    cached->mesh_ids = piece->mesh_ids;
    lib::mutex::unlock(&a->mutex);
  }

  lib::lifetime::deref(&load->impl->pieces_complete, ctx->runner);
}

void _after_pending_texture(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerPiece> piece
) {
  ZoneScoped;
  lib::lifetime::deref(&piece->complete, ctx->runner);
}

void _generate_texture(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerTexture> texture,
  Ref<PerPiece> piece,
  Ref<PerLoad> load,
  Ref<session::Data> session
) {
  ZoneScoped;
  auto a = &session->artline;
  auto key = lib::hash64::from_cstr(texture->file_path);

  {
    lib::mutex::lock(&load->ready.mutex);

    auto arr = &load->ready.texture_keys;
    lib::array::ensure_space(arr, 1);
    (*arr)->data[(*arr)->count++] = key;

    lib::mutex::unlock(&load->ready.mutex);
  }

  {
    lib::mutex::lock(&a->mutex);
    auto hit = lib::u64_table::lookup(a->textures_by_key, key);
    if (hit == nullptr) {
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
          &session->vulkan->queue_work,
          session.ptr
        )
      );

      auto task_finish_texture = lib::defer(
        lib::task::create(
          _finish_texture,
          texture.ptr,
          piece.ptr,
          &session->vulkan->textures,
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

      texture->texture_id = lib::guid::next(session->guid_counter);
      texture->key = key;

      lib::u64_table::insert(&a->textures_by_key, key, CachedTexture {
        .ref_count = 1,
        .pending = task_finish_texture.second,
        .texture_id = texture->texture_id,
      });
    } else {
      hit->ref_count++;
      texture->texture_id = hit->texture_id;

      if (hit->pending == nullptr) {
        lib::lifetime::deref(&piece->complete, ctx->runner);
      } else {
        auto task = lib::task::create(
          _after_pending_texture,
          piece.ptr
        );

        ctx->new_tasks.insert(ctx->new_tasks.end(), {
          task,
        });

        ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
          { hit->pending, task },
        });

        lib::task::inject_pending(ctx); // still under mutex
      }
    }
    lib::mutex::unlock(&a->mutex);
  }
}

void _finish_meshes(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerPiece> piece,
  Ref<session::Data> session
) {
  ZoneScoped;

  {
    auto a = &session->artline;
    lib::mutex::lock(&a->mutex);
    auto cached = lib::u64_table::lookup(
      a->meshes_by_key,
      piece->mesh_key
    );
    assert(cached != nullptr);
    cached->pending = nullptr;
    cached->mesh_ids = piece->mesh_ids;
    lib::mutex::unlock(&a->mutex);
  }

  lib::lifetime::deref(&piece->complete, ctx->runner);
}

void _after_pending_meshes(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<PerPiece> piece,
  Ref<session::Data> session
) {
  ZoneScoped;
  {
    auto a = &session->artline;
    lib::mutex::lock(&a->mutex);
    auto hit = lib::u64_table::lookup(a->meshes_by_key, piece->mesh_key);
    assert(hit != nullptr);
    piece->mesh_ids = hit->mesh_ids;
    lib::mutex::unlock(&a->mutex);
  }
  lib::lifetime::deref(&piece->complete, ctx->runner);
}

void _generate_meshes(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<Piece> desc_model,
  Ref<PerPiece> model,
  Ref<PerLoad> load,
  Ref<session::Data> session
) {
  ZoneScoped;

  auto a = &session->artline;
  lib::hash64_t key = {};
  switch (desc_model->geometry.type) {
    case PieceGeometry::Type::File: {
      auto it = &desc_model->geometry.file;
      key = lib::hash64::from_cstr(it->path);
      break;
    }
    case PieceGeometry::Type::Gen0: {
      auto it = &desc_model->geometry.gen0;
      auto h = lib::hash64::begin();
      lib::hash64::add_value(&h, it->signature);
      lib::hash64::add_value(&h, load->dll_id);
      key = lib::hash64::get(&h);
      break;
    }
    default: {
      assert("Unknown mesh type" && false);
    }
  }
  model->mesh_key = key;

  {
    lib::mutex::lock(&load->ready.mutex);

    auto arr = &load->ready.mesh_keys;
    lib::array::ensure_space(arr, 1);
    (*arr)->data[(*arr)->count++] = key;

    lib::mutex::unlock(&load->ready.mutex);
  }

  lib::Task *task_our_pending = nullptr;
  {
    lib::mutex::lock(&a->mutex);
    auto hit = lib::u64_table::lookup(a->meshes_by_key, key);
    if (hit == nullptr) {
      task_our_pending = lib::task::create(
        _finish_meshes,
        model.ptr,
        session.ptr
      );

      ctx->new_tasks.push_back(task_our_pending);

      lib::u64_table::insert(&a->meshes_by_key, key, CachedMesh {
        .ref_count = 1,
        .pending = task_our_pending,
        .mesh_ids = nullptr, // valid after `task_our_pending` finishes.
      });
    } else {
      hit->ref_count++;
      if (hit->pending == nullptr) {
        model->mesh_ids = hit->mesh_ids;
        lib::lifetime::deref(&model->complete, ctx->runner);
      } else {
        auto task = lib::task::create(
          _after_pending_meshes,
          model.ptr,
          session.ptr
        );

        ctx->new_tasks.push_back(task);

        ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
          { hit->pending, task },
        });

        lib::task::inject_pending(ctx); // still under mutex
      }
    }
    lib::mutex::unlock(&a->mutex);
  }

  if (task_our_pending != nullptr) {
    lib::array_t<common::mesh::T06> *t06_meshes = nullptr;

    switch (desc_model->geometry.type) {
      case PieceGeometry::Type::File: {
        auto it = &desc_model->geometry.file;

        t06_meshes = lib::array::create<common::mesh::T06>(load->misc, 1);
        t06_meshes->data[t06_meshes->count++] = (
          common::t06::read_file(it->path.start)
        );
        break;
      }
      case PieceGeometry::Type::Gen0: {
        auto it = &desc_model->geometry.gen0;

        #if 0
          t06_meshes = generate_mc_v0(
            load->misc,
            it->signed_distance_fn,
            it->texture_uv_fn
          );
        #else
          t06_meshes = generate_dc_v1(
            load->misc,
            it->signed_distance_fn,
            it->texture_uv_fn,
            &it->params
          );
        #endif
        break;
      }
      default: {
        assert("Unknown mesh type" && false);
      }
    }

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
      auto signal_init_buffer = lib::task::create_external_signal();
      auto task_init_buffer = lib::defer(
        lib::task::create(
          _upload_mesh_init_buffer,
          &t06_meshes->data[i],
          &model->meshes->data[i].mesh_item,
          signal_init_buffer,
          &session->vulkan->queue_work,
          session.ptr
        )
      );
      auto signal_init_blas = lib::task::create_external_signal();
      auto task_init_blas = lib::defer(
        lib::task::create(
          _upload_mesh_init_blas,
          &model->meshes->data[i].mesh_item,
          signal_init_blas,
          &session->vulkan->queue_work,
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
          &session->vulkan->meshes,
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
        { task_finish_mesh.second, task_our_pending },
      });
    }
  }
}

void _begin_model(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<Piece> desc_model,
  Ref<PerLoad> load,
  Ref<session::Data> session
) {
  ZoneScoped;

  auto model = lib::allocator::make<PerPiece>(load->misc);
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
    _piece_end,
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
    .file_path = desc_model->material.file_path_albedo,
    .format = engine::common::texture::ALBEDO_TEXTURE_FORMAT,
  };
  model->textures->data[1] = {
    .file_path = desc_model->material.file_path_normal,
    .format = engine::common::texture::NORMAL_TEXTURE_FORMAT,
  };
  model->textures->data[2] = {
    .file_path = desc_model->material.file_path_romeao,
    .format = engine::common::texture::ROMEAO_TEXTURE_FORMAT,
  };

  for (size_t i = 0; i < NUM_TEXTURE_TYPES; i++) {
    auto task_generate_texture = lib::task::create(
      _generate_texture,
      &model->textures->data[i],
      model,
      load.ptr,
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
  lib::lifetime::init(&impl->pieces_complete, 0);
  impl->desc.pieces = lib::array::create<Piece>(load->misc, 0);
  
  load->impl = impl;

  {
    load->dll_file_path_copy = lib::cstr::copy(load->misc, load->dll_file_path);
    ((char *)load->dll_file_path_copy.end)[-1] = 'x';

    {
      auto result = CopyFile(
        load->dll_file_path.start,
        load->dll_file_path_copy.start,
        false
      );

      if (result == 0) {
        auto error = GetLastError();
        LOG("CopyFile error: {}", error);
      }
    }

    HINSTANCE h_lib = LoadLibrary(load->dll_file_path_copy.start);
    assert(h_lib != nullptr);

    auto describe_fn = (DescribeFn *) GetProcAddress(h_lib, "describe");
    assert(describe_fn != nullptr);
    describe_fn(&impl->desc);

    impl->h_lib = h_lib;
  }

  for (size_t i = 0; i < impl->desc.pieces->count; i++) {
    lib::lifetime::ref(&impl->pieces_complete);
    auto model = &impl->desc.pieces->data[i];

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
      session.ptr
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_finish.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { impl->pieces_complete.yarn, task_finish.first },
  });
}

} // namespace
