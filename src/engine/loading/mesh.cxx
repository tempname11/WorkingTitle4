#include <src/task/after_inflight.hxx>
#include <src/task/defer.hxx>
#include "mesh.hxx"

namespace engine::loading::mesh {

engine::common::mesh::T05 zero_t05 = {};

engine::common::mesh::T05 read_t05_file(const char *filename) {
  ZoneScoped;
  auto file = fopen(filename, "rb");

  // assert(file != nullptr);
  if (file == nullptr) {
    return zero_t05;
  }

  fseek(file, 0, SEEK_END);
  auto size = ftell(file);
  fseek(file, 0, SEEK_SET);
  auto buffer = (uint8_t *) malloc(size);
  fread((void *) buffer, 1, size, file);

  // assert(ferror(file) == 0);
  if (ferror(file) != 0) {
    fclose(file);
    free(buffer);
    return zero_t05;
  }

  fclose(file);

  auto triangle_count = *((uint32_t*) buffer);

  // assert(size == sizeof(uint32_t) + sizeof(engine::common::mesh::VertexT05) * 3 * triangle_count);
  if (size != sizeof(uint32_t) + sizeof(engine::common::mesh::VertexT05) * 3 * triangle_count) {
    free(buffer);
    return zero_t05;
  }

  return {
    .buffer = buffer,
    .triangle_count = triangle_count,
    // three vertices per triangle
    .vertices = ((engine::common::mesh::VertexT05 *) (buffer + sizeof(uint32_t))),
  };
}

void deinit_t05(engine::common::mesh::T05 *it) {
  ZoneScoped;
  if (it->buffer != nullptr) {
    free(it->buffer);
  }
}

struct LoadData {
  lib::GUID mesh_id;
  std::string path;
  engine::common::mesh::T05 the_mesh;
  SessionData::Vulkan::Meshes::Item mesh_item;
};

void _load_read_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<LoadData> data 
) {
  ZoneScoped;
  data->the_mesh = read_t05_file(data->path.c_str());
}

void _load_init_buffer(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Use<SessionData::Vulkan::Core> core,
  Own<LoadData> data 
) {
  ZoneScoped;

  data->mesh_item.triangle_count = data->the_mesh.triangle_count;
  lib::gfx::multi_alloc::init(
    &data->mesh_item.multi_alloc,
    { lib::gfx::multi_alloc::Claim {
      .info = {
        .buffer = {
          .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
          .size = std::max(
            size_t(1), // for zero buffer
            data->the_mesh.triangle_count * 3 * sizeof(engine::common::mesh::VertexT05)
          ),
          .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        },
      },
      .memory_property_flags = VkMemoryPropertyFlagBits(0
      | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
      | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
      ),
      .p_stake_buffer = &data->mesh_item.vertex_stake,
    }},
    core->device,
    core->allocator,
    &core->properties.basic,
    &core->properties.memory
  );

  void *mem;
  {
    auto result = vkMapMemory(
      core->device,
      data->mesh_item.vertex_stake.memory,
      data->mesh_item.vertex_stake.offset,
      data->mesh_item.vertex_stake.size,
      0,
      &mem
    );
    assert(result == VK_SUCCESS);
  }
  memcpy(
    mem,
    data->the_mesh.vertices,
    data->the_mesh.triangle_count * 3 * sizeof(engine::common::mesh::VertexT05)
  );
  vkUnmapMemory(
    core->device,
    data->mesh_item.vertex_stake.memory
  );
}

void _load_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Own<SessionData::Vulkan::Meshes> meshes,
  Own<SessionData::MetaMeshes> meta_meshes,
  Own<LoadData> data
) {
  ZoneScoped;

  meshes->items.insert({ data->mesh_id, data->mesh_item });
  auto meta = &meta_meshes->items.at(data->mesh_id);
  meta->status = SessionData::MetaMeshes::Status::Ready;
  meta->will_have_loaded = nullptr;
  meta->invalid = data->mesh_item.triangle_count == 0;

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  deinit_t05(&data->the_mesh);

  delete data.ptr;
}

void _unload_item(
  SessionData::Vulkan::Meshes::Item *item,
  Use<SessionData::Vulkan::Core> core
) {
  lib::gfx::multi_alloc::deinit(
    &item->multi_alloc,
    core->device,
    core->allocator
  );
}

void _reload_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Vulkan::Meshes> meshes,
  Own<SessionData::MetaMeshes> meta_meshes,
  Own<LoadData> data
) {
  ZoneScoped;

  auto item = &meshes->items.at(data->mesh_id);
  auto old_item = *item; // copy old data
  *item = data->mesh_item; // replace the data

  _unload_item(
    &old_item,
    core
  );

  auto meta = &meta_meshes->items.at(data->mesh_id);
  meta->invalid = item->triangle_count == 0;
  meta->will_have_reloaded = nullptr;

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  deinit_t05(&data->the_mesh);

  delete data.ptr;
}

struct DerefData {
  lib::GUID mesh_id;
};

void _deref(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Vulkan::Meshes> meshes,
  Own<SessionData::MetaMeshes> meta_meshes,
  Own<DerefData> data
) {
  ZoneScoped;

  auto meta = &meta_meshes->items.at(data->mesh_id);
  assert(meta->ref_count > 0);

  // we must have waited for the initial load to finish.
  assert(meta->status == SessionData::MetaMeshes::Status::Ready);

  // we must have waited for reload to finish.
  assert(meta->will_have_reloaded == nullptr);

  meta->ref_count--;
  if (meta->ref_count == 0) {
    auto mesh = &meshes->items.at(data->mesh_id);
    _unload_item(
      mesh,
      core
    );

    meta_meshes->by_path.erase(meta->path);
    meta_meshes->items.erase(data->mesh_id);
    meshes->items.erase(data->mesh_id);
  }

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  delete data.ptr;
}

void deref(
  lib::GUID mesh_id,
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Use<SessionData::MetaMeshes> meta_meshes
) {
  ZoneScoped;

  lib::lifetime::ref(&session->lifetime);

  auto meta = &meta_meshes->items.at(mesh_id);
  auto data = new DerefData {
    .mesh_id = mesh_id,
  };
  auto task_deref = lib::task::create(
    after_inflight,
    session.ptr,
    lib::task::create(
      _deref,
      session.ptr,
      &session->vulkan.core,
      &session->vulkan.meshes,
      &session->meta_meshes,
      data
    )
  );
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_deref,
  });
  if (meta->will_have_loaded != nullptr) {
    ctx->new_dependencies.insert(ctx->new_dependencies.end(),
      { meta->will_have_loaded, task_deref }
    );
  }
  if (meta->will_have_reloaded != nullptr) {
    ctx->new_dependencies.insert(ctx->new_dependencies.end(),
      { meta->will_have_reloaded, task_deref }
    );
  }
}

void reload(
  lib::GUID mesh_id,
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Own<SessionData::MetaMeshes> meta_meshes
) {
  ZoneScoped;

  lib::lifetime::ref(&session->lifetime);

  auto meta = &meta_meshes->items.at(mesh_id);
  assert(meta->ref_count > 0);

  // Reloading will likely only ever be manual, and right now the user
  // would simply have to wait to reload something that's already reloading.
  // We have to ensure the status is right before calling this.
  assert(meta->status == SessionData::MetaMeshes::Status::Ready);

  auto data = new LoadData {
    .mesh_id = mesh_id,
    .path = meta->path,
  };

  auto task_read_file = lib::task::create(
    _load_read_file,
    data
  );
  auto task_init_buffer = defer(
    lib::task::create(
      _load_init_buffer,
      &session->vulkan.core,
      data
    )
  );
  auto task_finish = lib::task::create(
    after_inflight,
    session.ptr,
    lib::task::create(
      _reload_finish,
      session.ptr,
      &session->vulkan.core,
      &session->vulkan.meshes,
      &session->meta_meshes,
      data
    )
  );

  assert(!meta->will_have_reloaded);
  meta->will_have_reloaded = task_finish;

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_read_file,
    task_init_buffer.first,
    task_finish,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_file, task_init_buffer.first },
    { task_init_buffer.second, task_finish },
  });
}

lib::Task* load(
  std::string &path,
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Own<SessionData::MetaMeshes> meta_meshes,
  Use<SessionData::GuidCounter> guid_counter,
  lib::GUID *out_mesh_id
) {
  ZoneScoped;

  auto it = meta_meshes->by_path.find(path);
  if (it != meta_meshes->by_path.end()) {
    auto mesh_id = it->second;
    *out_mesh_id = mesh_id;

    auto meta = &meta_meshes->items.at(mesh_id);
    meta->ref_count++;

    if (meta->status == SessionData::MetaMeshes::Status::Loading) {
      assert(meta->will_have_loaded != nullptr);
      return meta->will_have_loaded;
    }

    if (meta->status == SessionData::MetaMeshes::Status::Ready) {
      return nullptr;
    }

    assert(false);
  }

  lib::lifetime::ref(&session->lifetime);

  auto mesh_id = lib::guid::next(guid_counter.ptr);
  *out_mesh_id = mesh_id;
  meta_meshes->by_path.insert({ path, mesh_id });
  
  auto data = new LoadData {
    .mesh_id = mesh_id,
    .path = path,
  };

  auto task_read_file = lib::task::create(
    _load_read_file,
    data
  );
  auto task_init_buffer = defer(
    lib::task::create(
      _load_init_buffer,
      &session->vulkan.core,
      data
    )
  );
  auto task_finish = defer(
    lib::task::create(
      _load_finish,
      session.ptr,
      &session->vulkan.meshes,
      &session->meta_meshes,
      data
    )
  );
  
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_read_file,
    task_init_buffer.first,
    task_finish.first,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_file, task_init_buffer.first },
    { task_init_buffer.second, task_finish.first },
  });

  SessionData::MetaMeshes::Item meta = {
    .ref_count = 1, 
    .status = SessionData::MetaMeshes::Status::Loading,
    .will_have_loaded = task_finish.second,
    .path = path,
  };
  meta_meshes->items.insert({ mesh_id, meta });

  return task_finish.second;
}

} // namespace
