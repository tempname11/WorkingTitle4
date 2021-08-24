#pragma once
#include <glm/glm.hpp>
#include <src/lib/gfx/multi_alloc.hxx>

namespace engine::common::texture {
  template<typename T>
  struct Data {
    T *data;
    int width;
    int height;
    int channels;
    uint32_t computed_mip_levels;
  };
}