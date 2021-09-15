#version 460
layout(binding = 0, rgba16) uniform image2D probe_light_map; // format??

void main() {
  /*
  vec3 light = texture(lbuffer_image, (vec2(gl_WorkGroupID.xy) + 0.5) / gl_NumWorkGroups.xy).rgb;
  imageStore(final_image, ivec2(gl_WorkGroupID.xy), vec4(2.0 * light, 1.0));
  */
  imageStore(probe_light_map, ivec2(gl_WorkGroupID.xy), vec4(1.0));
}