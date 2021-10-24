#pragma once
#include <vector>
#include <src/lib/task.hxx>

namespace lib {

std::pair<lib::Task *, lib::Task *> defer(lib::Task *task);
std::pair<lib::Task *, nullptr_t> defer_many(std::vector<lib::Task *> *tasks);

} // namespace