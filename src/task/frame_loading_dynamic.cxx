#include <src/engine/loading/simple.hxx>
#include "frame_loading_dynamic.hxx"

TASK_DECL {
  ZoneScoped;
  if (imgui_reactions->reload) {
    // @Note: the check is ridiculous and need to be rethought later.
    if (meshes->items.size() > 0) {
      engine::loading::simple::unload(
        ctx,
        unfinished_yarns.ptr,
        scene.ptr,
        core.ptr,
        inflight_gpu.ptr,
        meshes.ptr
      );
      // @Note: the order of calla matters here, the first call injects an unload task immediately,
      // so tasks that use same resources in the second one are sure to be called later.

      engine::loading::simple::load(
        ctx,
        unfinished_yarns.ptr,
        scene.ptr,
        core.ptr,
        meshes.ptr
      );
    }
  }
}
