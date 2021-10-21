# GI attention

- only write attention if weight > 0
- tag r32ui in shaders

- wraparound!

- pack into 32 bits? 
- or introduce gradual attention instead?
- 16 bit probe_map?
- 2 pixel border -> 1