#include <thread>
#include <cassert>

namespace lib::main_thread {

struct Token {};
static Token the_token;

Token *get_token(std::thread::id main_thread_id) {
  if (main_thread_id == std::this_thread::get_id()) {
    return &the_token;
  } else {
    return nullptr;
  }
}

} // namespace