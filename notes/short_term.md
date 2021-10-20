# Problem

Memory usage seems to increase over time. Yet Tracy profiling shows everything is great.
So there might be "external" allocations we don't track. In particular, the driver might
allocate Vulkan memory (HOST_VISIBLE + HOST_COHERENT) this way, which would mean we leak it.

Also, over time, performance drops, but swapchain recreation (via window resize) helps.
I suspect that this is related.

For both problems, I don't see a direct approach, need to clean up some old stuff and get
better tools first.

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
  - heap trackers at session level.
