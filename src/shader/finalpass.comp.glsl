#version 460
layout(binding = 0, rgba16) uniform image2D final_image;
layout(binding = 1) uniform sampler2D lbuffer_image;

void main() {
  vec3 light = texture(lbuffer_image, (vec2(gl_WorkGroupID.xy) + 0.5) / gl_NumWorkGroups.xy).rgb;
  imageStore(final_image, ivec2(gl_WorkGroupID.xy), vec4(2.0 * light, 1.0));
}