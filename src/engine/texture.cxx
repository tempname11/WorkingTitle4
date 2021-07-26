#include <stb_image.h>
#include <src/global.hxx>
#include <src/lib/gfx/utilities.hxx>
#include "texture.hxx"

namespace engine::texture {

engine::common::texture::Data<uint8_t> load_rgba8(const char *filename) {
  int width, height, _channels = 4;
  auto data = stbi_load(filename, &width, &height, &_channels, 4);
  assert(data != nullptr);
  return engine::common::texture::Data<uint8_t> {
    .data = data,
    .width = width,
    .height = height,
    .channels = 4,
    .computed_mip_levels = lib::gfx::utilities::mip_levels(width, height),
  };
}

void prepare(
  size_t texel_size,
  engine::common::texture::Data<uint8_t> *data,
  engine::common::texture::GPU_Data *gpu_data,
  lib::gfx::multi_alloc::StakeBuffer *staging_stake,
  SessionData::Vulkan::Core *core,
  VkCommandBuffer cmd,
  VkQueue queue_work
) {
  ZoneScoped;
  { ZoneScopedN("copy_to_buffer");
    void *mem = nullptr;
    auto result = vkMapMemory(
      core->device,
      staging_stake->memory,
      staging_stake->offset,
      staging_stake->size,
      0, &mem
    );
    assert(result == VK_SUCCESS);
    memcpy(
      mem,
      data->data,
      texel_size
        * data->width
        * data->height
    );
    vkUnmapMemory(core->device, staging_stake->memory);
  }

  { TracyVkZone(core->tracy_context, cmd, "prepare_textures");
    { // transition before copy
      VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = gpu_data->stake.image,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = data->computed_mip_levels,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      };

      vkCmdPipelineBarrier(
        cmd, 
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
      );
    }

    { // copy
      VkBufferImageCopy region = {
        .imageSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .layerCount = 1,
        },
        .imageExtent = {
          .width = uint32_t(data->width),
          .height = uint32_t(data->height),
          .depth = 1,
        },
      };

      vkCmdCopyBufferToImage(
        cmd,
        staging_stake->buffer,
        gpu_data->stake.image,
        VK_IMAGE_LAYOUT_GENERAL,
        1, &region
      );
    }

    { // generate mip levels
      auto w = int32_t(data->width);
      auto h = int32_t(data->height);

      for (uint32_t m = 0; m < data->computed_mip_levels - 1; m++) {
        auto next_w = w > 1 ? w / 2 : 1;
        auto next_h = h > 1 ? h / 2 : 1;
        VkImageBlit blit = {
          .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = m,
            .layerCount = 1,
          },
          .srcOffsets = {
            {},
            { .x = w, .y = h, .z = 1, },
          },
          .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = m + 1,
            .layerCount = 1,
          },
          .dstOffsets = {
            {},
            { .x = next_w, .y = next_h, .z = 1, },
          },
        };
        vkCmdBlitImage(
          cmd,
          gpu_data->stake.image, VK_IMAGE_LAYOUT_GENERAL,
          gpu_data->stake.image, VK_IMAGE_LAYOUT_GENERAL,
          1, &blit,
          VK_FILTER_LINEAR
        );
        w = next_w;
        h = next_h;
      }
    }

    { // transition after copy
      VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = gpu_data->stake.image,
        .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = data->computed_mip_levels,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      };

      // VK_PIPELINE_STAGE_ALL_COMMANDS here means:
      //  we don't know who will be using this texture.
      vkCmdPipelineBarrier(
        cmd, 
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
      );
    }
  }
}

} // namespace
