#include <src/engine/mesh.hxx>
#include <src/task/after_inflight.hxx>
#include <src/task/defer.hxx>
#include "mesh.hxx"

namespace engine::loading::mesh {

struct LoadData {
  lib::GUID mesh_id;
  const char *path;
  engine::common::mesh::T05 the_mesh;
  SessionData::Vulkan::Meshes::Item mesh_item;
};

void _load_read_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<LoadData> data 
) {
  ZoneScoped;
  data->the_mesh = engine::mesh::read_t05_file(data->path);
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
          .size = data->the_mesh.triangle_count * 3 * sizeof(engine::common::mesh::VertexT05),
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
  Own<SessionData::Vulkan::Meshes> meshes,
  Own<SessionData::MetaMeshes> meta_meshes,
  Own<LoadData> data
) {
  ZoneScoped;

  meshes->items.insert({ data->mesh_id, data->mesh_item });
  auto meta = &meta_meshes->items.at(data->mesh_id);
  meta->status = SessionData::MetaMeshes::Status::Ready;
  meta->will_have_loaded = nullptr;

  engine::mesh::deinit_t05(&data->the_mesh);

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
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Vulkan::Meshes> meshes,
  Own<SessionData::MetaMeshes> meta_meshes,
  Use<SessionData::UnfinishedYarns> unfinished_yarns,
  Own<LoadData> data,
  Own<lib::Task> yarn
) {
  ZoneScoped;

  assert(meshes->items.contains(data->mesh_id));
  auto item = &meshes->items.at(data->mesh_id);
  auto old_item = *item; // copy old data
  *item = data->mesh_item; // replace the data

  _unload_item(
    &old_item,
    core
  );

  meta_meshes->items.at(data->mesh_id).reload_in_progress = false;

  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.erase(yarn.ptr);
  }
  task::signal(ctx->runner, yarn.ptr);

  engine::mesh::deinit_t05(&data->the_mesh);

  delete data.ptr;
}

struct DerefData {
  lib::GUID mesh_id;
};

void _deref(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Vulkan::Meshes> meshes,
  Own<SessionData::MetaMeshes> meta_meshes,
  Own<SessionData::UnfinishedYarns> unfinished_yarns,
  Own<DerefData> data,
  Own<lib::Task> yarn
) {
  ZoneScoped;

  auto meta = &meta_meshes->items.at(data->mesh_id);
  assert(meta->ref_count > 0);

  // we also need to support deref while loading...
  assert(meta->status == SessionData::MetaMeshes::Status::Ready);
  // ...or reloading
  assert(!meta->reload_in_progress);
  // watch out for c_str() below when doing this.

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

  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.erase(yarn.ptr);
  }
  task::signal(ctx->runner, yarn.ptr);

  delete data.ptr;
}

void deref(
  lib::GUID mesh_id,
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Use<SessionData::UnfinishedYarns> unfinished_yarns,
  Ref<RenderingData::InflightGPU> inflight_gpu
) {
  ZoneScoped;
  auto yarn = task::create_yarn_signal();
  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.insert(yarn);
  }

  auto data = new DerefData {
    .mesh_id = mesh_id,
  };

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    lib::task::create(
      after_inflight,
      inflight_gpu.ptr,
      lib::task::create(
        _deref,
        &session->vulkan.core,
        &session->vulkan.meshes,
        &session->meta_meshes,
        &session->unfinished_yarns,
        data,
        yarn
      )
    ),
  });
}

void reload(
  lib::GUID mesh_id,
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Use<SessionData::UnfinishedYarns> unfinished_yarns,
  Own<SessionData::MetaMeshes> meta_meshes,
  Ref<RenderingData::InflightGPU> inflight_gpu
) {
  auto yarn = task::create_yarn_signal();
  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.insert(yarn);
  }

  auto meta = &meta_meshes->items.at(mesh_id);
  assert(meta->ref_count > 0);

  // @Incomplete
  assert(meta->status == SessionData::MetaMeshes::Status::Ready);

  assert(!meta->reload_in_progress);
  meta->reload_in_progress = true;

  auto data = new LoadData {
    .mesh_id = mesh_id,
    .path = meta->path.c_str(),
    // ^ this pointer will be valid for the tasks,
    // no need to copy the string
  };

  auto task_read_file = lib::task::create(
    _load_read_file,
    data
  );
  auto task_init_buffer = lib::task::create(
    defer,
    lib::task::create(
      _load_init_buffer,
      &session->vulkan.core,
      data
    )
  );
  auto task_finish = lib::task::create(
    after_inflight,
    inflight_gpu.ptr,
    lib::task::create(
      _reload_finish,
      &session->vulkan.core,
      &session->vulkan.meshes,
      &session->meta_meshes,
      &session->unfinished_yarns,
      data,
      yarn
    )
  );

  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_read_file,
    task_init_buffer,
    task_finish,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_file, task_init_buffer },
    { task_init_buffer, task_finish },
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
  // should we use an unfinished yarn here, and not rely on caller doing it?

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

  auto mesh_id = lib::guid::next(guid_counter.ptr);
  *out_mesh_id = mesh_id;
  meta_meshes->by_path.insert({ path, mesh_id });

  SessionData::MetaMeshes::Item meta = {
    .ref_count = 1, 
    .status = SessionData::MetaMeshes::Status::Loading,
    .path = path,
  };
  meta_meshes->items.insert({ mesh_id, meta });
  
  auto data = new LoadData {
    .mesh_id = mesh_id,
    .path = meta_meshes->items.at(mesh_id).path.c_str(),
    // ^ this pointer will be valid for the tasks,
    // no need to copy the string
  };

  auto task_read_file = lib::task::create(
    _load_read_file,
    data
  );
  auto task_init_buffer = lib::task::create(
    defer,
    lib::task::create(
      _load_init_buffer,
      &session->vulkan.core,
      data
    )
  );
  auto task_finish = lib::task::create(
    defer,
    lib::task::create(
      _load_finish,
      &session->vulkan.meshes,
      &session->meta_meshes,
      data
    )
  );
  
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_read_file,
    task_init_buffer,
    task_finish,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_file, task_init_buffer },
    { task_init_buffer, task_finish },
  });

  return task_finish;
}

} // namespace
