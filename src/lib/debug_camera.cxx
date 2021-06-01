#include <glm/glm.hpp>
/*
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
*/
#include "debug_camera.hxx"

namespace lib::debug_camera {

State init() {
  return {
    .position = glm::vec3(0.0f, 0.0f, 0.0f),
    .lon_lat = glm::vec2(0.0f),
		.rotation_q = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
  };
}

static const auto camera_speed = 8.0f;
static const auto mouse_sensitivity = 2.0f;

/*
void update(State *it, application::Processed *processed) {
  it->lon_lat += processed->cursor_delta * mouse_sensitivity;
	it->lon_lat.y = fmaxf(-1.0f, fminf(1.0f, it->lon_lat.y));
	it->lon_lat.x = fmodf(it->lon_lat.x, 4.0f);

	it->rotation_q = (
		glm::angleAxis(it->lon_lat.y * glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::angleAxis(it->lon_lat.x * glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f))
	);

	auto movement = glm::vec3(0.0);
	if (
		processed->pressed_keys.contains(GLFW_KEY_W) ||
		processed->pressed_keys.contains(GLFW_KEY_UP) 
	) {
		movement += glm::vec3(0.0f, +1.0f, 0.0f);
	}

	if (
		processed->pressed_keys.contains(GLFW_KEY_S) ||
		processed->pressed_keys.contains(GLFW_KEY_DOWN) 
	) {
		movement += glm::vec3(0.0f, -1.0f, 0.0f);
	}

	if (
		processed->pressed_keys.contains(GLFW_KEY_D) ||
		processed->pressed_keys.contains(GLFW_KEY_RIGHT) 
	) {
		movement += glm::vec3(+1.0f, 0.0f, 0.0f);
	}

	if (
		processed->pressed_keys.contains(GLFW_KEY_A) ||
		processed->pressed_keys.contains(GLFW_KEY_LEFT) 
	) {
		movement += glm::vec3(-1.0f, 0.0f, 0.0f);
	}

	if (
		processed->pressed_keys.contains(GLFW_KEY_E) ||
		processed->pressed_keys.contains(GLFW_KEY_PAGE_UP) 
	) {
		movement += glm::vec3(0.0f, 0.0f, +1.0f);
	}

	if (
		processed->pressed_keys.contains(GLFW_KEY_Q) ||
		processed->pressed_keys.contains(GLFW_KEY_PAGE_DOWN) 
	) {
		movement += glm::vec3(0.0f, 0.0f, -1.0f);
	}

  it->position +=
		(glm::inverse(it->rotation_q) * movement) *
		(processed->elapsed.count() * camera_speed);
}
*/

glm::mat4 to_view_matrix(State *it) {
  auto translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f) - it->position);
	auto rotation = glm::mat4_cast(it->rotation_q);
  return rotation * translation;
}

} // namespace