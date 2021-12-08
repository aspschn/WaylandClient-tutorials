#include "window.h"

#include <stdio.h>

#include "application.h"

//==============
// Xdg
//==============

// Xdg shell
static void xdg_shell_ping_handler(void *data,
        struct zxdg_shell_v6 *xdg_shell, uint32_t serial)
{
    zxdg_shell_v6_pong(xdg_shell, serial);
}

static const struct zxdg_shell_v6_listener xdg_shell_listener = {
    .ping = xdg_shell_ping_handler,
};

// Xdg surface
static void xdg_surface_configure_handler(void *data,
        struct zxdg_surface_v6 *xdg_surface, uint32_t serial)
{
    zxdg_surface_v6_ack_configure(xdg_surface, serial);
}

static const struct zxdg_surface_v6_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler,
};

// Toplevel
static void xdg_toplevel_configure_handler(void *data,
        struct zxdg_toplevel_v6 *xdg_toplevel, int32_t width, int32_t height,
        struct wl_array *states)
{
    // printf("TOPLEVEL Configure: %dx%d\n", width, height);
}

static void xdg_toplevel_close_handler(void *data,
        struct zxdg_toplevel_v6 *xdg_toplevel)
{
    // printf("TOPLEVEL Close %p\n", xdg_toplevel);
}

static const struct zxdg_toplevel_v6_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler,
};

//=============
// Window
//=============

bl_window* bl_window_new()
{
    bl_window *window = malloc(sizeof(bl_window));

    window->xdg_shell = NULL;
    window->xdg_surface = NULL;
    window->xdg_toplevel = NULL;

    window->xdg_shell_listener = xdg_shell_listener;
    window->xdg_surface_listener = xdg_surface_listener;
    window->xdg_toplevel_listener = xdg_toplevel_listener;

    if (bl_app == NULL) {
        fprintf(stderr, "bl_app is NULL.\n");
    }
    window->surface = wl_compositor_create_surface(bl_app->compositor);

    window->width = 480;
    window->height = 360;

    return window;
}

void bl_window_show(bl_window *window)
{
    zxdg_shell_v6_add_listener(window->xdg_shell,
        &(window->xdg_shell_listener), NULL);

    window->xdg_surface = zxdg_shell_v6_get_xdg_surface(window->xdg_shell,
        window->surface);
    zxdg_surface_v6_add_listener(window->xdg_surface,
        &(window->xdg_surface_listener), NULL);

    window->xdg_toplevel = zxdg_surface_v6_get_toplevel(window->xdg_surface);
    zxdg_toplevel_v6_add_listener(window->xdg_toplevel,
        &(window->xdg_toplevel_listener), NULL);
}
