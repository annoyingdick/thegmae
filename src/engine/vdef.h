#define VEC3DUP(x) x.raw[0], x.raw[1], x.raw[2]
#define VEC2XZ(x, a) (vec3s){{x.raw[0] * a, 0, x.raw[1] * a}}
#define VEC3XZ2(x) (vec2s){{x.raw[0], x.raw[2]}}
