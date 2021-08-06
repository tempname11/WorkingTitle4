### milestone: dynamic loading
- mesh load batching, with:
  - chunked allocation of GPU mem
  - at most N meshes loaded at once
  - reload support
  - unload support
  - ref_many API for mesh?
  - ...

- mesh code review
  - should be split into files
  - lib::lifetime instead of will_have_ ?

- textures: get them up to speed with meshes

- overall loading performance review