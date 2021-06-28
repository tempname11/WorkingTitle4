### more things to do
[ ] fix the intermittent bug?

### refactoring
[ ] move engine/rendering/** -> engine/**
[ ] move runtime stuff to engine/**
[ ] move imgui stuff to engine/**

### code annoyances
* some files are too large, target is 500 (?) LOC at most
* initialize structs with "{ ... }", not ".member = ..."
* GLSL includes?

### architectural problems
* semaphores are used unnecessarily to separate work <-> imgui <-> compose
* task scheduling is dynamic and costly, but it shouldn't really be