#ifndef _COMMON_SKY_GLSL_
#define _COMMON_SKY_GLSL_

#include "constants.glsl"

vec3 sky(vec3 ray, vec3 sun_direction, vec3 sun_illuminance, bool show_sun) {
  // Ad hoc stuff that looks okay.

  vec3 result = (
    (vec3(0.2, 0.2, 0.9) + 0.05 * ray)
      * sun_illuminance / 10.0
      * pow(max(0.0, sun_direction.z), 4.0)
      * (1.5 + dot(ray, sun_direction))
    + vec3(show_sun ? 1.0 : 0.0)
      * sun_illuminance / 2.0
      * pow(
        max(0.0, dot(ray, sun_direction)),
        200.0
      )
  );
  return result;
}

#endif // _COMMON_SKY_GLSL_