#version 460
layout(binding = 0, rgba16_snorm) uniform image2D gchannel0;
/*
layout(binding = 1, rgba8) uniform image2D gchannel1;
layout(binding = 2, rgba8) uniform image2D gchannel2;
*/

void main() {
  // @Incomplete
  imageStore(gchannel0, ivec2(gl_WorkGroupID.xy), vec4(0.0, mod(gl_WorkGroupID.y, 3), 1.0, 1.0));
}