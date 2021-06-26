### more things to do
[ ] remove "example" from naming
[ ] move session code to engine/session**
[ ] move rendering code to engine/rendering/**
[ ] fix the intermittent bug
[ ] linear_to_srgb?

### find out
[ ] can't we create graphics pipelines and render passes before knowing the swapchain info?
[ ] how do push constants work?

### code annoyances
* some files are too large, target is 500 LOC at most
* initialize structs with "{ ... }", not ".member = ..."
* no GLSL includes?

### architectural problems
* task scheduling is dynamic and costly, but it shouldn't really be