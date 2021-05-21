#pragma once
#include <thread>

namespace lib::main_thread {

struct Token;

Token *get_token(std::thread::id main_thread_id);

} // namespace