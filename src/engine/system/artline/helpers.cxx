#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <src/lib/base.hxx>
#include "helpers.hxx"

namespace engine::system::artline {

float cube(glm::vec3 position, glm::vec3 center, float halfwidth) {
  return (
    lib::min(
      lib::min(
        halfwidth - abs(position.x - center.x),
        halfwidth - abs(position.y - center.y)
      ),
      halfwidth - abs(position.z - center.z)
    )
  );
}
float sphere(glm::vec3 position, glm::vec3 center, float radius) {
  return radius - glm::length(position - center);
}

glm::vec2 triplanar_texture_uv(glm::vec3 position, glm::vec3 N) {
  auto s = position;
  return (
    N.x * glm::vec2(s.y, s.z) +
    N.y * glm::vec2(s.x, s.z) +
    N.z * glm::vec2(s.x, s.y)
  );
}

glm::vec2 ad_hoc_sphere_texture_uv(glm::vec3 position, glm::vec3 N) {
  auto s = position;
  auto r = glm::length(glm::vec2(s.x, s.y));
  auto dir = glm::normalize(glm::vec2(s.x, s.y));
  auto t = glm::atan(r, abs(s.z)) / (0.5f * glm::pi<float>());

  return glm::mix(
    0.5f + 0.5f * (dir * t),
    0.5f + 0.5f * (dir * r),
    abs(N.z)
  );
}

glm::mat4 rotation(float turns, glm::vec3 axis) {
  return glm::rotate(id, glm::pi<float>() * 2.0f * turns, axis);
}

glm::mat4 translation(glm::vec3 t) {
  return glm::mat4(
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    t.x, t.y, t.z, 1
  );
}

glm::mat4 scaling(float m) {
  return glm::mat4(
    m, 0, 0, 0,
    0, m, 0, 0,
    0, 0, m, 0,
    0, 0, 0, 1
  );
}

} // namespace