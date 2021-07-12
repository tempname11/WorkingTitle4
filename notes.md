### currently
- normal map
- roughness-metallic-AO map 

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

### minor problems
- fullscreen toggle: weird visual jump