#ifndef RAY_H
#define RAY_H
#include "config.h"
#include "vec3.h"

typedef struct ray
{
    Vec3 origin;
    Vec3 direction;
    rt_float time; /* using double for more precision (todo: check if we can use float) */
} Ray;

static inline Vec3 ray_point_param(Ray self, rt_float t)
{
    return vec3_add(self.origin, vec3_mul_val(self.direction, t));
}

#endif
