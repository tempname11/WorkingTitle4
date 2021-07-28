### known bugs
- artifacts on top and left edge of viewport
- specular highlights seem to be "biased"
    as if only on one side of object, when it should be even.

### architectural problems
- unfinished yarns could be just an atomic counter with last-will-signal.
- task::inject seems net harmful, should deprecate it.
- resource aliasing seems net harmful. 
- lib::task, avoid so many allocations
- semaphores are used unnecessarily to separate work <-> imgui <-> compose
- texture mip levels are generated at runtime

### minor issues
- memory usage seems to increase over time,
    but Tracy profiling shows everything is great.
    maybe these are external (DLL?) allocations and we don't track them.

- when running the Vulkan Configurator overrides, some additional errors pop up.
    need to enable their presence by default, and also fix the actual errors.

- fullscreen toggle: weird visual jump

### refactoring
- InflightGPU would be easier to use inside SessionData
- engine/** file naming is a bit of a mess
- subtasks don't seem useful, remove them?
- make defer a function?