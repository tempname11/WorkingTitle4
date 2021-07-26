#include <src/engine/mesh.hxx>
#include <src/task/defer.hxx>
#include "mesh.hxx"

namespace engine::loading::mesh {

struct LoadData {
  lib::GUID mesh_id;
  const char *path;
  engine::common::mesh::T05 the_mesh;
  SessionData::Vulkan::Meshes::Item mesh_item;
};

void _read_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<LoadData> data 
) {
  ZoneScoped;
  data->the_mesh = engine::mesh::read_t05_file(data->path);
}

void _init_buffer(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Use<SessionData::Vulkan::Core> core,
  Own<LoadData> data 
) {
  ZoneScoped;

  data->mesh_item.data.triangle_count = data->the_mesh.triangle_count;
  lib::gfx::multi_alloc::init(
    &data->mesh_item.data.multi_alloc,
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
      .p_stake_buffer = &data->mesh_item.data.vertex_stake,
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
      data->mesh_item.data.vertex_stake.memory,
      data->mesh_item.data.vertex_stake.offset,
      data->mesh_item.data.vertex_stake.size,
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
    data->mesh_item.data.vertex_stake.memory
  );
}

void _load_cleanup(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<LoadData> data 
) {
  ZoneScoped;
  engine::mesh::deinit_t05(&data->the_mesh);
  delete data.ptr;
}

void _load_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<SessionData::Vulkan::Meshes> meshes,
  Own<SessionData::MetaMeshes> meta_meshes,
  Own<LoadData> data
) {
  meshes->items.insert({ data->mesh_id, data->mesh_item });
  auto meta = &meta_meshes->items.at(data->mesh_id);
  meta->status = SessionData::MetaMeshes::Item::Status::Ready;
  meta->will_have_loaded = nullptr;
}

void deref(
  lib::GUID mesh_id,
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Vulkan::Meshes> meshes,
  Own<SessionData::MetaMeshes> meta_meshes
) {
  auto meta = &meta_meshes->items.at(mesh_id);
  assert(meta->ref_count > 0);
  meta->ref_count--;
  if (meta->ref_count == 0) {
    auto mesh = &meshes->items.at(mesh_id);
    lib::gfx::multi_alloc::deinit(
      &mesh->data.multi_alloc,
      core->device,
      core->allocator
    );

    meta_meshes->by_path.erase(meta->path);
    meta_meshes->items.erase(mesh_id);
    meshes->items.erase(mesh_id);
  }
}

lib::Task* load(
  std::string path,
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Own<SessionData::MetaMeshes> meta_meshes,
  Use<SessionData::GuidCounter> guid_counter,
  lib::GUID *out_mesh_id
) {
  auto it = meta_meshes->by_path.find(path);
  if (it != meta_meshes->by_path.end()) {
    auto mesh_id = it->second;
    *out_mesh_id = mesh_id;

    auto meta = &meta_meshes->items.at(mesh_id);
    meta->ref_count++;

    if (meta->status == SessionData::MetaMeshes::Item::Status::Loading) {
      assert(meta->will_have_loaded != nullptr);
      return meta->will_have_loaded;
    }

    if (meta->status == SessionData::MetaMeshes::Item::Status::Ready) {
      return nullptr;
    }

    assert(false);
  }

  auto mesh_id = lib::guid::next(guid_counter.ptr);
  *out_mesh_id = mesh_id;

  SessionData::MetaMeshes::Item meta = {
    .ref_count = 1, 
    .status = SessionData::MetaMeshes::Item::Status::Loading,
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
    _read_file,
    data
  );
  auto task_init_buffer = lib::task::create(
    defer,
    lib::task::create(
      _init_buffer,
      &session->vulkan.core,
      data
    )
  );
  auto task_cleanup = lib::task::create(
    defer,
    lib::task::create(
      _load_cleanup,
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
    task_cleanup,
  });

  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_file, task_init_buffer },
    { task_init_buffer, task_finish },
    { task_finish, task_cleanup },
  });

  return task_finish;
}

} // namespace
