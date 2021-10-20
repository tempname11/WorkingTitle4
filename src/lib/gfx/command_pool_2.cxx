#include <cassert>
#include "command_pool_2.hxx"

CommandPool1 *command_pool_2_borrow(CommandPool2 *pool2) {
  std::scoped_lock lock(pool2->mutex);
  assert(pool2->available.size() > 0);
  auto pool = pool2->available.back();
  pool2->available.pop_back();
  return pool;
}

void command_pool_2_return(CommandPool2 *pool2, CommandPool1 *pool1) {
  std::scoped_lock lock(pool2->mutex);
  pool2->available.push_back(pool1);
}
