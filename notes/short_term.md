# Tasks

- Measure performance
  - Display frame time charts
  - Record GPU frame time
  - Record CPU frame time
  - Storage for frame stats (?)

- Organize GPU memory allocation
  - fix ImGui view
  - migrate multi_alloc
  - migrate allocator
  - "Arena"
    - with multi-alloc-like capability
    - mutexes where really needed
    - tracked at top level
  - heap trackers at session level.
