#pragma once

namespace lib::task {

namespace _internal {
  typedef std::vector<ResourceAccessDescription> RADs;

  template <typename T>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, T*);

  template <typename T, typename... FnArgs>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<Shared<T>, FnArgs...>*);

  template <typename T, typename... FnArgs>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<Exclusive<T>, FnArgs...>*);

  template<>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<>*) { }

  template <typename T, typename... FnArgs>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<Shared<T>, FnArgs...>*) {
    std::tuple<FnArgs...> *p = nullptr;
    set_exclusive_flags_based_on_type(v, i + 1, p);
  }

  template <typename T, typename... FnArgs>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<Exclusive<T>, FnArgs...>*) {
    v[i].exclusive = true;
    std::tuple<FnArgs...> *p = nullptr;
    set_exclusive_flags_based_on_type(v, i + 1, p);
  }
}

template<QueueIndex ix, typename... FnArgs, typename... PassedArgs>
inline TaskDescription describe(void (*fn)(Context *, QueueMarker<ix>, FnArgs...), PassedArgs... args) {
  std::vector<void *> tt = { args... };
  std::vector<ResourceAccessDescription> v = { ResourceAccessDescription(args)... };
  _internal::set_exclusive_flags_based_on_type(v, 0, (std::tuple<FnArgs...> *)nullptr);
  std::function<TaskSig> bound_fn = std::bind(fn, std::placeholders::_1, QueueMarker<ix>(), args...);
  return TaskDescription {
    .queue_index = ix,
    .fn = bound_fn,
    .resources = v,
  };
}

} // namespace