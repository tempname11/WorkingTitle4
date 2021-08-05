#include <cassert>
#include <cstdio>
#include <cstdint>
#include <string>

namespace lib::io {

bool is_eof(FILE *file) {
  auto pos0 = ftell(file);
  fseek(file, 0, SEEK_END);
  auto pos1 = ftell(file);
  fseek(file, pos0, SEEK_SET);
  return pos0 == pos1;
}

namespace magic {
  void write(FILE *file, char const *str) {
    fwrite(str, 1, strlen(str), file);
  }

  bool check(FILE *file, char const *str) {
    auto len = strlen(str);
    char buf[16];
    assert(len < 16);
    buf[len] = 0;
    fread(buf, 1, len, file);
    return 0 == strcmp(str, buf);
  }
}

namespace string {
  void write(FILE *file, std::string *str) {
    uint32_t size = str->size();
    fwrite(&size, 1, sizeof(size), file);
    fwrite(str->c_str(), 1, size, file); 
  }

  void read(FILE *file, std::string *str) {
    uint32_t size = 0;
    fread(&size, 1, sizeof(size), file);
    str->resize(size);
    fread(str->data(), 1, size, file);
  }
}

} // namespace
