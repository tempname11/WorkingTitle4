#include <glm/glm.hpp>

namespace example {
  struct VS_UBO {
    glm::mat4 projection;
    glm::mat4 view;
  };

  struct FS_UBO {
    alignas(16) glm::vec3 camera_position;

    // light
    alignas(16) glm::vec3 light_position;
    alignas(16) glm::vec3 light_intensity;

    // material
    alignas(16) glm::vec3 albedo;
    float metallic;
    float roughness;
    float ao;
  };
}
