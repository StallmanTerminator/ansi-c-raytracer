
#include "mixture.h"
#include "../utils.h"
Pdf make_mixture_pdf(Pdf *first, Pdf *second)
{
    Pdf self = {};
    self.type = PDF_MIXTURE;
    self.data.mixture.first = first;
    self.data.mixture.second = second;
    return self;
}

rt_float mixture_pdf_value(Vec3 direction, const PdfData *self)
{
    return (pdf_value(direction, self->mixture.first) +
            pdf_value(direction, self->mixture.second)) /
           2;
}

Vec3 mixture_pdf_generate(const PdfData *self)
{
    if (random_rt_float() > 0.5)
    {
        return pdf_generate(self->mixture.first);
    }
    else
    {
        return pdf_generate(self->mixture.second);
    }
}
