# known bugs
- crash when resizing to width > 2048 pixels
- models with more than 2^16 vertices are not imported correctly (should be split up?)

# intermittent bugs
- `assert( m_head != m_tail );` in TracyVulkan.hpp
  (seems correlated with number of Vulkan warnings beforehand.)

### architectural problems
- all compute workgroups are size 1 :UseComputeLocalSize
- "lpass" should be split info a few different passes
- descriptor set counts are fixed
- uploader seems very rushed, need to come back to it from a performance perspective
- also need automatic GPU memory region compacting
- also need multi-alloc-like capability for `allocator`
- allocators probably should be top-level, not nested inside other structures

- error handing for file reads is hairy.
  consider a set of read helpers that provide zero output on error and go on.
  the error is then checked once at the end.

- custom CPU allocators (frame, "short-term")
- resource aliasing seems net harmful. 
- lib::task: avoid so many allocations
- semaphores are used unnecessarily to separate work <-> imgui <-> compose
- all file IO assumes little endian.

### deprecations
- Ref/Use/Own is moot for mutex-protected data, so should use Ref everywhere
  (grep "Some<SessionData" and the like)

- task::inject seems net harmful, should deprecate it.
- subtasks don't seem useful, remove them?
- `multi_alloc` (in favor of `allocator`)

### good to have
- debug pass (copy intermediate data to swapchain)
- separate reference raytracing pass?
- BLAS compaction
- async tools, with an in-progress task GUI view.
- all paths should be relative to *whatever is referencing them*
- should also normalize paths on input

### minor issues
- can we share uniform block declarations (without macros) between different files.
  - they have to have different bindings!

- many allocators are hard to track, both conceptually, and in imgui.
- rand() is not thread-safe, need to use something different.

- memory usage seems to increase over time,
  but Tracy profiling shows everything is great.
  maybe these are external (DLL?) allocations and we don't track them.

- when running the Vulkan Configurator overrides, some additional errors pop up.
  need to enable their presence by default, and also fix the actual errors.

- fullscreen toggle: weird visual jump
- fullscreen toggle: always primary monitor (want the one which the window "belongs" to)
- texture mip levels are generated at runtime
- worker_count (and flags?) needs to be accessible from the task `ctx`

### refactoring
- the directory `src/task` should not be there. tasks should be where they logically belong.
- session/setup should be split up into smaller files

- split off `display::Data` init from `session_iteration_try_rendering`
  (and use `*data = {...};` init style)

### return to this with fresh ideas
- direct lighting calibration
  (in particular, the gloss on the roof at Sponza is very suspicious)