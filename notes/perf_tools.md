# Performance Tools TODO

- Measure performance
  - Storage for frame stats (rolling N frames)
  - Record GPU frame time
  - Record CPU frame time
  - Display frame time charts

- Organize GPU memory allocation
  - fix ImGui view
  - migrate multi_alloc
  - migrate allocator
  - "Arena"
    - with multi-alloc-like capability
    - mutexes where really needed
    - tracked at top level
  - heap trackers at session level.
