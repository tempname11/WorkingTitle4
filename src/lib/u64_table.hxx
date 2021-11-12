#pragma once
#include "_base/array.hxx"

// Potential improvements:
// * Make impl not templated (use `sizeof(T)` and `memcpy`), move into ".cxx".
// * VS debugger visualization / NatVis.
// * `_fill` could move all the data from lower tier bucket it accesses.
// * Storing bucket entry counts would probably make inserts faster.
//   (for < 32 buckets store them the header)
//   (for >= 32 buckets store them in the lower bits of the first key)

namespace lib {
  namespace u64_table {
    struct hash_t {
      uint64_t as_number;

      bool operator==(hash_t h) const { return this->as_number == h.as_number; }
    };
  }

  template<typename T>
  struct u64_table_t {
    lib::allocator_t *acr;
    u64_table_t *prev_table;
    size_t prev_table_refs;
    size_t num_buckets; // power-of-2
    u64_table::hash_t *keys;
    T *values;
  };
}

namespace lib::u64_table {

const size_t BUCKET_NUM_ENTRIES = 32;

inline hash_t from_guid(lib::GUID guid) {
  return hash_t(guid);
}

template<typename T>
u64_table_t<T> *create(
  lib::allocator_t *acr,
  size_t min_initial_capacity
) {
  size_t num_buckets = 1;
  while (num_buckets * BUCKET_NUM_ENTRIES < min_initial_capacity) {
    num_buckets *= 2;
  }
  auto alignment = lib::max(alignof(u64_table_t<T>), alignof(T));
  auto size = sizeof(u64_table_t<T>);

  size = (
    (size + alignof(hash_t) - 1) / alignof(hash_t)
  ) * alignof(hash_t);
  auto keys_offset = size;
  size += sizeof(hash_t) * num_buckets * BUCKET_NUM_ENTRIES;
  auto memset_zero_size = size;

  size = (
    (size + alignof(T) - 1) / alignof(T)
  ) * alignof(T);
  auto values_offset = size;
  size += sizeof(T) * num_buckets * BUCKET_NUM_ENTRIES;


  auto it = (u64_table_t<T> *) acr->alloc_fn(acr, size, alignment);
  memset(it, 0, memset_zero_size);
  it->acr = acr;
  it->num_buckets = num_buckets;
  it->keys = (hash_t *) ((uint8_t *) it + keys_offset);
  it->values = (T *) ((uint8_t *) it + values_offset);
  return it;
}

template<typename T>
void destroy(u64_table_t<T> *it) {
  if (it->prev_table != nullptr) {
    destroy(it->prev_table);
  }
  it->acr->dealloc_fn(it->acr, it);
}

template<typename T>
void _fill(
  size_t out_max_count,
  hash_t *out_keys,
  T *out_values,
  uint64_t test_partial_key,
  uint64_t test_divisor,
  u64_table_t<T> *it
) {
  auto bucket = test_partial_key % it->num_buckets;
  auto first_entry_index = bucket * BUCKET_NUM_ENTRIES;
  bool did_fill = false;

  for (size_t i = 0; i < BUCKET_NUM_ENTRIES; i++) {
    auto p_key = &it->keys[first_entry_index + i];

    if (p_key->as_number == 0) {
      // @CopyPaste :LazyBucket {
      if (true
        && i == 0
        && it->prev_table != nullptr
        && it->keys[first_entry_index + 1].as_number == 0
      ) {
        _fill(
          BUCKET_NUM_ENTRIES,
          p_key,
          &it->values[first_entry_index + 0],
          bucket,
          it->num_buckets,
          it->prev_table
        );

        assert(it->prev_table_refs > 0);
        it->prev_table_refs--;
        if (it->prev_table_refs == 0) {
          destroy(it->prev_table);
          it->prev_table = nullptr;
        }

        i = size_t(-1); // restart loop!
        continue;
      }
      // @CopyPaste :LazyBucket }

      break;
    }

    if ((p_key->as_number % test_divisor) == test_partial_key) {
      *out_keys = *p_key;
      *out_values = it->values[first_entry_index + i];

      assert(out_max_count > 0);
      out_max_count--;
      out_keys++;
      out_values++;

      did_fill = true;
    }
  }

  if (!did_fill) {
    out_keys[1] = hash_t(~uint64_t(0));
  }
}

template<typename T>
T *lookup(u64_table_t<T> *it, hash_t key) {
  assert(key.as_number != 0);
  auto bucket = key.as_number % it->num_buckets;
  auto first_entry_index = bucket * BUCKET_NUM_ENTRIES;

  for (size_t i = 0; i < BUCKET_NUM_ENTRIES; i++) {
    auto p_key = &it->keys[first_entry_index + i];

    if (p_key->as_number == 0) {
      // @CopyPaste :LazyBucket {
      if (true
        && i == 0
        && it->prev_table != nullptr
        && it->keys[first_entry_index + 1].as_number == 0
      ) {
        _fill(
          BUCKET_NUM_ENTRIES,
          p_key,
          &it->values[first_entry_index + 0],
          bucket,
          it->num_buckets,
          it->prev_table
        );

        assert(it->prev_table_refs > 0);
        it->prev_table_refs--;
        if (it->prev_table_refs == 0) {
          destroy(it->prev_table);
          it->prev_table = nullptr;
        }

        i = size_t(-1); // restart loop!
        continue;
      }
      // @CopyPaste :LazyBucket }

      break;
    }

    if (*p_key == key) {
      return &it->values[first_entry_index + i];
    }
  }

  return nullptr;
}

template<typename T>
void insert(u64_table_t<T> **p_it, hash_t key, T value) {
  auto it = *p_it;
  assert(key.as_number != 0);
  auto bucket = key.as_number % it->num_buckets;
  auto first_entry_index = bucket * BUCKET_NUM_ENTRIES;

  for (size_t i = 0; i < BUCKET_NUM_ENTRIES; i++) {
    auto p_key = &it->keys[first_entry_index + i];

    if (p_key->as_number == 0) {
      // @CopyPaste :LazyBucket {
      if (true
        && i == 0
        && it->prev_table != nullptr
        && it->keys[first_entry_index + 1].as_number == 0
      ) {
        _fill(
          BUCKET_NUM_ENTRIES,
          p_key,
          &it->values[first_entry_index + 0],
          bucket,
          it->num_buckets,
          it->prev_table
        );

        assert(it->prev_table_refs > 0);
        it->prev_table_refs--;
        if (it->prev_table_refs == 0) {
          destroy(it->prev_table);
          it->prev_table = nullptr;
        }

        i = size_t(-1); // restart loop!
        continue;
      }
      // @CopyPaste :LazyBucket }

      *p_key = key;
      it->values[first_entry_index + i] = value;
      return;
    }

    if (*p_key == key) {
      assert("Duplicate key" && false);
      // Actually, there might be duplicates beyond the first zero-keyed index.
      // But we don't check for them...
    }
  }

  // no space!
  auto new_table = create<T>(it->acr, 2 * it->num_buckets * BUCKET_NUM_ENTRIES);
  new_table->prev_table = it;
  new_table->prev_table_refs = 2 * it->num_buckets;
  insert<T>(&new_table, key, value);
  *p_it = new_table;
}

template<typename T>
void remove(u64_table_t<T> *it, hash_t key) {
  assert(key.as_number != 0);
  auto bucket = key.as_number % it->num_buckets;
  auto first_entry_index = bucket * BUCKET_NUM_ENTRIES;
  size_t slot_index = size_t(-1);

  for (size_t i = 0; i < BUCKET_NUM_ENTRIES; i++) {
    auto p_key = &it->keys[first_entry_index + i];

    if (p_key->as_number == 0) {
      // @CopyPaste :LazyBucket {
      if (true
        && i == 0
        && it->prev_table != nullptr
        && it->keys[first_entry_index + 1].as_number == 0
      ) {
        _fill(
          BUCKET_NUM_ENTRIES,
          p_key,
          &it->values[first_entry_index + 0],
          bucket,
          it->num_buckets,
          it->prev_table
        );

        assert(it->prev_table_refs > 0);
        it->prev_table_refs--;
        if (it->prev_table_refs == 0) {
          destroy(it->prev_table);
          it->prev_table = nullptr;
        }

        i = (size_t) -1; // restart loop!
        continue;
      }
      // @CopyPaste :LazyBucket }

      break;
    }

    if (*p_key == key) {
      slot_index = first_entry_index + i;
      break;
    }
  }

  if (slot_index == size_t(-1)) {
    assert("Key not found for removing" && false);
  }

  size_t last_occupied_index = BUCKET_NUM_ENTRIES - 1;
  for (size_t i = slot_index - first_entry_index; i < BUCKET_NUM_ENTRIES; i++) {
    auto p_key = &it->keys[first_entry_index + i];
    if (p_key->as_number == 0) {
      last_occupied_index = first_entry_index + i - 1;
    }
  }

  it->keys[slot_index] = it->keys[last_occupied_index];
  it->values[slot_index] = it->values[last_occupied_index];
  it->keys[last_occupied_index] = hash_t(0);
}

} // namespace