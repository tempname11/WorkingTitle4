#include <glm/glm.hpp>

namespace example {
  struct UBO_Frame {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 projection_inverse;
    glm::mat4 view_inverse;
  };

  struct UBO_DirectionalLight {
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 intensity;
  };

  struct UBO_Material {
    alignas(16) glm::vec3 albedo;
    float metallic;
    float roughness;
    float ao;
  };
}
