#ifndef _COMMON_HELPERS_GLSL_
#define _COMMON_HELPERS_GLSL_

vec3 barycentric_interpolate(vec2 b, vec3 v0, vec3 v1, vec3 v2) {
    return (1.0 - b.x - b.y) * v0 + b.x * v1 + b.y * v2;
}

#endif // _COMMON_HELPERS_GLSL_
