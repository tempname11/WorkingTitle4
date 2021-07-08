#pragma once
#define FMT_USE_NONTYPE_TEMPLATE_PARAMETERS 0
#include <fmt/format.h>
#include <Tracy.hpp>

enum QueueIndex {
  QUEUE_INDEX_MAIN_THREAD_ONLY,
  QUEUE_INDEX_HIGH_PRIORITY,
  QUEUE_INDEX_NORMAL_PRIORITY,
  QUEUE_INDEX_LOW_PRIORITY,
  QUEUE_COUNT
};

void _log(std::string &str);

#define LOG(...) {\
  auto str = fmt::format(__VA_ARGS__);\
  _log(str);\
}

#define DBG LOG

template<typename R, typename T> R checked_integer_cast(T value) {
  R result = (R) value;
  assert((T) result == value);
  assert(
    (result >= 0 && value >= 0) ||
    (result < 0 && value < 0)
  );
  return result;
}
