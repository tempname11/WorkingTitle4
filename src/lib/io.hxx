#pragma once

namespace lib::io {

bool is_eof(FILE *file);

namespace magic {
  void write(FILE *file, char const *str);
  bool check(FILE *file, char const *str);
}

namespace string {
  void write_c(FILE *file, char const *c_str, uint32_t size = 0);
  void write(FILE *file, std::string *str);
  void read(FILE *file, std::string *str);
}

} // namespace
