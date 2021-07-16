#include "frame_cleanup.hxx"

TASK_DECL {
  ZoneScoped;
  delete frame_info.ptr;
}
