#include <sol/sol.hpp>
#include <src/global.hxx>

namespace tools {

void console(char const *command) {
  sol::state lua;
  lua.set_function("print", [](std::string str) {
    _log(str);
  });
  lua.script(command);
}

} // namespace