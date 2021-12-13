#ifndef _BLUSHER_WINDOW_H
#define _BLUSHER_WINDOW_H

#include <stdlib.h>

#include <wayland-client.h>

//#include <unstable/xdg-shell.h>
#include <stable/xdg-shell.h>

typedef struct bl_surface bl_surface;
typedef struct bl_title_bar bl_title_bar;

typedef struct bl_window {
    bl_surface *surface;

    struct xdg_wm_base *xdg_wm_base;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

    struct xdg_wm_base_listener xdg_wm_base_listener;
    struct xdg_surface_listener xdg_surface_listener;
    struct xdg_toplevel_listener xdg_toplevel_listener;

    int width;
    int height;
    const char *title;
    bl_title_bar *title_bar;
    bl_surface *body;
} bl_window;

bl_window* bl_window_new();

void bl_window_show(bl_window *window);

/// \brief Free the window. Should not call manually.
void bl_window_free(bl_window *window);

#endif /* _BLUSHER_WINDOW_H */
