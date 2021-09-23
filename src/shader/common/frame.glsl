#ifndef _COMMON_FRAME_GLSL_
#define _COMMON_FRAME_GLSL_

struct FrameFlags {
  bool show_normals;
  bool show_sky;
  bool disable_direct_lighting;
  bool disable_indirect_lighting;
};

#endif // _COMMON_FRAME_GLSL_