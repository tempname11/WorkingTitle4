- `assert( m_head != m_tail );` in TracyVulkan.hpp
  (currently seems to follow a warning for a cmd freed before fully used)
  (in case it happens, `ZoneValue`-s with cmd handles are placed)
  [LAST: 21-10-21]
