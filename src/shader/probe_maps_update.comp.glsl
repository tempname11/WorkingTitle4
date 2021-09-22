#version 460
layout(binding = 0, r11f_g11f_b10f) uniform image2D probe_light_map;
layout(binding = 1) uniform sampler2D lbuffer2_image;

void main() {
  // @Temporary
  vec4 value = texture(
    lbuffer2_image,
    (vec2(gl_WorkGroupID.xy) + 0.5) / /*gl_NumWorkGroups.xy*/ 2048
  );
  imageStore(
    probe_light_map,
    ivec2(gl_WorkGroupID.xy),
    value
  );
}