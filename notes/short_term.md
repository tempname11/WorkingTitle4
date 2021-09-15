# milestone: pretty picture
  - ddgi later
    - hysteresis: previous probe maps -> new probe maps
    - indirect light pass: GB2 -> LB2

  - ddgi next
    - probe_light_map
      - read
	  - write
      - figure out if rgba16 shader format works at all

    - raytracing pass, writing probes into GB2
    - "secondary GBuffer" (GB2)
    - direct light passes: GB2 -> LB2

  - ddgi current
    - "secondary LBuffer" (LB2)
      - fake transition from light pass (terminology?)
      - declare in probe_maps_update shader
      - bind to probe_maps_update
      - images
      - views
