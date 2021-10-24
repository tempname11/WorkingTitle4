# Global Illumination TODO

## known bugs
- memory usage is too big (see stream_plan.md)
- grid boundaries still look bad. (see stream_plan.md)
- "weird circle" still pops up
- [minor] "self-lighting"
- no texture LODs, will probably affect performance most.

## harder tasks
* try atomic-based attention strategies:
  - [basic] only add to attention if weight > 0
  - [promising] accumulate, and atomic add value based on weight / source

* try to make use of "confidence" in A16? (but it must decay over time)
* try compressing attention flags into queues and only processing them.

*** research sparse textures more.
