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

struct LoadMeshData {
  lib::GUID mesh_id;
  engine::common::mesh::T06 t06;
  engine::session::Vulkan::Meshes::Item mesh_item;
  CountdownData *countdown;
};

void _read_mesh_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadMeshData> data 
) {
  ZoneScoped;
  data->t06 = grup::mesh::read_t06_file("assets/gi_test_0.t06"); // @Tmp !
}

void _finish_mesh(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<LoadMeshData> data,
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
  std::vector<session::Data::Scene::Item> scene_items;
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
    item->status = Data::Status::Ready;
    // @Incomplete: store mesh ids so we can clean them up later!
  }

  scene->items.insert(
    scene->items.end(),
    finish_data->scene_items.begin(),
    finish_data->scene_items.end()
  );

  if (data->yarn_end != nullptr) {
    lib::task::signal(ctx->runner, data->yarn_end);
  }

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  delete data.ptr;
  delete finish_data.ptr;
}

void _add_mesh_tasks(
  lib::GUID mesh_id,
  CountdownData *countdown,
  Ref<engine::session::Data> session,
  lib::task::ContextBase *ctx
) {
  auto data = new LoadMeshData {
    .mesh_id = mesh_id,
    .countdown = countdown,
  };
  auto task_read_file = lib::task::create( // @Tmp
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
      _finish_mesh,
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

  engine::system::artline::Description desc;
  {
    HINSTANCE h_lib = LoadLibrary(data->dll_filename.c_str());
    assert(h_lib != nullptr);

    auto describe_fn = (DescribeFn *) GetProcAddress(h_lib, "describe");
    assert(describe_fn != nullptr);

    describe_fn(&desc);

    auto result = FreeLibrary(h_lib);
    assert(result != 0);
  }

  auto countdown = new CountdownData {
    .yarn_zero = lib::task::create_yarn_signal(),
  };

  auto finish_data = new FinishData {};

  for (auto &model : desc.models) {
    auto mesh_id = lib::guid::next(&session->guid_counter);
    auto texture_albedo_id = lib::guid::next(&session->guid_counter);
    auto texture_normal_id = lib::guid::next(&session->guid_counter);
    auto texture_romeao_id = lib::guid::next(&session->guid_counter);

    finish_data->scene_items.push_back(session::Data::Scene::Item {
      .owner_id = data->dll_id,
      .transform = model.transform,
      .mesh_id = mesh_id,
      .texture_albedo_id = texture_albedo_id,
      .texture_normal_id = texture_normal_id,
      .texture_romeao_id = texture_romeao_id,
    });

    _add_mesh_tasks(
      mesh_id,
      countdown,
      session,
      ctx
    );

    _add_texture_tasks(
      texture_albedo_id,
      engine::common::texture::ALBEDO_TEXTURE_FORMAT,
      &model.filename_albedo,
      countdown,
      session,
      ctx
    );

    _add_texture_tasks(
      texture_normal_id,
      engine::common::texture::NORMAL_TEXTURE_FORMAT,
      &model.filename_normal,
      countdown,
      session,
      ctx
    );

    _add_texture_tasks(
      texture_romeao_id,
      engine::common::texture::ROMEAO_TEXTURE_FORMAT,
      &model.filename_romeao,
      countdown,
      session,
      ctx
    );
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
