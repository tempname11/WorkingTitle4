### known bugs
- artifacts on top and left edge of viewport
- specular highlights seem to be "biased"
    as if only on one side of object, when it should be even.

### architectural problems
- uploader seems very rushed, need to come back to it from a performance perspective
- also need automatic GPU memory region compacting

- error handing for file reads is hairy.
    consider a set of read helpers that provide zero output on error and go on
    the error is then checked once at the end.

- custom CPU allocators (frame, "short-term")
- resource aliasing seems net harmful. 
- lib::task: avoid so many allocations
- semaphores are used unnecessarily to separate work <-> imgui <-> compose

### deprecations
- Ref/Use/Own is moot for mutex-protected data, so should use Ref everywhere
    (grep "Some<SessionData" and the like)
- task::inject seems net harmful, should deprecate it.
- subtasks don't seem useful, remove them?
- multi_alloc?

### good to have
- tools should be migrated inside engine!

### minor issues
- rand() is not thread-safe, need to use something different.

- memory usage seems to increase over time,
    but Tracy profiling shows everything is great.
    maybe these are external (DLL?) allocations and we don't track them.

- when running the Vulkan Configurator overrides, some additional errors pop up.
    need to enable their presence by default, and also fix the actual errors.

- fullscreen toggle: weird visual jump
- fullscreen toggle: always primary monitor (want the one which the window "belongs" to)
- texture mip levels are generated at runtime

### refactoring
- engine/** file naming is a bit of a mess