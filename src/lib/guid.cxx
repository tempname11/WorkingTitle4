#include "guid.hxx"

namespace lib::guid {

GUID next(Counter *it) {
  return it->next.fetch_add(1) + 1;
}

} // namespace