### architectural problems
- Need `readonly`/`writeonly`/other qualifiers for perf.
- Need automatic GPU memory region compacting
- Uploader seems very @Rushed, need to come back to it from a performance perspective
- Semaphores are used unnecessarily to separate `work` <-> `imgui` <-> `compose`.
- Render passes are not really used, but they should be. :UseRenderPasses
- Some compute workgroups are size 1 which is not good for perf. :UseComputeLocalSize
- "lpass" should be split info a few different passes :ManyLights
- Descriptor pool counts are fixed; can run out of them. :FixedDescriptorPool
- Custom CPU allocators (frame, "short-term") :Better_CPU_Allocators
- Way too many allocations in  `lib::task` implementation.
- All file IO assumes little endian.
- Texture mip levels are generated at runtime.
- No texture compression is used.

### deprecations
- Resource aliasing seems net harmful. 
- Subtasks don't seem useful, remove them. Yarns do it better.
- Ref/Use/Own is moot for mutex-protected data, so should use Ref everywhere.
- `rand()` is not thread-safe generally (although it is in MSVC).
- `task::inject` seems net harmful. (can we really avoid it?)

### minor issues
- fullscreen toggle: weird visual jump
- fullscreen toggle: always primary monitor (want the one which the window "belongs" to)
- some file read operations assert on bad input :HandleInputGracefully
- TAA shimmer for small pixel movement
- Motion Blur ghosting

- eye adaptation is heavily biased towards edge of viewport
  (because of vkCmdBlit clamping and non-power-of-2 sizes) 

- When running the Vulkan Configurator overrides, some additional errors pop up.
  (Need to enable their presence by default, and also fix the actual errors.)

- all VkSubmitInfo `pWaitDstStageMask`-s are conservative, what should they actually be?

### good to have
- Separate reference raytracing pass.
- BLAS compaction.
- Async running tools, ideally with an in-progress task UI view.
- All disk paths should be relative to *whatever is referencing them*.
- Should also normalize disk paths after user input.
- interactive scene editor (selection, translate-rotate-scale)

### GI: work to be done
- investigate "self-lighting"
- shadows still have "bands" which seem related to "seams" in the octomap.
- use texture LODs
- try "lazy" probe maps.
- blend cascade levels.