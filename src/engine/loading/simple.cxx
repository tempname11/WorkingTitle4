#include <cassert>
#include <stb_image.h>
#include <src/global.hxx>
#include <src/lib/guid.hxx>
#include <src/task/defer.hxx>
#include <src/engine/mesh.hxx>
#include <src/engine/texture.hxx>
#include "mesh.hxx"
#include "simple.hxx"

namespace engine::loading::simple {

namespace task = lib::task;
namespace usage = lib::usage;

struct Data {
  lib::GUID group_id;

  lib::GUID mesh_id;

  // texture stuff
  engine::common::texture::Data<uint8_t> the_albedo;
  engine::common::texture::Data<uint8_t> the_normal;
  engine::common::texture::Data<uint8_t> the_romeao;
  SessionData::Vulkan::Textures::Item texture_albedo_item;
  SessionData::Vulkan::Textures::Item texture_normal_item;
  SessionData::Vulkan::Textures::Item texture_romeao_item;
  lib::gfx::multi_alloc::Instance texture_staging_multi_alloc;
  lib::gfx::multi_alloc::StakeBuffer texture_staging_albedo;
  lib::gfx::multi_alloc::StakeBuffer texture_staging_normal;
  lib::gfx::multi_alloc::StakeBuffer texture_staging_romeao;

  // also for textures
  VkSemaphore semaphore_finished;
  VkCommandPool command_pool;
};

void prepare_textures(
  SessionData::Vulkan::Core *core,
  VkQueue queue_work,
  Data *data
) {
  ZoneScoped;

   VkCommandBuffer cmd;
  { ZoneScopedN("allocate_command_buffer");
    VkCommandBufferAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = data->command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
    };
    {
      auto result = vkAllocateCommandBuffers(
        core->device,
        &alloc_info,
        &cmd
      );
      assert(result == VK_SUCCESS);
    }
  }

  { // begin
    auto info = VkCommandBufferBeginInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    auto result = vkBeginCommandBuffer(cmd, &info);
    assert(result == VK_SUCCESS);
  }

  engine::texture::prepare(
    &data->the_albedo,
    &data->texture_albedo_item.data,
    &data->texture_staging_albedo,
    core,
    cmd,
    queue_work
  );

  engine::texture::prepare(
    &data->the_normal,
    &data->texture_normal_item.data,
    &data->texture_staging_normal,
    core,
    cmd,
    queue_work
  );

  engine::texture::prepare(
    &data->the_romeao,
    &data->texture_romeao_item.data,
    &data->texture_staging_romeao,
    core,
    cmd,
    queue_work
  );

  { // end
    auto result = vkEndCommandBuffer(cmd);
    assert(result == VK_SUCCESS);
  }

  { // submit
    uint64_t one = 1;
    auto timeline_info = VkTimelineSemaphoreSubmitInfo {
      .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
      .signalSemaphoreValueCount = 1,
      .pSignalSemaphoreValues = &one,
    };
    VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = &timeline_info,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &data->semaphore_finished,
    };
    auto result = vkQueueSubmit(
      queue_work,
      1,
      &submit_info,
      VK_NULL_HANDLE
    );
    assert(result == VK_SUCCESS);
  }
}

void _read_texture_files(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<Data> data 
) {
  ZoneScoped;
  data->the_albedo = engine::texture::load_rgba8("assets/texture/albedo.jpg");
  data->the_normal = engine::texture::load_rgba8("assets/texture/normal.jpg");
  data->the_romeao = engine::texture::load_rgba8("assets/texture/romeao.png");
}

void _init_texture_images(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Some<lib::gpu_signal::Support> gpu_signal_support,
  usage::Full<VkQueue> queue_work,
  usage::Full<lib::Task> signal,
  usage::Full<Data> data 
) {
  ZoneScoped;

  { ZoneScopedN("semaphore_finished");
    VkSemaphoreTypeCreateInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    };
    VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = &timeline_info,
    };
    {
      auto result = vkCreateSemaphore(
        core->device,
        &create_info,
        core->allocator,
        &data->semaphore_finished
      );
      assert(result == VK_SUCCESS);
    }
  }

  { ZoneScopedN("command_pool");
    VkCommandPoolCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = core->queue_family_index,
    };
    auto result = vkCreateCommandPool(
      core->device,
      &create_info,
      core->allocator,
      &data->command_pool
    );
    assert(result == VK_SUCCESS);
  }

   lib::gpu_signal::associate(
    gpu_signal_support.ptr,
    signal.ptr,
    core->device,
    data->semaphore_finished,
    1
  );

  lib::gfx::multi_alloc::init(
    &data->texture_staging_multi_alloc,
    {
      lib::gfx::multi_alloc::Claim {
        .info = {
          .buffer = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = VkDeviceSize(
               data->the_albedo.width *
               data->the_albedo.height *
               engine::texture::ALBEDO_TEXEL_SIZE
            ),
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          },
        },
        .memory_property_flags = VkMemoryPropertyFlagBits(0
          | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        ),
        .p_stake_buffer = &data->texture_staging_albedo,
      },
      lib::gfx::multi_alloc::Claim {
        .info = {
          .buffer = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = VkDeviceSize(
               data->the_normal.width *
               data->the_normal.height *
               engine::texture::NORMAL_TEXEL_SIZE
            ),
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          },
        },
        .memory_property_flags = VkMemoryPropertyFlagBits(0
          | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        ),
        .p_stake_buffer = &data->texture_staging_normal,
      },
      lib::gfx::multi_alloc::Claim {
        .info = {
          .buffer = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = VkDeviceSize(
               data->the_romeao.width *
               data->the_romeao.height *
               engine::texture::ROMEAO_TEXEL_SIZE
            ),
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          },
        },
        .memory_property_flags = VkMemoryPropertyFlagBits(0
          | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        ),
        .p_stake_buffer = &data->texture_staging_romeao,
      }
    },
    core->device,
    core->allocator,
    &core->properties.basic,
    &core->properties.memory
  );

  lib::gfx::multi_alloc::init(
    &data->texture_albedo_item.data.multi_alloc,
    { lib::gfx::multi_alloc::Claim {
      .info = {
        .image = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
          .imageType = VK_IMAGE_TYPE_2D,
          .format = engine::texture::ALBEDO_TEXTURE_FORMAT,
          .extent = {
            .width = uint32_t(data->the_albedo.width),
            .height = uint32_t(data->the_albedo.height),
            .depth = 1,
          },
          .mipLevels = data->the_albedo.computed_mip_levels,
          .arrayLayers = 1,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .tiling = VK_IMAGE_TILING_OPTIMAL,
          .usage = (0
            | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT // for mips
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT
          ),
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        },
      },
      .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .p_stake_image = &data->texture_albedo_item.data.stake,
    }},
    core->device,
    core->allocator,
    &core->properties.basic,
    &core->properties.memory
  );

  lib::gfx::multi_alloc::init(
    &data->texture_normal_item.data.multi_alloc,
    { lib::gfx::multi_alloc::Claim {
      .info = {
        .image = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
          .imageType = VK_IMAGE_TYPE_2D,
          .format = engine::texture::NORMAL_TEXTURE_FORMAT,
          .extent = {
            .width = uint32_t(data->the_normal.width),
            .height = uint32_t(data->the_normal.height),
            .depth = 1,
          },
          .mipLevels = data->the_normal.computed_mip_levels,
          .arrayLayers = 1,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .tiling = VK_IMAGE_TILING_OPTIMAL,
          .usage = (0
            | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT // for mips
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT
          ),
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        },
      },
      .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .p_stake_image = &data->texture_normal_item.data.stake,
    }},
    core->device,
    core->allocator,
    &core->properties.basic,
    &core->properties.memory
  );

  lib::gfx::multi_alloc::init(
    &data->texture_romeao_item.data.multi_alloc,
    { lib::gfx::multi_alloc::Claim {
      .info = {
        .image = {
          .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
          .imageType = VK_IMAGE_TYPE_2D,
          .format = engine::texture::ROMEAO_TEXTURE_FORMAT,
          .extent = {
            .width = uint32_t(data->the_romeao.width),
            .height = uint32_t(data->the_romeao.height),
            .depth = 1,
          },
          .mipLevels = data->the_romeao.computed_mip_levels,
          .arrayLayers = 1,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .tiling = VK_IMAGE_TILING_OPTIMAL,
          .usage = (0
            | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT // for mips
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT
          ),
          .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        },
      },
      .memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      .p_stake_image = &data->texture_romeao_item.data.stake,
    }},
    core->device,
    core->allocator,
    &core->properties.basic,
    &core->properties.memory
  );

  {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = data->texture_albedo_item.data.stake.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = engine::texture::ALBEDO_TEXTURE_FORMAT,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = data->the_albedo.computed_mip_levels,
        .layerCount = 1,
      },
    };
    auto result = vkCreateImageView(
      core->device,
      &create_info,
      core->allocator,
      &data->texture_albedo_item.data.view
    );
    assert(result == VK_SUCCESS);
  }
  {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = data->texture_normal_item.data.stake.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = engine::texture::NORMAL_TEXTURE_FORMAT,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = data->the_normal.computed_mip_levels,
        .layerCount = 1,
      },
    };
    auto result = vkCreateImageView(
      core->device,
      &create_info,
      core->allocator,
      &data->texture_normal_item.data.view
    );
    assert(result == VK_SUCCESS);
  }
  {
    VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = data->texture_romeao_item.data.stake.image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = engine::texture::ROMEAO_TEXTURE_FORMAT,
      .subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = data->the_romeao.computed_mip_levels,
        .layerCount = 1,
      },
    };
    auto result = vkCreateImageView(
      core->device,
      &create_info,
      core->allocator,
      &data->texture_romeao_item.data.view
    );
    assert(result == VK_SUCCESS);
  }

  prepare_textures(
    core.ptr,
    *queue_work,
    data.ptr
  );
}

void _insert_items(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::GuidCounter> guid_counter,
  usage::Full<SessionData::Scene> scene,
  usage::Full<SessionData::Vulkan::Meshes> meshes,
  usage::Full<SessionData::Vulkan::Textures> textures,
  usage::Full<Data> data 
) {
  ZoneScoped;
  auto albedo_id = lib::guid::next(guid_counter.ptr);
  auto normal_id = lib::guid::next(guid_counter.ptr);
  auto romeao_id = lib::guid::next(guid_counter.ptr);
  textures->items.insert({ albedo_id, data->texture_albedo_item });
  textures->items.insert({ normal_id, data->texture_normal_item });
  textures->items.insert({ romeao_id, data->texture_romeao_item });
  scene->items.push_back(SessionData::Scene::Item {
    .group_id = data->group_id,
    .transform = glm::mat4(1.0f),
    .mesh_id = data->mesh_id,
    .texture_albedo_id = albedo_id,
    .texture_normal_id = normal_id,
    .texture_romeao_id = romeao_id,
  });
}

void _cleanup_textures(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<Data> data 
) {
  ZoneScoped;

  // @Note: can do this earlier in a separate task
  stbi_image_free(data->the_albedo.data);
  stbi_image_free(data->the_normal.data);
  stbi_image_free(data->the_romeao.data);

  lib::gfx::multi_alloc::deinit(
    &data->texture_staging_multi_alloc,
    core->device,
    core->allocator
  );

  vkDestroyCommandPool(
   core->device,
   data->command_pool,
   core->allocator
  );

  vkDestroySemaphore(
   core->device,
   data->semaphore_finished,
   core->allocator
  );

}

void _finish(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Some<SessionData::UnfinishedYarns> unfinished_yarns,
  usage::Full<SessionData::Groups> groups,
  usage::None<lib::Task> yarn,
  usage::Full<Data> data 
) {
  ZoneScoped;

  for (size_t i = 0; i < groups->items.size(); i++) {
    if (groups->items[i].group_id == data->group_id) {
      assert(groups->items[i].status == SessionData::Groups::Status::Loading);
      groups->items[i].status = SessionData::Groups::Status::Ready;
    }
  }

  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.erase(yarn.ptr);
  }

  task::signal(ctx->runner, yarn.ptr);

  delete data.ptr;
}

struct UnloadData {
  lib::GUID group_id;
};

void _unload(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<UnloadData> unload_data,
  usage::Some<SessionData::UnfinishedYarns> unfinished_yarns,
  usage::Some<SessionData::Vulkan::Core> core,
  usage::Full<SessionData::Scene> scene,
  usage::Full<SessionData::Vulkan::Meshes> meshes,
  usage::Full<SessionData::Vulkan::Textures> textures,
  usage::None<lib::Task> yarn
) {
  ZoneScoped;

  for (size_t i = 0; i < scene->items.size(); i++) {
    auto *item = &scene->items[i];
    if (item->group_id == unload_data->group_id) {
      engine::loading::mesh::deref(
        item->mesh_id,
        core,
        meshes
      );

      *item = scene->items.back();
      scene->items.pop_back();
      i--;
    }
  }

  // XXX wrong iteration
  // @Note: this duplicates code in session_cleanup (.textures, .meshes)
  for (auto &item : textures->items) {
    vkDestroyImageView(
      core->device,
      item.second.data.view,
      core->allocator
    );

    lib::gfx::multi_alloc::deinit(
      &item.second.data.multi_alloc,
      core->device,
      core->allocator
    );
  }

  // XXX wrong clear
  textures->items.clear();

  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.erase(yarn.ptr);
  }
  task::signal(ctx->runner, yarn.ptr);

  free(unload_data.ptr);
}

void load(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Use<SessionData::UnfinishedYarns> unfinished_yarns
) {
  ZoneScoped;
  auto yarn = task::create_yarn_signal();
  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.insert(yarn);
  }

  // @Note: we need to get rid of Data dependency in each task.
  // need to go finer-grained

   
  lib::GUID mesh_id = lib::guid::invalid;
  auto signal_mesh_loaded = engine::loading::mesh::load(
    ctx,
    session,
    &session->guid_counter,
    &mesh_id
  );

  auto data = new Data {
    .group_id = group_id,
    .mesh_id = mesh_id,
  };

  auto signal_init_texture_images = task::create_external_signal();
  auto task_read_texture_files = task::create(
    _read_texture_files,
    data
  );
  auto task_init_texture_images = task::create(
    defer,
    task::create(
      _init_texture_images,
      &session->vulkan.core,
      &session->gpu_signal_support,
      &session->vulkan.queue_work,
      signal_init_texture_images,
      data
    )
  );
  auto task_insert_items = task::create(
    defer,
    task::create(
      _insert_items,
      &session->guid_counter,
      &session->scene,
      &session->vulkan.meshes,
      &session->vulkan.textures,
      data
    )
  );
  auto task_cleanup_textures = task::create(
    defer,
    task::create(
      _cleanup_textures,
      &session->vulkan.core,
      data
    )
  );
  auto task_finish = task::create(
    defer,
    task::create(
      _finish,
      unfinished_yarns.ptr,
      &session->groups,
      yarn,
      data
    )
  );
  ctx->new_tasks.insert(ctx->new_tasks.end(), {
    task_read_texture_files,
    task_init_texture_images,
    task_insert_items,
    task_cleanup_textures,
    task_finish,
  });
  ctx->new_dependencies.insert(ctx->new_dependencies.end(), {
    { task_read_texture_files, task_init_texture_images },
    { signal_init_texture_images, task_cleanup_textures },
    { task_cleanup_textures, task_finish },
    { signal_mesh_loaded, task_insert_items },
    { signal_init_texture_images, task_insert_items },
    { task_insert_items, task_finish },
  });
}

void unload(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Use<SessionData::UnfinishedYarns> unfinished_yarns,
  Use<RenderingData::InflightGPU> inflight_gpu
) {
  auto yarn = task::create_yarn_signal();
  {
    std::scoped_lock lock(unfinished_yarns->mutex);
    unfinished_yarns->set.insert(yarn);
  }
  auto data = new UnloadData {
    .group_id = group_id,
  };
  auto task_unload = task::create(
    _unload,
    data,
    unfinished_yarns.ptr,
    &session->vulkan.core,
    &session->scene,
    &session->vulkan.meshes,
    &session->vulkan.textures,
    yarn
  );
  {
    std::scoped_lock lock(inflight_gpu->mutex);
    std::vector<std::pair<lib::Task *, lib::Task *>> dependencies;
    if (inflight_gpu.ptr != nullptr) {
      for (auto signal : inflight_gpu->signals) {
        dependencies.push_back({ signal, task_unload });
      }
    }
    task::inject(ctx->runner, {
      task_unload
    }, {
      .new_dependencies = dependencies,
    });
  }
}

void reload(
  lib::task::ContextBase *ctx,
  lib::GUID group_id,
  Ref<SessionData> session,
  Use<SessionData::UnfinishedYarns> unfinished_yarns,
  Use<RenderingData::InflightGPU> inflight_gpu
) {
  unload(
    ctx,
    group_id,
    session,
    unfinished_yarns,
    inflight_gpu
  );

  // @Hack: `unload` just injects one task, and it's use of resources
  // guarantees that loading will happen afterwards and not conflict.

  load(
    ctx,
    group_id,
    session,
    unfinished_yarns
  );
}

} // namespace