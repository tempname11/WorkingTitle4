# need to check
- Do we also need barriers across synchronised submits, i.e. different frames?!
- When running the Vulkan Configurator overrides, some additional errors/warnings
    pop up. Need to enable their presence by default, and also fix them.

# architectural problems
- Need proper support for multiple lights. :ManyLights
- Combine LPass parts to a render pass :UseRenderPasses. Also :IndirectFirst.
- Need automatic GPU memory region compacting.
- Uploader need a performance-oriented overhaul :UploaderMustBeImproved
- Semaphores are used unnecessarily to separate `work` <-> `imgui` <-> `compose`.
- Descriptor pool counts are fixed; can run out of them. :FixedDescriptorPool
- Custom CPU allocators (frame, "short-term") :Better_CPU_Allocators
- Way too many allocations in  `lib::task` implementation.
- All file IO assumes little endian.

# minor performance
- Shaders need `readonly`/`writeonly`/other qualifiers for perf.
- VkSubmitInfo `pWaitDstStageMask`-s are conservative, what should they actually be?
- BLAS compaction.
- No texture compression is used (this might be big?)
- Texture mip levels are generated at runtime.

# deprecations
- Resource aliasing seems net harmful. 
- Subtasks don't seem useful, remove them. Yarns do it better.
- `rand()` is not thread-safe generally (although it is in MSVC).
- `task::inject` seems net harmful. (can we really avoid it? @Think)

# minor bugs
- TAA shimmer for small pixel movement
- Motion Blur ghosting
- fullscreen toggle: weird visual jump
- some file read operations assert on bad input :HandleInputGracefully
- Eye adaptation behaves weirdly. Probably is heavily biased towards edge
    of viewport, because of vkCmdBlit clamping and non-power-of-2 sizes.

# good to have
- Fullscreen toggle: we want to use the monitor which the window covers the most.
- Async tools, ideally with a simple in-progress task imgui view.
- All disk paths should be relative to whatever is referencing them.
    This will break current asset files.
    Don't forget to normalize disk paths after user input.
