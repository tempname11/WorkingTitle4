#pragma once

namespace lib::usage {

namespace internal {
  template<typename T>
  struct Base {
    T *ptr;
    T &operator *() { return *ptr; }
    T *operator ->() { return ptr; }
  };
}

template<typename T> struct Full : internal::Base<T> {
  Full(T *_ptr) { this->ptr = _ptr; }
};
template<typename T> struct Some : internal::Base<T> {
  Some(T *_ptr) { this->ptr = _ptr; }
  Some(Full<T> other) { this->ptr = other.ptr; }
};
template<typename T> struct None : internal::Base<T> {
  None(T *_ptr) { this->ptr = _ptr; }
  None(Full<T> other) { this->ptr = other.ptr; }
  None(Some<T> other) { this->ptr = other.ptr; }
};

} // namespace