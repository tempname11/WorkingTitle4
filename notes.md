### more things to do
[ ] why do weird artifacts appear?
[ ] fix the intermittent bug?
[ ] linear_to_srgb?

### find out
[ ] how do push constants work?

### refactoring
[ ] move engine/rendering/** -> engine/**
[ ] move runtime stuff to engine/**

### code annoyances
* some files are too large, target is 500 LOC at most
* initialize structs with "{ ... }", not ".member = ..."
* no GLSL includes?

### architectural problems
* semaphores are used unnecessarily to separate work <-> imgui <-> compose
* task scheduling is dynamic and costly, but it shouldn't really be