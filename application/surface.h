#ifndef _BLUSHER_SURFACE_H
#define _BLUSHER_SURFACE_H

#include <wayland-client.h>

#include "color.h"

typedef struct bl_pointer_event bl_pointer_event;

typedef struct bl_surface {
    struct bl_surface *parent;

    struct wl_surface *surface;
    struct wl_subsurface *subsurface;
    struct wl_callback *frame_callback;
    struct wl_buffer *buffer;

    void *shm_data;
    int shm_data_size;

    double x;
    double y;
    double width;
    double height;
    bl_color color;

    void (*pointer_move_event)(struct bl_surface*, bl_pointer_event*);
    void (*pointer_press_event)(struct bl_surface*, bl_pointer_event*);
    void (*pointer_release_event)(struct bl_surface*, bl_pointer_event*);
} bl_surface;

bl_surface* bl_surface_new(bl_surface *parent);

void bl_surface_set_geometry(bl_surface *surface,
        double x, double y, double width, double height);

void bl_surface_set_color(bl_surface *surface, const bl_color color);

void bl_surface_show(bl_surface *surface);

void bl_surface_free(bl_surface *surface);

#endif /* _BLUSHER_SURFACE_H */
