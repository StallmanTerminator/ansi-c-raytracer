#include "isotropic.h"
Material isotropic_create(Texture albedo)
{
    Isotropic *iso = malloc(sizeof(Isotropic));

    iso->albedo = albedo;

    Material res = {
        .type = MATERIAL_ISOTROPIC,
        .data = iso,
    };

    return res;
}

bool isotropic_callback(const Ray *r_in, const HitRecord *record, MaterialRecord *mat_record, const Isotropic *self)
{

    mat_record->scattered = (Ray){
        .direction = random_vec3_unit_in_disk(),
        .origin = record->pos,
        .time = r_in->time,
    };
    mat_record->is_specular = false;
    mat_record->pdf.type = PDF_NONE;
    mat_record->attenuation = texture_get(record->u, record->v, &record->pos, &self->albedo);
    return true;
}
