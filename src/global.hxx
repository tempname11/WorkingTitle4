#pragma once
#include <cassert>
#include <fmt/format.h>
#include <Tracy.hpp>
#include <src/lib/task.hxx>

enum QueueIndex {
  QUEUE_INDEX_MAIN_THREAD_ONLY,
  QUEUE_INDEX_CONTROL_THREAD_ONLY,
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

template<typename T>
using Ref = lib::task::Ref<T>;

template<typename T>
using Use = lib::task::Use<T>;

template<typename T>
using Own = lib::task::Own<T>;
