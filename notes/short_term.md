# Milestone: GI in Sponza
  - *** use "lazy" probe maps. ***
  - use texture LODs properly
  - use skybox in L2
  - investigate things looking "too dim"
  - investigate "smooth backface" artifacts
  - investigate "self-lighting"
  - investigate "zero-grid-coord-repeat"
  - investigate "edge-sponza-wraparound-maybe"
  - think more carefully about equations, energy conservation etc. :GI_Equations

  - use visibility testing
    - read in get_indirect_luminance, guessing a weight.
    - update map in probe_maps_update
    - introduce probe_visibility_map
    - settle on a format and size of visibility map.
    - read about Chebyshev chest etc.
    - think of a good test for light bleeding.
