#pragma once
#include <cstdlib>
#include <src/lib/base.hxx>
#include <src/lib/single_allocator.hxx>
#include "public.hxx"
#include "impl.hxx"

namespace engine::system::entities {

void init(Impl *out) {
  *out = {
    .types = lib::array::create<by_type_t>(lib::allocator::crt, 0),
  };
}

void deinit(Impl *it) {
  lib::array::destroy(it->types);
}

int compare_by_numeric_type(const void *l0, const void *r0) {
  auto l = (added_component_t *) l0;
  auto r = (added_component_t *) r0;

  if (l->type < r->type) {
    return -1;
  } else if (l->type > r->type) {
    return 1;
  }

  return 0;
}

void add(Impl *it, added_component_t *components, uint8_t count) {
  uint64_t mask;
  qsort(components, count, sizeof(*components), compare_by_numeric_type);

  for (size_t i = 0; i < count; i++) {
    mask |= (1 << uint64_t(components[i].type));
  }

  size_t type_index = size_t(-1);
  for (size_t i = 0; i < it->types->count; i++) {
    auto ty = &it->types->data[i];
    if (ty->component_bit_mask == mask) {
      type_index = i;
      break;
    }
  }

  if (type_index == size_t(-1)) {
    lib::array::ensure_space(&it->types, 1);
    type_index = it->types->count++;
    it->types->data[type_index] = by_type_t {
      .component_bit_mask = mask,
      .component_count = count,
      .indices = lib::array::create<component::index_t>(
        lib::single_allocator::create(
          lib::allocator::GB * sizeof(component::index_t)
        ),
        0
      ),
    };
    for (size_t i = 0; i < count; i++) {
      it->types->data[type_index].component_index_offsets[
        uint8_t(components[i].type)
      ] = i; // we have sorted them.
    }
  }


  auto ty = &it->types->data[type_index];

  lib::array::ensure_space(&ty->indices, count);
  for (size_t i = 0; i < count; i++) {
    ty->indices->data[ty->indices->count + i] = components[i].ix;
  }
  ty->indices->count += count;
}

} // namespace
