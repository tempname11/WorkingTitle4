#include "cleanup.hxx"

namespace engine::frame {

void cleanup(
  lib::task::Context<QUEUE_INDEX_LOW_PRIORITY> *ctx,
  Ref<engine::display::Data::FrameInfo> frame_info,
  Ref<engine::misc::FrameData> frame_data
) {
  ZoneScoped;
  ctx->changed_parents = {
    { .ptr = frame_data.ptr, .children = {} }
  };

  delete frame_info.ptr;
  delete frame_data.ptr;
}

} // namespace
