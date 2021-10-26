# Global Illumination TODO

## Known bugs
- Grid boundary values jump too much on movement.
- Cascade boundaries are visible.
- No texture LODs, will manifest in aliasing and perf.

## Harder tasks
* Try atomic-based attention strategies:
  - [basic] only add to attention if weight > 0
  - [promising] accumulate, and atomic add value based on weight / source

* Try to make use of "confidence" in A16? (but it must decay over time)
* Try compressing attention flags into queues and only processing them.
