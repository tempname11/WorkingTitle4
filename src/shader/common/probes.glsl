#include "constants.glsl"

float madfrac(float a, float b) {
  return a * b - floor(a * b);
}

// originally from DDGI sample code.
// see http://jcgt.org/published/0008/02/01/
vec3 spherical_fibonacci(float i, float n) {
  float phi = 2.0 * PI * madfrac(i, PHI - 1.0);
  float cos_theta = 1.0 - (2.0 * i + 1.0) / n;
  float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
  // there was a `saturate` in the original, but I think it's not needed.

  return vec3(
    cos(phi) * sin_theta,
    sin(phi) * sin_theta,
    cos_theta
  );
}

float sign_not_zero(float k) {
  return k >= 0.0 ? 1.0 : -1.0;
}

vec2 sign_not_zero(vec2 v) {
  return vec2(
    sign_not_zero(v.x),
    sign_not_zero(v.y)
  );
}

// see http://jcgt.org/published/0003/02/01/
vec2 octo_encode(vec3 v) {
  float l1norm = abs(v.x) + abs(v.y) + abs(v.z);
  vec2 result = v.xy / l1norm;
  if (v.z < 0.0) {
    result = (1.0 - abs(result.yx)) * sign_not_zero(result.xy);
  }
  return result;
}

// see http://jcgt.org/published/0003/02/01/
vec3 octo_decode(vec2 o) {
  vec3 v = vec3(o.x, o.y, 1.0 - abs(o.x) - abs(o.y));
  if (v.z < 0.0) {
    v.xy = (1.0 - abs(v.yx)) * sign_not_zero(v.xy);
  }
  return normalize(v);
}
