#pragma once

namespace lib::io {

bool is_eof(FILE *file);

namespace magic {
  void write(FILE *file, char const *str);
  bool check(FILE *file, char const *str);
}

namespace string {
  void write(FILE *file, std::string *str);
  void read(FILE *file, std::string *str);
}

} // namespace
