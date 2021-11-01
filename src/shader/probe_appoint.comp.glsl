#version 460
#extension GL_GOOGLE_include_directive : enable
#include "common/frame.glsl"
#include "common/probes.glsl"

layout(
  local_size_x = 8, // :ProbeAppointLocalSize
  local_size_y = 8,
  local_size_z = 1
) in;

layout(binding = 0) uniform Frame { FrameData data; } frame;
layout(binding = 1, r32ui) uniform readonly uimage2D probe_attention_prev; // :ProbeAttentionFormat
layout(binding = 2, rgba16ui) uniform uimage2D probe_confidence; // :ProbeConfidenceFormat
layout(set = 1, binding = 0) writeonly buffer ProbeWorkset {
  uvec4 data[];
} probe_workset;
layout(set = 1, binding = 1) buffer ProbeCounter { uint next_id; } probe_counter;

layout(push_constant) uniform Cascade {
  uint level;
} cascade;

void main() {
  uvec3 probe_coord = gl_GlobalInvocationID;

  bool out_of_bounds = any(greaterThanEqual(
    probe_coord,
    frame.data.probe.grid_size
  )); // needed because maybe `grid_size % local_group_size != 0`

  if (out_of_bounds) { 
    return;
  }

  uvec2 z_subcoord = uvec2(
    probe_coord.z % frame.data.probe.grid_size_z_factors.x,
    probe_coord.z / frame.data.probe.grid_size_z_factors.x
  );

  uvec2 cascade_subcoord = uvec2(
    cascade.level % PROBE_CASCADE_COUNT_FACTORS.x,
    cascade.level / PROBE_CASCADE_COUNT_FACTORS.x
  );

  uvec2 combined_coord = (
    probe_coord.xy +
    frame.data.probe.grid_size.xy * (
      z_subcoord +
      frame.data.probe.grid_size_z_factors * (
        cascade_subcoord
      )
    )
  );

  uvec4 confidence_packed = imageLoad(
    probe_confidence,
    ivec2(combined_coord)
  ); // :ProbeConfidenceFormat
  float volatility = confidence_packed.b / 65535.0;
  float accumulator = confidence_packed.a / 65535.0 * PROBE_ACCUMULATOR_MAX;
  uint last_update_frame_number = confidence_packed.r * 65536 + confidence_packed.g;
  uint frames_passed = frame.data.number - last_update_frame_number;
  float beta = max(0.0, accumulator - min(frames_passed, PROBE_ACCUMULATOR_MAX));

  bool should_appoint = false;
  { // invalidation
    ivec3 infinite_grid_min = frame.data.probe.cascades[cascade.level].infinite_grid_min;
    ivec3 infinite_grid_min_prev = frame.data.probe.cascades[cascade.level].infinite_grid_min_prev;

    ivec3 infinite_grid_coord = (0
      + (ivec3(probe_coord) - infinite_grid_min) % ivec3(frame.data.probe.grid_size)
      + infinite_grid_min
    );
    bool out_of_bounds = (false
      || any(lessThan(
        infinite_grid_coord,
        infinite_grid_min_prev
      ))
      || any(greaterThan(
        infinite_grid_coord,
        infinite_grid_min_prev + ivec3(frame.data.probe.grid_size) - 1
      ))
    );
    if (out_of_bounds) {
      imageStore(
        probe_confidence,
        ivec2(combined_coord),
        uvec4(0)
      );
      should_appoint = true;
    }
  }

  uint attention = imageLoad(
    probe_attention_prev,
    ivec2(combined_coord)
  ).r;
  
  if (beta < PROBE_ACCUMULATOR_MAX - PROBE_MAX_SKIPS) {
    should_appoint = true;
  }

  if (frame.data.flags.disable_indirect_skips) {
    should_appoint = true;
  }

  if (attention == 0 || !should_appoint) {
    return;
  }

  uint id = atomicAdd(probe_counter.next_id, 1);
  probe_workset.data[id] = uvec4(probe_coord, attention);
}