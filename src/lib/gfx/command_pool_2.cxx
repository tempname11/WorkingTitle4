#include <cassert>
#include "command_pool_2.hxx"

VkCommandPool command_pool_2_borrow(CommandPool2 *pool2) {
  std::scoped_lock lock(pool2->mutex);
  assert(pool2->pools.size() > 0);
  auto pool = pool2->pools.back();
  pool2->pools.pop_back();
  return pool;
}

void command_pool_2_return(CommandPool2 *pool2, VkCommandPool pool) {
  std::scoped_lock lock(pool2->mutex);
  pool2->pools.push_back(pool);
}
