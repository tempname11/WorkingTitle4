#include <cassert>
#include <mutex>
#include <vector>
#include <src/global.hxx>
#include "gpu_signal.hxx"

namespace lib::gpu_signal {

struct Entry {
  VkSemaphore semaphore;
  uint64_t value;
  lib::task::Task *signal;
};

struct Storage {
  std::mutex mutex;
  std::vector<Entry> entries;
  uint64_t new_entries_value;
};

lib::task::Task *create(
  Support *it,
  VkDevice device,
  VkSemaphore semaphore,
  uint64_t value
) {
  ZoneScoped;
  auto signal = lib::task::create_external_signal();
  auto entry = Entry {
    .semaphore = semaphore,
    .value = value,
    .signal = signal,
  };

  // sanity check
  uint64_t current;
  vkGetSemaphoreCounterValue(device, semaphore, &current);
  assert(current < value);

  {
    std::scoped_lock(it->storage->mutex);
    it->storage->entries.push_back(entry);
    it->storage->new_entries_value++;
    VkSemaphoreSignalInfo signal_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
      .semaphore = it->new_entries_semaphore,
      .value = it->storage->new_entries_value,
    };
    {
      auto result = vkSignalSemaphore(device, &signal_info);
      assert(result == VK_SUCCESS);
    }
  }

  return signal;
}

void waiting_thread(
  Storage *storage,
  VkDevice device,
  VkSemaphore new_entries_semaphore,
  lib::task::Runner *tr
) {
  ZoneScoped;
  tracy::SetThreadName("lib::gpu_signal::waiting_thread");
  bool quit = false;
  std::vector<VkSemaphore> semaphores;
  std::vector<uint64_t> values;
  while (!quit) {
    uint64_t new_entries_wait_value;
    {
      std::scoped_lock lock(storage->mutex);
      for (auto &entry : storage->entries) {
        semaphores.push_back(entry.semaphore);
        values.push_back(entry.value);
      }
      new_entries_wait_value = storage->new_entries_value + 1;
    }
    semaphores.push_back(new_entries_semaphore);
    values.push_back(new_entries_wait_value);
    VkSemaphoreWaitInfo waitInfo = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
      .flags = VK_SEMAPHORE_WAIT_ANY_BIT,
      .semaphoreCount = (uint32_t) semaphores.size(),
      .pSemaphores = semaphores.data(),
      .pValues = values.data(),
    };
    {
      ZoneScopedN("vkWaitSemaphores");
      auto result = vkWaitSemaphores(device, &waitInfo, UINT64_MAX);
      assert(result == VK_SUCCESS);
    }

    std::scoped_lock lock(storage->mutex);
    if (storage->entries.size() == 0) {
      // this can only happen when no normal semaphores are present,
      // and deinit_support is called.
      quit = true;
    }
    for (size_t i = 0; i < semaphores.size(); i++) {
      uint64_t value;
      {
        auto result = vkGetSemaphoreCounterValue(device, semaphores[i], &value);
        assert(result == VK_SUCCESS);
      }
      if (i == semaphores.size() - 1) {
        // it's the new_entries semaphore
        // just ignore it and loop
      } else {
        // it's one of the normal semaphores
        if (value >= values[i]) {
          {
            // workaround a validation layer bug...?
            VkSemaphoreWaitInfo waitInfo = {
              .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
              .flags = VK_SEMAPHORE_WAIT_ANY_BIT,
              .semaphoreCount = 1,
              .pSemaphores = &semaphores[i],
              .pValues = &values[i],
            };
            auto result = vkWaitSemaphores(device, &waitInfo, 0);
            assert(result == VK_SUCCESS);
          }
          // signal and delete the corresponding task
          for (size_t j = 0; j < storage->entries.size(); j++) {
            auto entry = &storage->entries[j];
            if (entry->semaphore == semaphores[i] && entry->value == values[i]) {
              lib::task::signal(tr, entry->signal);
              storage->entries.erase(storage->entries.begin() + j);
              break;
            }
          }
        }
      }
    }
    semaphores.clear();
    values.clear();
  }
}

Support init_support(
  lib::task::Runner *tr,
  VkDevice device,
  const VkAllocationCallbacks *allocator
) {
  ZoneScoped;
  VkSemaphore new_entries_semaphore;
  VkSemaphoreTypeCreateInfo timeline_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
    .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
  };
  VkSemaphoreCreateInfo create_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .pNext = &timeline_info,
  };
  {
    auto result = vkCreateSemaphore(device, &create_info, allocator, &new_entries_semaphore);
    assert(result == VK_SUCCESS);
  }
  auto storage = new Storage {};
  auto thread = std::thread(waiting_thread, storage, device, new_entries_semaphore, tr);
  return {
    .storage = storage,
    .new_entries_semaphore = new_entries_semaphore,
    .thread = std::move(thread),
  };
}

void deinit_support(
  Support *it,
  VkDevice device,
  const VkAllocationCallbacks *allocator
) {
  ZoneScoped;
  {
    std::scoped_lock(it->storage->mutex);
    assert(it->storage->entries.size() == 0);
    it->storage->new_entries_value++;
    VkSemaphoreSignalInfo signal_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
      .semaphore = it->new_entries_semaphore,
      .value = it->storage->new_entries_value,
    };
    auto result = vkSignalSemaphore(device, &signal_info);
    assert(result == VK_SUCCESS);
  }
  it->thread.join();
  vkDestroySemaphore(device, it->new_entries_semaphore, allocator);
}

} // namespace