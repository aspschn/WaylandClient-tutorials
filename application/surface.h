#ifndef _BL_SURFACE_H
#define _BL_SURFACE_H

#include <wayland-client.h>

typedef struct bl_pointer_event bl_pointer_event;

typedef struct bl_surface {
    struct wl_surface *surface;
    struct wl_subsurface *subsurface;
    struct wl_callback *frame_callback;

    double x;
    double y;
    double width;
    double height;

    void (*pointer_press_event)(bl_pointer_event*);
} bl_surface;

bl_surface* bl_surface_new();

void bl_surface_set_geometry(bl_surface *surface,
        double x, double y, double width, double height);

#endif /* _BL_SURFACE_H */
