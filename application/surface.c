#include "surface.h"

#include <stdlib.h>

bl_surface* bl_surface_new()
{
    bl_surface *surface = malloc(sizeof(bl_surface));

    surface->surface = NULL;
    surface->subsurface = NULL;
    surface->frame_callback = NULL;

    surface->x = 0;
    surface->y = 0;
    surface->width = 0;
    surface->height = 0;

    return surface;
}

void bl_surface_set_geometry(bl_surface *surface,
        double x, double y, double width, double height)
{
    surface->x = x;
    surface->y = y;
    surface->width = width;
    surface->height = height;
}
