#ifndef _BLUSHER_WINDOW_H
#define _BLUSHER_WINDOW_H

#include <stdlib.h>

#include <wayland-client.h>

#include <unstable/xdg-shell.h>

typedef struct bl_surface bl_surface;
typedef struct bl_title_bar bl_title_bar;

typedef struct bl_window {
    bl_surface *surface;

    struct zxdg_shell_v6 *xdg_shell;
    struct zxdg_surface_v6 *xdg_surface;
    struct zxdg_toplevel_v6 *xdg_toplevel;

    struct zxdg_shell_v6_listener xdg_shell_listener;
    struct zxdg_surface_v6_listener xdg_surface_listener;
    struct zxdg_toplevel_v6_listener xdg_toplevel_listener;

    int width;
    int height;
    const char *title;
    bl_title_bar *title_bar;
    bl_surface *body;
} bl_window;

bl_window* bl_window_new();

void bl_window_show(bl_window *window);

#endif /* _BLUSHER_WINDOW_H */
