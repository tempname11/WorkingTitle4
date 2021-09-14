# milestone: pretty picture
  - ddgi
    - raytracing pass, writing probes into GB2
    - [inflight_count] "secondary GBuffer" (GB2)
    - direct light passes: GB2 -> LB2
    - indirect light pass: GB2 -> LB2
    - [inflight_count] "secondary LBuffer" (LB2)
    - compute probe pass: LB2, previous probe maps -> new probe maps
       - fix probe_light_map barrier (see @Incomplete in record_transition.cxx)

    - [inflight_count] probe light map
      - use in shader
      - data layout
      - vulkan layout change
