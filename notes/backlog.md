# missing features
- Need proper support for multiple lights. :ManyLights

# need to check
- Why is nSight hanging when trying to profile?
- When running the Vulkan Configurator overrides, some additional errors/warnings
    pop up. Need to enable their presence by default, and also fix them.

# architectural problems
- Combine LPass parts to a render pass :UseRenderPasses. Also :IndirectFirst.
- Need automatic GPU memory region compacting.
- Semaphores are used unnecessarily to separate `work` <-> `imgui` <-> `compose`.
- We do not need (inflight-count) of every datum.
- Descriptor pool counts are fixed; can run out of them. :FixedDescriptorPool
- Custom CPU allocators (frame, "short-term") :Better_CPU_Allocators
- Way too many allocations in  `lib::task` implementation.
- All file IO assumes little endian.
- When ray tracing, no texture LODs, will manifest in aliasing and perf.

# minor performance
- VkSubmitInfo `pWaitDstStageMask`-s are conservative, what should they actually be?
- BLAS compaction.
- No texture compression is used (this might be big?)
- Texture mip levels are generated at runtime.

# deprecations
- `rand()` is not thread-safe generally (although it is in MSVC).

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
