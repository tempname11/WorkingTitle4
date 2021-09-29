# known bugs
- models with more than 2^16 vertices are not imported correctly

# intermittent bugs
- `assert( m_head != m_tail );` in TracyVulkan.hpp
  (seems correlated with number of Vulkan warnings beforehand.)
  (this is now hidden under TRACY_VULKAN_ENABLE and disabled by default)

### architectural problems
- render passes are not really used, but they should be. :UseRenderPasses
- some compute workgroups are size 1 which is not good for perf. :UseComputeLocalSize
- "lpass" should be split info a few different passes :ManyLights
- descriptor pool counts are fixed; can run out of them.
- uploader seems very rushed, need to come back to it from a performance perspective
- also need automatic GPU memory region compacting
- also need multi-alloc-like capability for `allocator`
- allocators probably should be top-level, not nested inside other structures
- custom CPU allocators (frame, "short-term")
- lib::task: avoid so many allocations
- semaphores are used unnecessarily to separate work <-> imgui <-> compose
- all file IO assumes little endian.

- error handing for file reads is currently hairy.
  consider a set of read helpers that provide zero output on error and go on.
  the error is then checked once at the end.

### deprecations
- resource aliasing seems net harmful. 
- task::inject seems net harmful.
- subtasks don't seem useful, remove them?
- `multi_alloc` (in favor of `allocator`)

- Ref/Use/Own is moot for mutex-protected data, so should use Ref everywhere
  (grep "Some<SessionData" and the like)

### good to have
- debug pass (copy user-chosen intermediate data to swapchain)
- separate reference raytracing pass.
- BLAS compaction.
- async running tools, ideally with an in-progress task UI view.
- all disk paths should be relative to *whatever is referencing them*
- should also normalize disk paths after user input

### minor issues
- move ad-hoc inline constants to uniform or at least `common/constants.glsl` :MoveToUniform
- r11g11b10 probe_light_map has yellow tint on saturation
- rand() is not thread-safe, need to replace their uses.
- fullscreen toggle: weird visual jump
- fullscreen toggle: always primary monitor (want the one which the window "belongs" to)
- texture mip levels are generated at runtime

- many allocators are hard to track, both conceptually, and in imgui.
  (and currently not all of them are displayed!)

- memory usage seems to increase over time,
  but Tracy profiling shows everything is great.
  maybe these are external (DLL?) allocations and we don't track them.

- when running the Vulkan Configurator overrides, some additional errors pop up.
  need to enable their presence by default, and also fix the actual errors.

### refactoring
- the directory `src/task` should not be there. tasks should be where they logically belong.
- session/setup does too much and should be split up into smaller files
- worker_count (and flags?) should be accessible from the task `ctx`

- split off `display::Data` init from `session_iteration_try_rendering`
  (and use `*data = {...};` init style)

### return to this with fresh ideas
- direct lighting calibration
  (in particular, the gloss on the roof at Sponza is very suspicious)