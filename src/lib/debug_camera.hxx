#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace lib::debug_camera {

struct State {
  glm::vec3 position;
  glm::vec2 lon_lat;
  glm::quat rotation_q;
};

struct Input {
  bool x_pos;
  bool x_neg;
  bool y_pos;
  bool y_neg;
  bool z_pos;
  bool z_neg;
  glm::vec2 cursor_position_delta;
};

State init();
void update(State *it, Input *input, double elapsed_sec);
glm::mat4 to_view_matrix(State *it);

} // namespace