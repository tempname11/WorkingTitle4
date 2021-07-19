#include "frame_cleanup.hxx"

TASK_DECL {
  ZoneScoped;
  ctx->changed_parents = {
    { .ptr = frame_data.ptr, .children = {} }
  };

  delete frame_info.ptr;
  delete frame_data.ptr;
}
