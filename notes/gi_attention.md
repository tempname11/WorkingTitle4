# GI, again

- memory usage is insane
  - we do not need (inflight-count) of everything. what can we slice?
  - research sparse textures ???
  * use queues and buffers, not flags and textures ?

- attentions lags 1 frame behind in terms of coordinates
- newly-wrapped grid values should be reset and have attention.
- grid boundaries still look bad.
- "weird circle" still pops up

* try atomic-based attention strategies:
  - only add to attention if weight > 0
  - keep binary value, but pack more into 32 bits
  - accumulate, and atomic add value based on weight / source
