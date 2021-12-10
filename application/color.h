#ifndef _BLUSHER_COLOR_H
#define _BLUSHER_COLOR_H

#include <stdint.h>

typedef struct bl_color {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    uint32_t alpha;
} bl_color;

bl_color bl_color_from_rgb(uint32_t r, uint32_t g, uint32_t b);

bl_color bl_color_from_rgba(uint32_t r, uint32_t g, uint32_t b, uint32_t a);

uint32_t bl_color_to_argb(bl_color color);

#endif /* _BLUSHER_COLOR_H */
