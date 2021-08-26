#include <src/task/defer.hxx>
#include <src/engine/uploader.hxx>
#include "../mesh.hxx"
#include "common.hxx"
#include "load.hxx"

namespace engine::loading::mesh {

void _load_read_file(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Own<LoadData> data 
) {
  ZoneScoped;
  data->the_mesh = read_t06_file(data->path.c_str());
}

void _load_init_buffer(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<SessionData> session,
  Use<SessionData::Vulkan::Core> core,
  Own<VkQueue> queue_work,
  Ref<lib::Task> signal,
  Own<LoadData> data 
) {
  ZoneScoped;

  data->mesh_item.index_count = data->the_mesh.index_count;
  data->mesh_item.buffer_offset_indices = 0;
  data->mesh_item.buffer_offset_vertices = data->the_mesh.index_count * sizeof(engine::common::mesh::IndexT06);
  VkBufferCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = std::max(
      size_t(1), // for invalid buffer
      (0
        + data->the_mesh.index_count * sizeof(engine::common::mesh::IndexT06)
        + data->the_mesh.vertex_count * sizeof(engine::common::mesh::VertexT06)
      )
    ),
    .usage = (0
      | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
      | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    ),
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  auto result = engine::uploader::prepare_buffer(
    &session->vulkan.uploader,
    core->device,
    core->allocator,
    &core->properties.basic,
    &create_info
  );

  data->mesh_item.id = result.id;
  if(data->the_mesh.index_count > 0) {
    memcpy(
      result.mem,
      data->the_mesh.indices,
      result.data_size
    );
  }

  engine::uploader::upload_buffer(
    ctx,
    signal.ptr,
    &session->vulkan.uploader,
    session.ptr,
    &session->vulkan.core,
    &session->gpu_signal_support,
    queue_work,
    result.id
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
  meta->invalid = data->mesh_item.index_count == 0;

  lib::lifetime::deref(&session->lifetime, ctx->runner);

  deinit_t06(&data->the_mesh);

  delete data.ptr;
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
  auto signal_init_buffer = lib::task::create_external_signal();
  auto task_init_buffer = defer(
    lib::task::create(
      _load_init_buffer,
      session.ptr,
      &session->vulkan.core,
      &session->vulkan.queue_work,
      signal_init_buffer,
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
    { signal_init_buffer, task_finish.first },
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
