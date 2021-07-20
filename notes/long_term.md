### refactoring
- move engine/rendering/** -> engine/**
- move runtime stuff to engine/**
- move imgui stuff to engine/**

### code annoyances
- initialize structs with "{ ... }", not ".member = ..."

### architectural problems
- semaphores are used unnecessarily to separate work <-> imgui <-> compose
- task scheduling is dynamic and costly, but it shouldn't really be
- lib::task: avoid so many allocations

### other weird stuff
- memory usage seems to increase over time, but Tracy profiling shows everything is great.
  maybe these are external (DLL?) allocations and we don't track them.
- when running the Vulkan Configurator overrides, some additional errors pop up.
  need to enable their presence by default, and also fix the actual errors.

### known bugs
- artifacts on top and left edge of viewport
- specular highlights seem to be "biased" (only on one symmetric side of object, although light is even)

### minor problems
- fullscreen toggle: weird visual jump