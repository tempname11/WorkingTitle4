# Global Illumination TODO

## memory usage is insane
  - see `stream_plan.md`

## small improvements
- can we use wrapping in probe map, so that we don't need to copy old -> new with no attention?
- attention lags 1 frame behind in terms of coordinates.
- newly-wrapped grid values should be reset and have attention.
- no texture LODs, will probably affect performance most.

## known bugs
- grid boundaries still look bad.
- "weird circle" still pops up
- [minor] "self-lighting"

## harder tasks
* try atomic-based attention strategies:
  - only add to attention if weight > 0 (basic)
  - accumulate, and atomic add value based on weight / source (promising)

* try compressing attention flags into queues and only processing them.

*** research sparse textures more.
