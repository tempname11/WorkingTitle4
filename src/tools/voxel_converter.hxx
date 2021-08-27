#pragma once

namespace tools {

void voxel_converter(
  char const* path_vox,
  char const* path_t06,
  bool enable_marching_cubes,
  bool enable_random_voxels
);

} // namespace
