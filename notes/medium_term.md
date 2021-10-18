### general issues
- Import models with more than 2^16 vertices. :NotEnoughIndices

- Memory usage seems to increase over time!?
  (but Tracy profiling shows everything is great!?)
  (maybe these are external (DLL?) allocations and we don't track them.)

- Over time, performance drops, but swapchain recreation (via window resize) helps.

### architectural problems
- Render passes are not really used, but they should be. :UseRenderPasses
- Some compute workgroups are size 1 which is not good for perf. :UseComputeLocalSize
- "lpass" should be split info a few different passes :ManyLights
- Descriptor pool counts are fixed; can run out of them. :FixedDescriptorPool
- Uploader seems very @Rushed, need to come back to it from a performance perspective
- Need automatic GPU memory region compacting
- Need multi-alloc-like capability for `allocator`
- Allocators should be top-level, not nested inside other structures :Sensible_GPU_Allocators
- Custom CPU allocators (frame, "short-term") :Better_CPU_Allocators
- Way too many allocations in  `lib::task` implementation.
- Semaphores are used unnecessarily to separate `work` <-> `imgui` <-> `compose`.
- All file IO assumes little endian.
- Texture mip levels are generated at runtime.
- No texture compression is used.

### deprecations
- Resource aliasing seems net harmful. 
- `task::inject` seems net harmful.
- Subtasks don't seem useful, remove them?
- `rand()` is not thread-safe generally (although it is in MSVC).

- `multi_alloc` (in favor of `allocator`).
  (Although it would be good to allocate many things without taking mutex every time.)

- Ref/Use/Own is moot for mutex-protected data, so should use Ref everywhere.
  (Grep "Some<SessionData" and the like.)

### minor issues
- r11g11b10 probe_light_map has yellow tint on saturation
- fullscreen toggle: weird visual jump
- fullscreen toggle: always primary monitor (want the one which the window "belongs" to)
- not all allocators are displayed in UI :Sensible_GPU_Allocators
- some file read operations assert on bad input :HandleInputGracefully

- eye adaptation is heavily biased towards edge of viewport
  (because of vkCmdBlit clamping and non-power-of-2 sizes) 

- When running the Vulkan Configurator overrides, some additional errors pop up.
  (Need to enable their presence by default, and also fix the actual errors.)

- intermittent `assert( m_head != m_tail );` in TracyVulkan.hpp
  (seems correlated with number of Vulkan warnings beforehand.)
  (this is now hidden under TRACY_VULKAN_ENABLE and disabled by default)

### refactoring
- Primary LBuffer: indirect light first. :IndirectFirst
- Move ad-hoc inline constants to uniform or at least `common/constants.glsl` :MoveToUniform
- The directory `src/task` should not be there. tasks should be where they logically belong.
- `session/setup.cxx` does too much and should be split up into smaller files.

- split off `display::Data` init from the huge `session_iteration_try_rendering`.
  (And use `*data = {...};` init style.)

- Error handing for file reads is currently hairy.
  (Consider a set of read helpers that provide zero output on error and go on...)
  (... with the error then checked once at the end.)

### good to have
- Debug pass (copy user-chosen intermediate data to swapchain).
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
- reference ray tracer.
- try "lazy" probe maps.
- blend cascade levels.