#include "task.hxx"

void rendering_frame_cleanup(
  task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  usage::Full<RenderingData::FrameInfo> frame_info
) {
  ZoneScoped;
  delete frame_info.ptr;
}
