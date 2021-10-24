# Performance Tools TODO

- Measure performance
  - Storage for frame stats (rolling N frames)
  - Record GPU frame time
  - Record CPU frame time
  - Display frame time charts

- Organize GPU memory allocation
  - heap trackers at session level.
  - "Arena"
    - with multi-alloc-like capability
    - mutexes where really needed
    - tracked at top level
  - migrate allocator
  - migrate multi_alloc
  - fix ImGui view
  - add ImGui graphs!
