#include <vulkan/vulkan.h>
#include <src/lib/task.hxx>
#include <src/engine/uploader.hxx>
#include "private.hxx"

namespace engine::system::artline {

void _upload_texture(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<VkFormat> format,
  Ref<engine::common::texture::Data<uint8_t>> data,
  Ref<engine::session::Vulkan::Textures::Item> texture_item,
  Ref<lib::Task> signal,
  Own<VkQueue> queue_work,
  Ref<engine::session::Data> session
) {
  ZoneScoped;
  auto core = &session->vulkan.core;

  VkImageCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = VK_IMAGE_TYPE_2D,
    .format = *format,
    .extent = {
      .width = std::max(
        uint32_t(1), // for invalid image
        uint32_t(data->width)
      ),
      .height = std::max(
        uint32_t(1), // for invalid image
        uint32_t(data->height)
      ),
      .depth = 1,
    },
    .mipLevels = std::max(
      uint32_t(1), // for invalid image
      data->computed_mip_levels
    ),
    .arrayLayers = 1,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  auto result = engine::uploader::prepare_image(
    &session->vulkan.uploader,
    core->device,
    core->allocator,
    &core->properties.basic,
    &create_info
  );
  texture_item->id = result.id;
  if (data->data != nullptr) {
    memcpy(
      result.mem,
      data->data,
      result.data_size
    );
  } else {
    memset(
      result.mem,
      0,
      result.data_size
    );
  }

  engine::uploader::upload_image(
    ctx,
    signal.ptr,
    &session->vulkan.uploader,
    session.ptr,
    queue_work,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    result.id
  );
}

} // namespace
