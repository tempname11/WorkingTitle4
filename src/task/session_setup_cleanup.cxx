#include <stb_image.h>
#include "session_setup_cleanup.hxx"

TASK_DECL {
  ZoneScoped;

  // @Note: we can do this earlier, after copy to buffer
  stbi_image_free(data->albedo.data);

  lib::gfx::multi_alloc::deinit(
    &data->multi_alloc,
    core->device,
    core->allocator
   );

   vkDestroyCommandPool(
     core->device,
     data->command_pool,
     core->allocator
   );
}
