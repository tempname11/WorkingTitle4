# milestone: pretty picture
  - ddgi
    - hysteresis: previous probe maps -> new probe maps
    - indirect light pass: GB2 -> LB2

    --- should work

    - probe_light_map
      - read
	  - write
      - figure out if rgba16 shader format works at all

    --- all boilerplate done

    - raytracing pass (compute or rt pipeline?), writing probes into GB2
