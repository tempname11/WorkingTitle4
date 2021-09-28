#version 460
#extension GL_EXT_ray_query : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "common/light_model.glsl"
#include "common/probes.glsl"
#include "common/frame.glsl"

const uint ray_count = 64; // :DDGI_N_Rays 64

layout(location = 0) in vec2 position;
layout(location = 0) out vec3 result; 

layout(input_attachment_index = 0, binding = 0) uniform subpassInput gchannel0;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput gchannel1;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput gchannel2;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput zchannel;
layout(binding = 4) uniform sampler2D probe_light_map_previous;
layout(binding = 5) uniform Frame { FrameData data; } frame;

void main() {
  // @Note check frame=0
  result = vec3(0.0, 0.0, 0.0); // @Incomplete
}
