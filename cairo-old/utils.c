#include "utils.h"

double pixel_to_pango_size(double pixel)
{
    return (pixel * 0.75) * PANGO_SCALE;
}
