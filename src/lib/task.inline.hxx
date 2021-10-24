#pragma once

namespace lib::task {

namespace _ {
  typedef std::vector<ResourceAccessDescription> RADs;

  template <typename T>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, T*);

  template <typename T, typename... FnArgs>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<Ref<T>, FnArgs...>*);

  template <typename T, typename... FnArgs>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<Use<T>, FnArgs...>*);

  template <typename T, typename... FnArgs>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<Own<T>, FnArgs...>*);

  template<>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<>*) { }

  template <typename T, typename... FnArgs>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<Ref<T>, FnArgs...>*) {
    std::tuple<FnArgs...> *p = nullptr;
    v.erase(v.begin() + i);
    set_exclusive_flags_based_on_type(v, i, p);
  }

  template <typename T, typename... FnArgs>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<Use<T>, FnArgs...>*) {
    std::tuple<FnArgs...> *p = nullptr;
    set_exclusive_flags_based_on_type(v, i + 1, p);
  }

  template <typename T, typename... FnArgs>
  inline void set_exclusive_flags_based_on_type(RADs &v, int i, std::tuple<Own<T>, FnArgs...>*) {
    v[i].exclusive = true;
    std::tuple<FnArgs...> *p = nullptr;
    set_exclusive_flags_based_on_type(v, i + 1, p);
  }
}

template<QueueIndex ix, typename... FnArgs, typename... PassedArgs>
inline Task * create(void (*fn)(Context<ix> *, FnArgs...), PassedArgs... args) {
  auto cast_fn = (void (*)(ContextBase *, FnArgs...))(fn);
  std::function<TaskSig> bound_fn = std::bind(cast_fn, std::placeholders::_1, args...);
  std::vector<ResourceAccessDescription> v = { ResourceAccessDescription(args)... };
  _::set_exclusive_flags_based_on_type(v, 0, (std::tuple<FnArgs...> *)nullptr);
  return new Task {
    .fn = bound_fn,
    .queue_index = ix,
    .resources = v,
  };
}

} // namespace