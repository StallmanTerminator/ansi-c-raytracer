#ifndef __SHAPE_H__
#define __SHAPE_H__

#include "../aabb.h"

typedef enum
{
    SHAPE_NONE,
    SHAPE_BOX,
    SHAPE_AAREC_XY,
    SHAPE_AAREC_XZ,
    SHAPE_AAREC_YZ,
    SHAPE_MOVING_SPHERE,
    SHAPE_SPHERE,
    SHAPE_TRANSLATE,
    SHAPE_BVH,
    SHAPE_HITABLE_LIST,

} ShapeTypes;

typedef struct
{
    uint32_t uid;
    bool is_leaf;
    void *data;
    ShapeTypes type;
} Object;

typedef struct hit_record HitRecord;

bool object_collide(Ray r, rt_float t_min, rt_float t_max, HitRecord *record, Object *self);

bool object_get_aabb(rt_float time_start, rt_float time_end, AABB *output, Object *self);

bool object_destroy(Object *self);

#endif /* B0BAA9D8_C3FE_4855_9A2C_59A9028F887C */