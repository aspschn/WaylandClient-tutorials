#include "color.h"

bl_color bl_color_from_rgb(uint32_t r, uint32_t g, uint32_t b)
{
    bl_color color = {
        .red = r,
        .green = g,
        .blue = b,
        .alpha = 255,
    };

    return color;
}

bl_color bl_color_from_rgba(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
{
    bl_color color = {
        .red = r,
        .green = g,
        .blue = b,
        .alpha = a,
    };

    return color;
}

uint32_t bl_color_to_argb(bl_color color)
{
    uint32_t ret = 0x00000000;

    ret += (color.red) << 16;
    ret += (color.green) << 8;
    ret += (color.blue) << 0;
    ret += (color.alpha) << 24;

    return ret;
}

