### horizon
- loading: performance
- loading: memory allocation
- basic scene editor capabilities?

### known bugs
- artifacts on top and left edge of viewport
- specular highlights seem to be "biased"
    as if only on one side of object, when it should be even.

### nasty intermittent bugs
- x3 [last 2021-08-06] session not deinitialized, some dependency (yarn?) was stuck
  (probably fixed, in mesh::load)

- x3 [last 2021-07-30] mesh reload crash, the buffer seems to be freed and then used.
  (likely not relevant anymore)

   
### architectural problems
- error handing for file reads is hairy
  consider a set of read helpers that provide zero output on error and go on
  the error is then checked once at the end.

- custom allocators (frame, "short-term")
- memory suballocation for mesh/texture
- resource aliasing seems net harmful. 
- lib::task, avoid so many allocations
- semaphores are used unnecessarily to separate work <-> imgui <-> compose

### deprecations
- Ref/Use/Own is moot for mutex-protected data, so should use Ref everywhere
- task::inject seems net harmful, should deprecate it.
  (grep "Some<SessionData>" and the like)

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
- subtasks don't seem useful, remove them?
- engine/** file naming is a bit of a mess