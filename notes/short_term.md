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
  - try 8 instead of 6 octosize?

  - use visibility testing
    - ? more complex bleeding tests ?
    - read in get_indirect_luminance, guessing a weight.
    - update map in probe_maps_update
    - introduce probe_visibility_map
