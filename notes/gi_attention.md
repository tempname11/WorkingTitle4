# GI attention

- wraparound!

- try atomic-based attention strategies:
  - only add to attention if weight > 0
  - keep binary value, but pack more into 32 bits
  - accumulate, and atomic add value based on weight / source

- 16 bit probe_map?
- 2 pixel border -> 1