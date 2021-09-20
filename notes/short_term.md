# milestone: pretty picture
  - ddgi
    - hysteresis: previous probe maps -> new probe maps
    - indirect light pass: GB2 -> LB2

    --- should work

    - probe_light_map
      - read
	  - write
      - figure out if rgba16 shader format works at all

    - pmu...
    - l2...

    - g2...
      - write g0, g1, g2, z
      - figure out the coord to write to
      - dispatch correct amount of workgroups
