#pragma once
#include "public.hxx"

#define DLL_EXPORT extern "C" __declspec(dllexport)

namespace engine::system::artline {

const glm::mat4 id = glm::mat4(1.0f);

float cube(glm::vec3 position, glm::vec3 center, float halfwidth);
float sphere(glm::vec3 position, glm::vec3 center, float radius);

glm::vec2 triplanar_texture_uv(glm::vec3 position, glm::vec3 N);
glm::vec2 ad_hoc_sphere_texture_uv(glm::vec3 position, glm::vec3 N);

glm::mat4 rotation(float turns, glm::vec3 axis);
glm::mat4 translation(glm::vec3 t);
glm::mat4 scaling(float m);

} // namespace