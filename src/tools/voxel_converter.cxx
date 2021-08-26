#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/color_space.hpp>
#include <src/engine/common/mesh.hxx>

namespace tools {

#define FLAG_RANDOMIZE

#define X 255
uint8_t mc_table[256][16] = {
  {X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 8, 3, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 1, 9, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {1, 8, 3, 9, 8, 1, X, X, X, X, X, X, X, X, X, X},
  {1, 2, 10, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 8, 3, 1, 2, 10, X, X, X, X, X, X, X, X, X, X},
  {9, 2, 10, 0, 2, 9, X, X, X, X, X, X, X, X, X, X},
  {2, 8, 3, 2, 10, 8, 10, 9, 8, X, X, X, X, X, X, X},
  {3, 11, 2, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 11, 2, 8, 11, 0, X, X, X, X, X, X, X, X, X, X},
  {1, 9, 0, 2, 3, 11, X, X, X, X, X, X, X, X, X, X},
  {1, 11, 2, 1, 9, 11, 9, 8, 11, X, X, X, X, X, X, X},
  {3, 10, 1, 11, 10, 3, X, X, X, X, X, X, X, X, X, X},
  {0, 10, 1, 0, 8, 10, 8, 11, 10, X, X, X, X, X, X, X},
  {3, 9, 0, 3, 11, 9, 11, 10, 9, X, X, X, X, X, X, X},
  {9, 8, 10, 10, 8, 11, X, X, X, X, X, X, X, X, X, X},
  {4, 7, 8, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {4, 3, 0, 7, 3, 4, X, X, X, X, X, X, X, X, X, X},
  {0, 1, 9, 8, 4, 7, X, X, X, X, X, X, X, X, X, X},
  {4, 1, 9, 4, 7, 1, 7, 3, 1, X, X, X, X, X, X, X},
  {1, 2, 10, 8, 4, 7, X, X, X, X, X, X, X, X, X, X},
  {3, 4, 7, 3, 0, 4, 1, 2, 10, X, X, X, X, X, X, X},
  {9, 2, 10, 9, 0, 2, 8, 4, 7, X, X, X, X, X, X, X},
  {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, X, X, X, X},
  {8, 4, 7, 3, 11, 2, X, X, X, X, X, X, X, X, X, X},
  {11, 4, 7, 11, 2, 4, 2, 0, 4, X, X, X, X, X, X, X},
  {9, 0, 1, 8, 4, 7, 2, 3, 11, X, X, X, X, X, X, X},
  {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, X, X, X, X},
  {3, 10, 1, 3, 11, 10, 7, 8, 4, X, X, X, X, X, X, X},
  {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, X, X, X, X},
  {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, X, X, X, X},
  {4, 7, 11, 4, 11, 9, 9, 11, 10, X, X, X, X, X, X, X},
  {9, 5, 4, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {9, 5, 4, 0, 8, 3, X, X, X, X, X, X, X, X, X, X},
  {0, 5, 4, 1, 5, 0, X, X, X, X, X, X, X, X, X, X},
  {8, 5, 4, 8, 3, 5, 3, 1, 5, X, X, X, X, X, X, X},
  {1, 2, 10, 9, 5, 4, X, X, X, X, X, X, X, X, X, X},
  {3, 0, 8, 1, 2, 10, 4, 9, 5, X, X, X, X, X, X, X},
  {5, 2, 10, 5, 4, 2, 4, 0, 2, X, X, X, X, X, X, X},
  {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, X, X, X, X},
  {9, 5, 4, 2, 3, 11, X, X, X, X, X, X, X, X, X, X},
  {0, 11, 2, 0, 8, 11, 4, 9, 5, X, X, X, X, X, X, X},
  {0, 5, 4, 0, 1, 5, 2, 3, 11, X, X, X, X, X, X, X},
  {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, X, X, X, X},
  {10, 3, 11, 10, 1, 3, 9, 5, 4, X, X, X, X, X, X, X},
  {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, X, X, X, X},
  {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, X, X, X, X},
  {5, 4, 8, 5, 8, 10, 10, 8, 11, X, X, X, X, X, X, X},
  {9, 7, 8, 5, 7, 9, X, X, X, X, X, X, X, X, X, X},
  {9, 3, 0, 9, 5, 3, 5, 7, 3, X, X, X, X, X, X, X},
  {0, 7, 8, 0, 1, 7, 1, 5, 7, X, X, X, X, X, X, X},
  {1, 5, 3, 3, 5, 7, X, X, X, X, X, X, X, X, X, X},
  {9, 7, 8, 9, 5, 7, 10, 1, 2, X, X, X, X, X, X, X},
  {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, X, X, X, X},
  {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, X, X, X, X},
  {2, 10, 5, 2, 5, 3, 3, 5, 7, X, X, X, X, X, X, X},
  {7, 9, 5, 7, 8, 9, 3, 11, 2, X, X, X, X, X, X, X},
  {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, X, X, X, X},
  {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, X, X, X, X},
  {11, 2, 1, 11, 1, 7, 7, 1, 5, X, X, X, X, X, X, X},
  {9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, X, X, X, X},
  {5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, X},
  {11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, X},
  {11, 10, 5, 7, 11, 5, X, X, X, X, X, X, X, X, X, X},
  {10, 6, 5, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 8, 3, 5, 10, 6, X, X, X, X, X, X, X, X, X, X},
  {9, 0, 1, 5, 10, 6, X, X, X, X, X, X, X, X, X, X},
  {1, 8, 3, 1, 9, 8, 5, 10, 6, X, X, X, X, X, X, X},
  {1, 6, 5, 2, 6, 1, X, X, X, X, X, X, X, X, X, X},
  {1, 6, 5, 1, 2, 6, 3, 0, 8, X, X, X, X, X, X, X},
  {9, 6, 5, 9, 0, 6, 0, 2, 6, X, X, X, X, X, X, X},
  {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, X, X, X, X},
  {2, 3, 11, 10, 6, 5, X, X, X, X, X, X, X, X, X, X},
  {11, 0, 8, 11, 2, 0, 10, 6, 5, X, X, X, X, X, X, X},
  {0, 1, 9, 2, 3, 11, 5, 10, 6, X, X, X, X, X, X, X},
  {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, X, X, X, X},
  {6, 3, 11, 6, 5, 3, 5, 1, 3, X, X, X, X, X, X, X},
  {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, X, X, X, X},
  {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, X, X, X, X},
  {6, 5, 9, 6, 9, 11, 11, 9, 8, X, X, X, X, X, X, X},
  {5, 10, 6, 4, 7, 8, X, X, X, X, X, X, X, X, X, X},
  {4, 3, 0, 4, 7, 3, 6, 5, 10, X, X, X, X, X, X, X},
  {1, 9, 0, 5, 10, 6, 8, 4, 7, X, X, X, X, X, X, X},
  {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, X, X, X, X},
  {6, 1, 2, 6, 5, 1, 4, 7, 8, X, X, X, X, X, X, X},
  {1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, X, X, X, X},
  {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, X, X, X, X},
  {7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, X},
  {3, 11, 2, 7, 8, 4, 10, 6, 5, X, X, X, X, X, X, X},
  {5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, X, X, X, X},
  {0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, X, X, X, X},
  {9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, X},
  {8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, X, X, X, X},
  {5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, X},
  {0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, X},
  {6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, X, X, X, X},
  {10, 4, 9, 6, 4, 10, X, X, X, X, X, X, X, X, X, X},
  {4, 10, 6, 4, 9, 10, 0, 8, 3, X, X, X, X, X, X, X},
  {10, 0, 1, 10, 6, 0, 6, 4, 0, X, X, X, X, X, X, X},
  {8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, X, X, X, X},
  {1, 4, 9, 1, 2, 4, 2, 6, 4, X, X, X, X, X, X, X},
  {3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, X, X, X, X},
  {0, 2, 4, 4, 2, 6, X, X, X, X, X, X, X, X, X, X},
  {8, 3, 2, 8, 2, 4, 4, 2, 6, X, X, X, X, X, X, X},
  {10, 4, 9, 10, 6, 4, 11, 2, 3, X, X, X, X, X, X, X},
  {0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, X, X, X, X},
  {3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, X, X, X, X},
  {6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, X},
  {9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, X, X, X, X},
  {8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, X},
  {3, 11, 6, 3, 6, 0, 0, 6, 4, X, X, X, X, X, X, X},
  {6, 4, 8, 11, 6, 8, X, X, X, X, X, X, X, X, X, X},
  {7, 10, 6, 7, 8, 10, 8, 9, 10, X, X, X, X, X, X, X},
  {0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, X, X, X, X},
  {10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, X, X, X, X},
  {10, 6, 7, 10, 7, 1, 1, 7, 3, X, X, X, X, X, X, X},
  {1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, X, X, X, X},
  {2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, X},
  {7, 8, 0, 7, 0, 6, 6, 0, 2, X, X, X, X, X, X, X},
  {7, 3, 2, 6, 7, 2, X, X, X, X, X, X, X, X, X, X},
  {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, X, X, X, X},
  {2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, X},
  {1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, X},
  {11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, X, X, X, X},
  {8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, X},
  {0, 9, 1, 11, 6, 7, X, X, X, X, X, X, X, X, X, X},
  {7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, X, X, X, X},
  {7, 11, 6, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {7, 6, 11, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {3, 0, 8, 11, 7, 6, X, X, X, X, X, X, X, X, X, X},
  {0, 1, 9, 11, 7, 6, X, X, X, X, X, X, X, X, X, X},
  {8, 1, 9, 8, 3, 1, 11, 7, 6, X, X, X, X, X, X, X},
  {10, 1, 2, 6, 11, 7, X, X, X, X, X, X, X, X, X, X},
  {1, 2, 10, 3, 0, 8, 6, 11, 7, X, X, X, X, X, X, X},
  {2, 9, 0, 2, 10, 9, 6, 11, 7, X, X, X, X, X, X, X},
  {6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, X, X, X, X},
  {7, 2, 3, 6, 2, 7, X, X, X, X, X, X, X, X, X, X},
  {7, 0, 8, 7, 6, 0, 6, 2, 0, X, X, X, X, X, X, X},
  {2, 7, 6, 2, 3, 7, 0, 1, 9, X, X, X, X, X, X, X},
  {1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, X, X, X, X},
  {10, 7, 6, 10, 1, 7, 1, 3, 7, X, X, X, X, X, X, X},
  {10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, X, X, X, X},
  {0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, X, X, X, X},
  {7, 6, 10, 7, 10, 8, 8, 10, 9, X, X, X, X, X, X, X},
  {6, 8, 4, 11, 8, 6, X, X, X, X, X, X, X, X, X, X},
  {3, 6, 11, 3, 0, 6, 0, 4, 6, X, X, X, X, X, X, X},
  {8, 6, 11, 8, 4, 6, 9, 0, 1, X, X, X, X, X, X, X},
  {9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, X, X, X, X},
  {6, 8, 4, 6, 11, 8, 2, 10, 1, X, X, X, X, X, X, X},
  {1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, X, X, X, X},
  {4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, X, X, X, X},
  {10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, X},
  {8, 2, 3, 8, 4, 2, 4, 6, 2, X, X, X, X, X, X, X},
  {0, 4, 2, 4, 6, 2, X, X, X, X, X, X, X, X, X, X},
  {1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, X, X, X, X},
  {1, 9, 4, 1, 4, 2, 2, 4, 6, X, X, X, X, X, X, X},
  {8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, X, X, X, X},
  {10, 1, 0, 10, 0, 6, 6, 0, 4, X, X, X, X, X, X, X},
  {4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, X},
  {10, 9, 4, 6, 10, 4, X, X, X, X, X, X, X, X, X, X},
  {4, 9, 5, 7, 6, 11, X, X, X, X, X, X, X, X, X, X},
  {0, 8, 3, 4, 9, 5, 11, 7, 6, X, X, X, X, X, X, X},
  {5, 0, 1, 5, 4, 0, 7, 6, 11, X, X, X, X, X, X, X},
  {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, X, X, X, X},
  {9, 5, 4, 10, 1, 2, 7, 6, 11, X, X, X, X, X, X, X},
  {6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, X, X, X, X},
  {7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, X, X, X, X},
  {3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, X},
  {7, 2, 3, 7, 6, 2, 5, 4, 9, X, X, X, X, X, X, X},
  {9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, X, X, X, X},
  {3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, X, X, X, X},
  {6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, X},
  {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, X, X, X, X},
  {1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, X},
  {4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, X},
  {7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, X, X, X, X},
  {6, 9, 5, 6, 11, 9, 11, 8, 9, X, X, X, X, X, X, X},
  {3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, X, X, X, X},
  {0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, X, X, X, X},
  {6, 11, 3, 6, 3, 5, 5, 3, 1, X, X, X, X, X, X, X},
  {1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, X, X, X, X},
  {0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, X},
  {11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, X},
  {6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, X, X, X, X},
  {5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, X, X, X, X},
  {9, 5, 6, 9, 6, 0, 0, 6, 2, X, X, X, X, X, X, X},
  {1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, X},
  {1, 5, 6, 2, 1, 6, X, X, X, X, X, X, X, X, X, X},
  {1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, X},
  {10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, X, X, X, X},
  {0, 3, 8, 5, 6, 10, X, X, X, X, X, X, X, X, X, X},
  {10, 5, 6, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {11, 5, 10, 7, 5, 11, X, X, X, X, X, X, X, X, X, X},
  {11, 5, 10, 11, 7, 5, 8, 3, 0, X, X, X, X, X, X, X},
  {5, 11, 7, 5, 10, 11, 1, 9, 0, X, X, X, X, X, X, X},
  {10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, X, X, X, X},
  {11, 1, 2, 11, 7, 1, 7, 5, 1, X, X, X, X, X, X, X},
  {0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, X, X, X, X},
  {9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, X, X, X, X},
  {7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, X},
  {2, 5, 10, 2, 3, 5, 3, 7, 5, X, X, X, X, X, X, X},
  {8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, X, X, X, X},
  {9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, X, X, X, X},
  {9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, X},
  {1, 3, 5, 3, 7, 5, X, X, X, X, X, X, X, X, X, X},
  {0, 8, 7, 0, 7, 1, 1, 7, 5, X, X, X, X, X, X, X},
  {9, 0, 3, 9, 3, 5, 5, 3, 7, X, X, X, X, X, X, X},
  {9, 8, 7, 5, 9, 7, X, X, X, X, X, X, X, X, X, X},
  {5, 8, 4, 5, 10, 8, 10, 11, 8, X, X, X, X, X, X, X},
  {5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, X, X, X, X},
  {0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, X, X, X, X},
  {10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, X},
  {2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, X, X, X, X},
  {0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, X},
  {0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, X},
  {9, 4, 5, 2, 11, 3, X, X, X, X, X, X, X, X, X, X},
  {2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, X, X, X, X},
  {5, 10, 2, 5, 2, 4, 4, 2, 0, X, X, X, X, X, X, X},
  {3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, X},
  {5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, X, X, X, X},
  {8, 4, 5, 8, 5, 3, 3, 5, 1, X, X, X, X, X, X, X},
  {0, 4, 5, 1, 0, 5, X, X, X, X, X, X, X, X, X, X},
  {8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, X, X, X, X},
  {9, 4, 5, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {4, 11, 7, 4, 9, 11, 9, 10, 11, X, X, X, X, X, X, X},
  {0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, X, X, X, X},
  {1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, X, X, X, X},
  {3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, X},
  {4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, X, X, X, X},
  {9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, X},
  {11, 7, 4, 11, 4, 2, 2, 4, 0, X, X, X, X, X, X, X},
  {11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, X, X, X, X},
  {2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, X, X, X, X},
  {9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, X},
  {3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, X},
  {1, 10, 2, 8, 7, 4, X, X, X, X, X, X, X, X, X, X},
  {4, 9, 1, 4, 1, 7, 7, 1, 3, X, X, X, X, X, X, X},
  {4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, X, X, X, X},
  {4, 0, 3, 7, 4, 3, X, X, X, X, X, X, X, X, X, X},
  {4, 8, 7, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {9, 10, 8, 10, 11, 8, X, X, X, X, X, X, X, X, X, X},
  {3, 0, 9, 3, 9, 11, 11, 9, 10, X, X, X, X, X, X, X},
  {0, 1, 10, 0, 10, 8, 8, 10, 11, X, X, X, X, X, X, X},
  {3, 1, 10, 11, 3, 10, X, X, X, X, X, X, X, X, X, X},
  {1, 2, 11, 1, 11, 9, 9, 11, 8, X, X, X, X, X, X, X},
  {3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, X, X, X, X},
  {0, 2, 11, 8, 0, 11, X, X, X, X, X, X, X, X, X, X},
  {3, 2, 11, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {2, 3, 8, 2, 8, 10, 10, 8, 9, X, X, X, X, X, X, X},
  {9, 10, 2, 0, 9, 2, X, X, X, X, X, X, X, X, X, X},
  {2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, X, X, X, X},
  {1, 10, 2, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {1, 3, 8, 9, 1, 8, X, X, X, X, X, X, X, X, X, X},
  {0, 9, 1, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {0, 3, 8, X, X, X, X, X, X, X, X, X, X, X, X, X},
  {X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X}
};
#undef X

const glm::vec3 mc_edge_vertices[] = {
  // bottom
  {0.5f, 0.0f, 0.0f},
  {1.0f, 0.5f, 0.0f},
  {0.5f, 1.0f, 0.0f},
  {0.0f, 0.5f, 0.0f},
  // top
  {0.5f, 0.0f, 1.0f},
  {1.0f, 0.5f, 1.0f},
  {0.5f, 1.0f, 1.0f},
  {0.0f, 0.5f, 1.0f},
  // middle
  {0.0f, 0.0f, 0.5f},
  {1.0f, 0.0f, 0.5f},
  {1.0f, 1.0f, 0.5f},
  {0.0f, 1.0f, 0.5f},
};

const glm::vec3 cube_vertices[] = {
  // -XY
  {-1.0f, -1.0f, -1.0f},
  {+1.0f, -1.0f, -1.0f},
  {+1.0f, +1.0f, -1.0f},
  {+1.0f, +1.0f, -1.0f},
  {-1.0f, +1.0f, -1.0f},
  {-1.0f, -1.0f, -1.0f},
  // +XY
  {-1.0f, -1.0f, +1.0f},
  {-1.0f, +1.0f, +1.0f},
  {+1.0f, +1.0f, +1.0f},
  {+1.0f, +1.0f, +1.0f},
  {+1.0f, -1.0f, +1.0f},
  {-1.0f, -1.0f, +1.0f},
  // -YZ
  {-1.0f, -1.0f, -1.0f},
  {-1.0f, +1.0f, -1.0f},
  {-1.0f, +1.0f, +1.0f},
  {-1.0f, +1.0f, +1.0f},
  {-1.0f, -1.0f, +1.0f},
  {-1.0f, -1.0f, -1.0f},
  // +YZ
  {+1.0f, -1.0f, -1.0f},
  {+1.0f, -1.0f, +1.0f},
  {+1.0f, +1.0f, +1.0f},
  {+1.0f, +1.0f, +1.0f},
  {+1.0f, +1.0f, -1.0f},
  {+1.0f, -1.0f, -1.0f},
  // -ZX
  {-1.0f, -1.0f, -1.0f},
  {-1.0f, -1.0f, +1.0f},
  {+1.0f, -1.0f, +1.0f},
  {+1.0f, -1.0f, +1.0f},
  {+1.0f, -1.0f, -1.0f},
  {-1.0f, -1.0f, -1.0f},
  // +ZX
  {-1.0f, +1.0f, -1.0f},
  {+1.0f, +1.0f, -1.0f},
  {+1.0f, +1.0f, +1.0f},
  {+1.0f, +1.0f, +1.0f},
  {-1.0f, +1.0f, +1.0f},
  {-1.0f, +1.0f, -1.0f},
};

const glm::vec3 cube_normals[] = {
  // -XY
  {0.0f, 0.0f, -1.0f},
  // +XY
  {0.0f, 0.0f, +1.0f},
  // -YZ
  {-1.0f, 0.0f, 0.0f},
  // +YZ
  {+1.0f, 0.0f, 0.0f},
  // -ZX
  {0.0f, -1.0f, 0.0f},
  // +ZX
  {0.0f, +1.0f, 0.0f},
};

namespace mesh {
  struct T06_Builder {
    std::vector<engine::common::mesh::IndexT06> indices;
    std::vector<engine::common::mesh::VertexT06> vertices;
  };

  void write(const char *out_filename, T06_Builder *mesh) {
    FILE *out = fopen(out_filename, "wb");
    assert(out != nullptr);
    uint32_t index_count = mesh->indices.size();
    uint32_t vertex_count = mesh->vertices.size();
    fwrite(&index_count, 1, sizeof(index_count), out);
    fwrite(&vertex_count, 1, sizeof(vertex_count), out);
    fwrite(mesh->indices.data(), 1, mesh->indices.size() * sizeof(engine::common::mesh::IndexT06), out);
    fwrite(mesh->vertices.data(), 1, mesh->vertices.size() * sizeof(engine::common::mesh::VertexT06), out);
    assert(ferror(out) == 0);
    fclose(out);
  }
}

union XYZ {
  uint64_t value;
  struct {
    int16_t x;
    int16_t y;
    int16_t z;
  } xyz;
};

struct IntermediateData {
  bool read_xyzi;
  std::unordered_map<uint64_t, uint8_t> voxels;
  glm::u8vec4 palette[256];
};

const float TEXTURE_MAPPING_SCALE = 1.0f / 8.0f;

void build_cubes(IntermediateData *data, mesh::T06_Builder *mesh) {
  for (auto elem: data->voxels) {
    XYZ voxel = { .value = elem.first };
    for (size_t i = 0; i < 6; i++) { // sides
      XYZ adjacent = { .xyz = {
        .x = int16_t(voxel.xyz.x + cube_normals[i].x),
        .y = int16_t(voxel.xyz.y + cube_normals[i].y),
        .z = int16_t(voxel.xyz.z + cube_normals[i].z),
      } };
      if (data->voxels.contains(adjacent.value)) {
        continue;
      }
      glm::vec3 v[6];
      for (size_t j = 0; j < 6; j++) { // triangle vertices
        v[j] = cube_vertices[6 * i + j] * 0.5f + 0.5f + glm::vec3(
          voxel.xyz.x, voxel.xyz.y, voxel.xyz.z
        );
      }
      auto normal = cube_normals[i];
      auto unit_x = glm::vec3(1.0f, 0.0f, 0.0f);
      auto unit_y = glm::vec3(0.0f, 1.0f, 0.0f);
      auto dot_x = std::abs(glm::dot(normal, unit_x));
      auto dot_y = std::abs(glm::dot(normal, unit_y));
      auto tangent = glm::normalize(
        dot_x < dot_y
          ? glm::cross(normal, unit_x)
          : glm::cross(normal, unit_y)
      );
      auto bitangent = glm::cross(normal, tangent);
      auto scale = TEXTURE_MAPPING_SCALE;
      for (size_t j = 0; j < 6; j++) { // triangle vertices
        // @Performance: if this is ever used seriously (which is unlikely),
        // we could some of the vertices. For now, don't bother.
        mesh->indices.push_back(mesh->indices.size());
        mesh->vertices.push_back(engine::common::mesh::VertexT06 {
          .position = v[j],
          .tangent = tangent,
          .bitangent = bitangent,
          .normal = normal,
          .uv = scale * glm::vec2(glm::dot(v[j], tangent), glm::dot(v[j], bitangent)),
        });
      }
    }
  }
}

void build_mc(IntermediateData *data, mesh::T06_Builder *mesh) {
  std::unordered_set<uint64_t> candidates;
  for (auto &elem: data->voxels) {
    XYZ voxel = { .value = elem.first };
    for (int16_t dx = -1; dx <= 1; dx++) {
      for (int16_t dy = -1; dy <= 1; dy++) {
        for (int16_t dz = -1; dz <= 1; dz++) {
          candidates.insert(XYZ { .xyz {
            .x = int16_t(voxel.xyz.x + dx),
            .y = int16_t(voxel.xyz.y + dy),
            .z = int16_t(voxel.xyz.z + dz),
          } }.value);
        }
      }
    }
  }

  for (auto elem: candidates) {
    XYZ voxel = { .value = elem };
    auto offset = glm::vec3(
      voxel.xyz.x, voxel.xyz.y, voxel.xyz.z
    );

    uint64_t values[] = {
      XYZ { .xyz {
        .x = int16_t(voxel.xyz.x + 0),
        .y = int16_t(voxel.xyz.y + 0),
        .z = int16_t(voxel.xyz.z + 0),
      } }.value,
      XYZ { .xyz {
        .x = int16_t(voxel.xyz.x + 1),
        .y = int16_t(voxel.xyz.y + 0),
        .z = int16_t(voxel.xyz.z + 0),
      } }.value,
      XYZ { .xyz {
        .x = int16_t(voxel.xyz.x + 1),
        .y = int16_t(voxel.xyz.y + 1),
        .z = int16_t(voxel.xyz.z + 0),
      } }.value,
      XYZ { .xyz {
        .x = int16_t(voxel.xyz.x + 0),
        .y = int16_t(voxel.xyz.y + 1),
        .z = int16_t(voxel.xyz.z + 0),
      } }.value,
      XYZ { .xyz {
        .x = int16_t(voxel.xyz.x + 0),
        .y = int16_t(voxel.xyz.y + 0),
        .z = int16_t(voxel.xyz.z + 1),
      } }.value,
      XYZ { .xyz {
        .x = int16_t(voxel.xyz.x + 1),
        .y = int16_t(voxel.xyz.y + 0),
        .z = int16_t(voxel.xyz.z + 1),
      } }.value,
      XYZ { .xyz {
        .x = int16_t(voxel.xyz.x + 1),
        .y = int16_t(voxel.xyz.y + 1),
        .z = int16_t(voxel.xyz.z + 1),
      } }.value,
      XYZ { .xyz {
        .x = int16_t(voxel.xyz.x + 0),
        .y = int16_t(voxel.xyz.y + 1),
        .z = int16_t(voxel.xyz.z + 1),
      } }.value,
    };

    bool bits[] = {
      data->voxels.contains(values[0]),
      data->voxels.contains(values[1]),
      data->voxels.contains(values[2]),
      data->voxels.contains(values[3]),
      data->voxels.contains(values[4]),
      data->voxels.contains(values[5]),
      data->voxels.contains(values[6]),
      data->voxels.contains(values[7]),
    };

    uint8_t variant = 0;
    for (size_t i = 0; i < 8; i++) {
      variant += bits[i] << i;
    }
    
    auto edge_vertex_indices = mc_table[variant];
    for (size_t i = 0; i < 5; i++) {
      auto index0 = edge_vertex_indices[3 * i];
      auto index1 = edge_vertex_indices[3 * i + 1];
      auto index2 = edge_vertex_indices[3 * i + 2];
      if (index0 == 255) {
        break;
      }

      glm::vec3 v[3];
      v[0] = mc_edge_vertices[index0] + offset;
      v[1] = mc_edge_vertices[index1] + offset;
      v[2] = mc_edge_vertices[index2] + offset;
      auto diff1 = mc_edge_vertices[index1] - mc_edge_vertices[index0];
      auto diff2 = mc_edge_vertices[index2] - mc_edge_vertices[index0];
      auto normal = -glm::normalize(glm::cross(
        diff1,
        diff2
      ));
      auto unit_x = glm::vec3(1.0f, 0.0f, 0.0f);
      auto unit_y = glm::vec3(0.0f, 1.0f, 0.0f);
      auto dot_x = std::abs(glm::dot(normal, unit_x));
      auto dot_y = std::abs(glm::dot(normal, unit_y));
      auto tangent = glm::normalize(
        dot_x < dot_y
          ? glm::cross(normal, unit_x)
          : glm::cross(normal, unit_y)
      );
      auto bitangent = glm::cross(normal, tangent);
      auto scale = TEXTURE_MAPPING_SCALE;

      for (size_t j = 0; j < 3; j++) {
        // @Performance: if this is ever used seriously (which is unlikely),
        // we could some of the vertices. For now, don't bother.
        mesh->indices.push_back(mesh->indices.size());
        mesh->vertices.push_back(engine::common::mesh::VertexT06 {
          .position = v[j],
          .tangent = tangent,
          .bitangent = bitangent,
          .normal = normal,
          .uv = scale * glm::vec2(glm::dot(v[j], tangent), glm::dot(v[j], bitangent)),
        });
      }
    }
  }
}

void read_chunk(uint8_t **pCursor, size_t *pBytesLeft, IntermediateData *data) {
  auto bytes_left = *pBytesLeft;
  auto cursor = *pCursor;
  if (bytes_left == 0) {
    return;
  }
  assert(bytes_left >= 12);
  if (0 == memcmp(cursor, "MAIN", 4)) {
    // DBG("MAIN");
    cursor += 12;
    bytes_left -= 12;
    while(bytes_left > 0) {
      read_chunk(&cursor, &bytes_left, data);
    }
  } else if (0 == memcmp(cursor, "RGBA", 4)) {
    // DBG("RGBA");
    cursor += 12;
    bytes_left -= 12;
    assert(bytes_left >= 4 * 256);
    data->palette[0] = glm::u8vec4(0.0);
    for (size_t i = 0; i < 256 - 1; i++) {
      data->palette[i + 1] = *((glm::u8vec4 *)(cursor + 4 * i));
    }
    cursor += 4 * 256;
    bytes_left -= 4 * 256;
  } else if (0 == memcmp(cursor, "XYZI", 4)) {
    // DBG("XYZI");
    cursor += 12;
    bytes_left -= 12;
    assert(bytes_left >= 4);
    uint32_t num_voxels = *((uint32_t *)cursor);
    cursor += 4;
    bytes_left -= 4;
    assert(bytes_left >= 4 * num_voxels);
    if (!data->read_xyzi) {
      data->read_xyzi = true;
      for (uint32_t i = 0; i < num_voxels; i++) {
        XYZ voxel = {
          .xyz = {
            .x = (int16_t) *(cursor + 4 * i + 0),
            .y = (int16_t) *(cursor + 4 * i + 1),
            .z = (int16_t) *(cursor + 4 * i + 2),
          }
        };
        uint8_t paletteIndex = *(cursor + 4 * i + 3);
        data->voxels.insert(std::pair(voxel.value, paletteIndex));
      }
    }
    cursor += 4 * num_voxels;
    bytes_left -= 4 * num_voxels;
  } else {
    // unknown chunk, skip it
    uint32_t n = *((uint32_t *)(cursor + 4));
    uint32_t m = *((uint32_t *)(cursor + 8));
    cursor += 12;
    bytes_left -= 12;
    if (bytes_left >= n + m) {
      cursor += n + m;
      bytes_left -= n + m;
    }
  }

  *pBytesLeft = bytes_left;
  *pCursor = cursor;
}

void voxel_converter(
  char const* path_vox,
  char const* path_t06,
  bool enable_marching_cubes
) {
  FILE *in = fopen(path_vox, "rb");
  assert(in != nullptr);
  fseek(in, 0, SEEK_END);
  auto vox_buffer_size = ftell(in);
  fseek(in, 0, SEEK_SET);
  auto vox_buffer = (uint8_t *) malloc(vox_buffer_size);
  fread((void *) vox_buffer, 1, vox_buffer_size, in);
  assert(ferror(in) == 0);
  fclose(in);

  uint8_t *cursor = vox_buffer;
  size_t bytes_left = vox_buffer_size;
  assert(bytes_left >= 8);
  assert(0 == memcmp(cursor, "VOX ", 4));
  const uint32_t version = 150;
  assert(0 == memcmp(cursor + 4, &version, 4));
  cursor += 8;
  bytes_left -= 8;
  IntermediateData data = {};
  read_chunk(&cursor, &bytes_left, &data);
  free(vox_buffer);
  // DBG("VOXEL COUNT: {}", data.voxels.size());

  #ifdef FLAG_RANDOMIZE
    srand(
      std::chrono::high_resolution_clock::now().time_since_epoch()
        / std::chrono::nanoseconds(1)
    );
    int16_t x = 0;
    int16_t y = 0;
    int16_t z = 0;
    for (size_t i = 0; i < 64; i++) {
      auto c = rand() % 6;
      if (c < 2) {
        x += (c % 2) * 2 - 1;
      } else if (c < 4) {
        y += (c % 2) * 2 - 1;
      } else if (c < 6) {
        z += (c % 2) * 2 - 1;
      }
      XYZ coord = { .xyz = {
        .x = x,
        .y = y,
        .z = z,
      } };
      data.voxels.insert({ coord.value, 0 });
    }
  #endif
  
  mesh::T06_Builder mesh = {};
  if (enable_marching_cubes) {
    build_mc(&data, &mesh);
  } else {
    build_cubes(&data, &mesh);
  }

  mesh::write(path_t06, &mesh);
}

} // namespace