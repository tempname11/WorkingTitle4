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

static const auto CAMERA_SPEED_PER_SEC = 8.0f;
static const auto MOUSE_SENSITIVITY = 1.0f;

void update(State *it, Input *input, double elapsed_sec) {
  it->lon_lat += input->cursor_position_delta * MOUSE_SENSITIVITY;
	it->lon_lat.y = fmaxf(-1.0f, fminf(1.0f, it->lon_lat.y));
	it->lon_lat.x = fmodf(it->lon_lat.x, 4.0f);

	it->rotation_q = (
		glm::angleAxis(it->lon_lat.y * glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)) *
		glm::angleAxis(it->lon_lat.x * glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f))
	);

	auto movement = glm::vec3(0.0);
	if (input->y_pos) {
		movement += glm::vec3(0.0f, +1.0f, 0.0f);
	}
	if (input->y_neg) {
		movement += glm::vec3(0.0f, -1.0f, 0.0f);
	}
	if (input->x_pos) {
		movement += glm::vec3(+1.0f, 0.0f, 0.0f);
	}
	if (input->x_neg) {
		movement += glm::vec3(-1.0f, 0.0f, 0.0f);
	}
	if (input->z_pos) {
		movement += glm::vec3(0.0f, 0.0f, +1.0f);
	}
	if (input->z_neg) {
		movement += glm::vec3(0.0f, 0.0f, -1.0f);
	}
	if (glm::length(movement) > 0.0f) {
		movement = glm::normalize(movement);
	}

  it->position +=
		(glm::inverse(it->rotation_q) * movement) *
		float(elapsed_sec * CAMERA_SPEED_PER_SEC);
}

glm::mat4 to_view_matrix(State *it) {
  auto translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f) - it->position);
	auto rotation = glm::mat4_cast(it->rotation_q);
  return rotation * translation;
}

} // namespace