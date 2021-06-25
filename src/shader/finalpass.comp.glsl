#version 460
layout(binding = 0, rgba8) uniform image2D final_image;
layout(binding = 1) uniform sampler2D lbuffer_image;

void main() {
  vec4 tmp = texture(lbuffer_image, vec2(gl_WorkGroupID.xy) / gl_NumWorkGroups.xy);
  imageStore(final_image, ivec2(gl_WorkGroupID.xy), tmp);
}