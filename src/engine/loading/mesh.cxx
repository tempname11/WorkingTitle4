#include <src/engine/mesh.hxx>
#include <src/task/defer.hxx>
#include "mesh.hxx"

namespace engine::loading::mesh {

struct LoadData {
  engine::common::mesh::T05 the_mesh;
  lib::GUID mesh_id;
  SessionData::Vulkan::Meshes::Item mesh_item;
};

void _read_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<LoadData> data 
) {
  ZoneScoped;
  data->the_mesh = engine::mesh::read_t05_file("assets/mesh.t05");
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
}

void _load_finish(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<SessionData::Vulkan::Meshes> meshes,
  Own<LoadData> data
) {
  data->mesh_item.ref_count = 1;
  meshes->items.insert({ data->mesh_id, data->mesh_item });
}

void deref(
  lib::GUID mesh_id,
  Use<SessionData::Vulkan::Core> core,
  Own<SessionData::Vulkan::Meshes> meshes
) {
  auto mesh = &meshes->items.at(mesh_id);
  assert(mesh->ref_count > 0);
  mesh->ref_count--;
  if (mesh->ref_count == 0) {
    lib::gfx::multi_alloc::deinit(
      &mesh->data.multi_alloc,
      core->device,
      core->allocator
    );

    meshes->items.erase(mesh_id);
  }
}

lib::Task* load(
  lib::task::ContextBase* ctx,
  Ref<SessionData> session,
  Use<SessionData::GuidCounter> guid_counter,
  lib::GUID *out_mesh_id
) {
  auto mesh_id = lib::guid::next(guid_counter.ptr);
  *out_mesh_id = mesh_id;

  auto data = new LoadData {
    .mesh_id = mesh_id,
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
