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

struct LoadTextureData {
  lib::cstr_range_t file_path;
  VkFormat format;
  engine::common::texture::Data<uint8_t> data;
};

struct FinishData {
  lib::Lifetime lifetime;
  HINSTANCE h_lib;
  Description desc;
};

struct PerMeshData {
  lib::GUID mesh_id; // Sadly, distinct from the uploaded buffer id.
  session::Vulkan::Meshes::Item mesh_item;
};

struct PerTextureData {
  lib::GUID texture_id; // Sadly, distinct from the uploaded texture id.
  session::Vulkan::Textures::Item texture_item;
};

struct AssembleData {
  lib::array_t<PerMeshData> *meshes;
  lib::array_t<PerTextureData> *textures;
};

void _read_texture_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadTextureData> data 
) {
  ZoneScoped;
  int width, height, _channels = 4;
  auto loaded = stbi_load(data->file_path.start, &width, &height, &_channels, 4);
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
  Ref<PerTextureData> per_texture,
  Ref<LoadTextureData> data,
  Own<engine::session::Vulkan::Textures> textures,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  per_texture->texture_id = lib::guid::next(&session->guid_counter);
  textures->items.insert({ per_texture->texture_id, per_texture->texture_item });
  stbi_image_free(data->data.data);
}

void _finish_mesh(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<common::mesh::T06> t06,
  Ref<PerMeshData> per_mesh,
  Own<engine::session::Vulkan::Meshes> meshes,
  Ref<engine::session::Data> session,
  Ref<LoadData> data
) {
  ZoneScoped;
  per_mesh->mesh_id = lib::guid::next(&session->guid_counter);
  meshes->items.insert({ per_mesh->mesh_id, per_mesh->mesh_item });
  grup::mesh::deinit_t06(t06.ptr);
}

void _assemble_scene_items(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<Model> model_data,
  Ref<AssembleData> assemble_data,
  Ref<FinishData> finish_data,
  Ref<LoadData> data
) {
  ZoneScoped;

  lib::mutex::lock(&data->ready->mutex);

  auto count = assemble_data->meshes->count;
  lib::array::ensure_space(&data->ready->scene_items, count);
  for (size_t i = 0; i < count; i++) {
    data->ready->scene_items->data[
      data->ready->scene_items->count++
    ] = session::Data::Scene::Item {
      .owner_id = data->dll_id,
      .transform = model_data->transform,
      .mesh_id = assemble_data->meshes->data[i].mesh_id,
      .texture_albedo_id = assemble_data->textures->data[0].texture_id,
      .texture_normal_id = assemble_data->textures->data[1].texture_id,
      .texture_romeao_id = assemble_data->textures->data[2].texture_id,
    };
  }

  lib::mutex::unlock(&data->ready->mutex);

  lib::lifetime::deref(&finish_data->lifetime, ctx->runner);
}

void _generate_model(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<Model> model_data,
  Ref<FinishData> finish_data,
  Ref<LoadData> data,
  Ref<session::Data> session
) {
  ZoneScoped;

  lib::array_t<common::mesh::T06> *t06_meshes = nullptr;
  switch (model_data->mesh.type) {
    case ModelMesh::Type::File: {
      auto it = &model_data->mesh.file;
      t06_meshes = lib::array::create<common::mesh::T06>(data->misc, 1);
      t06_meshes->data[t06_meshes->count++] = (
        grup::mesh::read_t06_file(it->path.start)
      );
      break;
    }
    case ModelMesh::Type::Density: {
      auto it = &model_data->mesh.density;
      t06_meshes = generate(data->misc, it->density_fn);
      // Note that this work is potentially very slow,
      // it would be better to move it to another task.
      break;
    }
    default: {
      assert("Unknown mesh type" && false);
    }
  }
    
  auto assemble_data = lib::allocator::make<AssembleData>(data->misc);
  *assemble_data = {
    .meshes = lib::array::create<PerMeshData>(
      data->misc,
      t06_meshes->count
    ),
    .textures = lib::array::create<PerTextureData>(
      data->misc,
      NUM_TEXTURE_TYPES
    ),
  };

  // initialized in tasks
  assemble_data->meshes->count = assemble_data->meshes->capacity;
  assemble_data->textures->count = assemble_data->textures->capacity;

  auto task_assemble_scene_items = lib::task::create(
    _assemble_scene_items,
    model_data.ptr,
    assemble_data,
    finish_data.ptr,
    data.ptr
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_assemble_scene_items,
  });

  for (size_t i = 0; i < t06_meshes->count; i++) {
    auto signal_init_buffer = lib::task::create_external_signal();
    auto task_init_buffer = lib::defer(
      lib::task::create(
        _upload_mesh_init_buffer,
        &t06_meshes->data[i],
        &assemble_data->meshes->data[i].mesh_item,
        signal_init_buffer,
        &session->vulkan.queue_work,
        session.ptr
      )
    );
    auto signal_init_blas = lib::task::create_external_signal();
    auto task_init_blas = lib::defer(
      lib::task::create(
        _upload_mesh_init_blas,
        &assemble_data->meshes->data[i].mesh_item,
        signal_init_blas,
        &session->vulkan.queue_work,
        session.ptr
      )
    );

    auto task_finish_mesh = lib::defer(
      lib::task::create(
        _finish_mesh,
        &t06_meshes->data[i],
        &assemble_data->meshes->data[i],
        &session->vulkan.meshes,
        session.ptr,
        data.ptr
      )
    );

    ctx->new_tasks.insert(ctx->new_tasks.end(), {
      task_init_buffer.first,
      task_init_blas.first,
    });

    ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
      { signal_init_buffer, task_init_blas.first },
      { signal_init_blas, task_finish_mesh.first },
      { task_finish_mesh.second, task_assemble_scene_items },
    });
  }

  // @Cleanup: we also have PerTextureData above, which is redundant.
  auto load_texture_data = lib::array::create<LoadTextureData>(data->misc, NUM_TEXTURE_TYPES);
  load_texture_data->count = load_texture_data->capacity;
  load_texture_data->data[0] = {
    .file_path = model_data->file_path_albedo,
    .format = engine::common::texture::ALBEDO_TEXTURE_FORMAT,
  };
  load_texture_data->data[1] = {
    .file_path = model_data->file_path_normal,
    .format = engine::common::texture::NORMAL_TEXTURE_FORMAT,
  };
  load_texture_data->data[2] = {
    .file_path = model_data->file_path_romeao,
    .format = engine::common::texture::ROMEAO_TEXTURE_FORMAT,
  };

  for (size_t i = 0; i < NUM_TEXTURE_TYPES; i++) {
    auto task_read_file = lib::task::create(
      _read_texture_file,
      &load_texture_data->data[i]
    );

    auto signal_init_image = lib::task::create_external_signal();
    auto task_init_image = lib::defer(
      lib::task::create(
        _upload_texture,
        &load_texture_data->data[i].format,
        &load_texture_data->data[i].data,
        &assemble_data->textures->data[i].texture_item,
        signal_init_image,
        &session->vulkan.queue_work,
        session.ptr
      )
    );

    auto task_finish_texture = lib::defer(
      lib::task::create(
        _finish_texture,
        &assemble_data->textures->data[i],
        &load_texture_data->data[i],
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
      { task_finish_texture.second, task_assemble_scene_items },
    });
  }
}

void _finish_dll(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadData> data,
  Ref<FinishData> finish_data,
  Own<session::Data::Scene> scene,
  Ref<session::Data> session
) {
  ZoneScoped;

  {
    auto result = FreeLibrary(finish_data->h_lib);
    assert(result != 0);
  }

  if (data->yarn_done != nullptr) {
    lib::task::signal(ctx->runner, data->yarn_done);
  }
}

void _load_dll(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadData> data,
  Ref<session::Data> session
) {
  ZoneScoped;

  auto finish_data = lib::allocator::make<FinishData>(data->misc);
  *finish_data = FinishData {
    .h_lib = nullptr, // initialized later
  };
  lib::lifetime::init(&finish_data->lifetime, 0);
  finish_data->desc.models = lib::array::create<Model>(data->misc, 0);

  {
    HINSTANCE h_lib = LoadLibrary(data->dll_filename.start);
    assert(h_lib != nullptr);

    auto describe_fn = (DescribeFn *) GetProcAddress(h_lib, "describe");
    assert(describe_fn != nullptr);
    describe_fn(&finish_data->desc);

    finish_data->h_lib = h_lib;
  }

  for (size_t i = 0; i < finish_data->desc.models->count; i++) {
    lib::lifetime::ref(&finish_data->lifetime);
    auto model = &finish_data->desc.models->data[i];

    auto task_generate = lib::task::create(
      _generate_model,
      model,
      finish_data,
      data.ptr,
      session.ptr
    );

    ctx->new_tasks.insert(ctx->new_tasks.end(), {
      task_generate,
    });
  }

  auto task_finish = lib::defer(
    lib::task::create(
      _finish_dll,
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
    { finish_data->lifetime.yarn, task_finish.first },
  });
}

} // namespace
