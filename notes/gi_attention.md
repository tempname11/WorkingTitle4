# GI, again

-> memory usage is insane
  - see `plan.md`

- small improvements
  - can we use wrapping in probe map, so that we don't need to copy old -> new with no attention?
  - attention lags 1 frame behind in terms of coordinates.
  - newly-wrapped grid values should be reset and have attention.

- known bugs
  - grid boundaries still look bad.
  - "weird circle" still pops up

* try atomic-based attention strategies:
  - only add to attention if weight > 0 (basic)
  - accumulate, and atomic add value based on weight / source (promising)

* try compressing attention flags into queues and only processing them.

*** research sparse textures more.
