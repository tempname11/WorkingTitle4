#pragma once
#include <src/global.hxx>
#include <src/lib/gfx/allocator.hxx>
#include <src/engine/constants.hxx>
#include <src/engine/display/data.hxx>
#include <src/engine/session/data.hxx>
#include "data.hxx"

namespace engine::datum::probe_workset {

void barrier_from_write_to_read(
  SData *it,
  Ref<engine::display::Data::FrameInfo> frame_info,
  VkCommandBuffer cmd
) {
  ZoneScoped;

  VkBufferMemoryBarrier barriers[2 * PROBE_CASCADE_COUNT];
  for (size_t c = 0; c < PROBE_CASCADE_COUNT; c++) {
    barriers[c * 2 + 0] = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .buffer = it->buffers_counter[c].buffer,
      .offset = 0,
      .size = VK_WHOLE_SIZE,
    };
    barriers[c * 2 + 1] = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .buffer = it->buffers_workset[c].buffer,
      .offset = 0,
      .size = VK_WHOLE_SIZE,
    };
  };
  vkCmdPipelineBarrier(
    cmd,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0,
    0, nullptr,
    sizeof(barriers) / sizeof(*barriers), barriers,
    0, nullptr
  );
}

} // namespace
