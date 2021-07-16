#include <src/global.hxx>
#include <src/task/defer.hxx>
#include <src/engine/mesh.hxx>
#include "simple.hxx"

namespace engine::loading::simple {

namespace task = lib::task;
namespace usage = lib::usage;

struct Data {
  engine::common::mesh::T05 the_mesh;
  SessionData::Vulkan::Meshes::Item mesh_item;
};

void _read_file(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<Data> data 
) {
  ZoneScoped;
  { ZoneScopedN("the_mesh_load");
    data->the_mesh = engine::mesh::read_t05_file("assets/mesh.t05");
  }
}

void _init_buffer(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<Data> data 
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

void _insert_items(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<SessionData::Scene> scene,
  usage::Full<SessionData::Vulkan::Meshes> meshes,
  usage::Full<Data> data 
) {
  ZoneScoped;
  meshes->items.push_back(data->mesh_item);
  size_t mesh_index = meshes->items.size() - 1;
  scene->items.push_back(SessionData::Scene::Item {
    .transform = glm::mat4(1.0f),
    .mesh_index = mesh_index,
  });
}

void _cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::UnfinishedYarns> unfinished_yarns,
  usage::None<lib::Task> yarn,
  usage::Full<Data> data 
) {
  ZoneScoped;
  engine::mesh::deinit_t05(&data->the_mesh);
  delete data.ptr;
  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.erase(yarn.ptr);
  }
  task::signal(ctx->runner, yarn.ptr);
}

void _unload(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::UnfinishedYarns> unfinished_yarns,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<SessionData::Scene> scene,
  usage::Full<SessionData::Vulkan::Meshes> meshes,
  usage::None<lib::Task> yarn
) {
  ZoneScoped;
  for (auto &item : meshes->items) {
    lib::gfx::multi_alloc::deinit(
      &item.data.multi_alloc,
      core->device,
      core->allocator
    );
  }
  meshes->items.clear();
  scene->items.clear();
  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.erase(yarn.ptr);
  }
  task::signal(ctx->runner, yarn.ptr);
}

// @Note: we pass these as pointers, but usage information gets lost,
// and so callers need to see what this does to adapt their own usage.
// Seems brittle.
void load(
  lib::task::ContextBase *ctx,
  SessionData::UnfinishedYarns *unfinished_yarns,
  SessionData::Scene *scene,
  SessionData::Vulkan::Core *core,
  SessionData::Vulkan::Meshes *meshes
) {
  ZoneScoped;
  auto yarn = task::create_yarn_signal();
  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.insert(yarn);
  }
  auto data = new Data {};
  auto task_read_file = task::create(
    _read_file,
    data
  );
  auto task_init_buffer = task::create(
    defer,
    task::create(
      _init_buffer,
      core,
      data
    )
  );
  auto task_insert_items = task::create(
    defer,
    task::create(
      _insert_items,
      scene,
      meshes,
      data
    )
  );
  auto task_cleanup = task::create(
    defer,
    task::create(
      _cleanup,
      unfinished_yarns,
      yarn,
      data
    )
  );
  task::inject(ctx->runner, {
    task_read_file,
    task_init_buffer,
    task_insert_items,
    task_cleanup
  }, {
    .new_dependencies = {
      { task_read_file, task_init_buffer },
      { task_init_buffer, task_insert_items },
      { task_insert_items, task_cleanup },
    },
  });
}

void unload(
  lib::task::ContextBase *ctx,
  SessionData::UnfinishedYarns *unfinished_yarns,
  SessionData::Scene *scene,
  SessionData::Vulkan::Core *core,
  RenderingData::InflightGPU *inflight_gpu,
  SessionData::Vulkan::Meshes *meshes
) {
  auto yarn = task::create_yarn_signal();
  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.insert(yarn);
  }
  auto task_unload = task::create(
    _unload,
    unfinished_yarns,
    core,
    scene,
    meshes,
    yarn
  );
  {
    std::scoped_lock lock(inflight_gpu->mutex);
    std::vector<std::pair<lib::Task *, lib::Task *>> dependencies;
    for (auto signal : inflight_gpu->signals) {
      dependencies.push_back({ signal, task_unload });
    }
    task::inject(ctx->runner, {
      task_unload
    }, {
      .new_dependencies = dependencies,
    });
  }
}

} // namespace