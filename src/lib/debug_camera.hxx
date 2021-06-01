#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
/*
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "application.hxx"
*/

namespace lib::debug_camera {

struct State {
  glm::vec3 position;
  glm::vec2 lon_lat;
  glm::quat rotation_q;
};

State init();
//void update(State *, application::Processed *);
glm::mat4 to_view_matrix(State *it);

} // namespace