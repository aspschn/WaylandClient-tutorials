#include "window.h"

#include <stdio.h>

#include <sys/mman.h>
#include <unistd.h>

#include "application.h"
#include "surface.h"
#include "title-bar.h"
#include "pointer-event.h"
#include "utils.h"

//==============
// Xdg
//==============

// Xdg shell
static void xdg_wm_base_ping_handler(void *data,
        struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping_handler,
};

// Xdg surface
static void xdg_surface_configure_handler(void *data,
        struct xdg_surface *xdg_surface, uint32_t serial)
{
    fprintf(stderr, "xdg_surface_configure_handler.\n");
    xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler,
};

// Toplevel
static void xdg_toplevel_configure_handler(void *data,
        struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height,
        struct wl_array *states)
{
    // printf("TOPLEVEL Configure: %dx%d\n", width, height);
}

static void xdg_toplevel_close_handler(void *data,
        struct xdg_toplevel *xdg_toplevel)
{
    // printf("TOPLEVEL Close %p\n", xdg_toplevel);
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler,
};

//=============
// Drawing
//=============

static void paint_pixels(int size, uint32_t color, void **shm_data)
{
    fprintf(stderr, "paint_pixels. shm_data: %p\n", *shm_data);
    uint32_t *pixel = *shm_data;

    for (int n = 0; n < size; ++n) {
        *pixel++ = color;
    }
}

static void create_window_surface(bl_surface *window_surface)
{
    fprintf(stderr, "create_window_surface. shm_data: %p\n",
        window_surface->shm_data);

    wl_surface_attach(window_surface->surface,
        window_surface->buffer, 0, 0);
    wl_surface_commit(window_surface->surface);

//    wl_buffer_destroy(buffer);
}

static void create_title_bar_surface(bl_surface *title_bar)
{
    wl_surface_attach(title_bar->surface, title_bar->buffer, 0, 0);
    wl_surface_commit(title_bar->surface);

//    wl_buffer_destroy(buffer);
}

//=============
// Window
//=============

bl_window* bl_window_new()
{
    bl_window *window = malloc(sizeof(bl_window));

    window->xdg_wm_base = NULL;
    window->xdg_surface = NULL;
    window->xdg_toplevel = NULL;

    window->xdg_wm_base_listener = xdg_wm_base_listener;
    window->xdg_surface_listener = xdg_surface_listener;
    window->xdg_toplevel_listener = xdg_toplevel_listener;

    if (bl_app == NULL) {
        fprintf(stderr, "bl_app is NULL.\n");
    }
    window->surface = bl_surface_new(NULL);

    window->width = 480;
    window->height = 360;
    window->title = "Window";

    window->title_bar = NULL;
    window->body = NULL;

    return window;
}

// TEST!!
static void frame_done(void *data, struct wl_callback *callback, uint32_t time);

static const struct wl_callback_listener window_listener = {
    .done = frame_done,
};

static void frame_done(void *data, struct wl_callback *callback, uint32_t time)
{
    bl_surface *window_surface = (bl_surface*)data;

    wl_callback_destroy(callback);
    wl_surface_damage(window_surface->surface, 0, 0, 100, 100);
    fprintf(stderr, "DRAW!!!!!!!\n");

    struct wl_buffer *window_buffer = NULL;
    create_window_surface(window_surface);
    paint_pixels(480 * 360, 0xff0000, &(window_surface->shm_data));

    window_surface->frame_callback = wl_surface_frame(window_surface->surface);
    wl_surface_attach(window_surface->surface, window_buffer, 0, 0);
    wl_callback_add_listener(window_surface->frame_callback,
        &window_listener, (void*)window_surface);
    wl_surface_commit(window_surface->surface);
}

static void frame_done_tb(void *data, struct wl_callback *callback, uint32_t time);

static const struct wl_callback_listener listener = {
    .done = frame_done_tb,
};

static void frame_done_tb(void *data, struct wl_callback *callback, uint32_t time)
{
    bl_surface *title_bar = (bl_surface*)data;
    wl_callback_destroy(callback);
    wl_surface_damage(title_bar->surface, 0, 0, 40, 100);
    fprintf(stderr, "DRAW!!\n");

    create_title_bar_surface(title_bar);
    paint_pixels(title_bar->width * title_bar->height, 0x00ff00,
        &(title_bar->shm_data));

    title_bar->frame_callback = wl_surface_frame(title_bar->surface);
    wl_surface_attach(title_bar->surface, title_bar->buffer, 0, 0);
    wl_callback_add_listener(title_bar->frame_callback,
        &listener, (void*)(title_bar));
    wl_surface_commit(title_bar->surface);
}

static void title_bar_pointer_move_handler(bl_surface *surface,
        bl_pointer_event *event)
{
}

static void title_bar_pointer_press_handler(bl_surface *surface,
        bl_pointer_event *event)
{
    fprintf(stderr, "You have pressed button %d on title bar, (%d, %d)\n",
        event->button, event->x, event->y);

    if (event->button == BTN_LEFT) {
        xdg_toplevel_move(bl_app->toplevel_windows[0]->xdg_toplevel,
            bl_app->seat, event->serial);
    }

    bl_pointer_event_free(event);
}
// TEST END!!

void bl_window_show(bl_window *window)
{
    xdg_wm_base_add_listener(window->xdg_wm_base,
        &(window->xdg_wm_base_listener), NULL);

    window->xdg_surface = xdg_wm_base_get_xdg_surface(window->xdg_wm_base,
        window->surface->surface);
    xdg_surface_add_listener(window->xdg_surface,
        &(window->xdg_surface_listener), NULL);

    window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
    xdg_toplevel_add_listener(window->xdg_toplevel,
        &(window->xdg_toplevel_listener), NULL);

    // Signal that the surface is ready to be configured.
    wl_surface_commit(window->surface->surface);

    // Wait for the surface to be configured.
    wl_display_roundtrip(bl_app->display);

    // Draw window surface.
    bl_surface_set_geometry(window->surface,
        0, 0, window->width, window->height);
    create_window_surface(window->surface);
    fprintf(stderr, "Before paint_pixels. shm_data: %p\n", window->surface->shm_data);
    paint_pixels(window->width * window->height, 0xffd6d1ce,
        &(window->surface->shm_data));

    // Draw title bar.
    window->title_bar = bl_title_bar_new(window);
    window->title_bar->surface->pointer_move_event =
        title_bar_pointer_move_handler;
    window->title_bar->surface->pointer_press_event =
        title_bar_pointer_press_handler;
    bl_title_bar_show(window->title_bar);

    // TEST!
//    window->surface->frame_callback =
//        wl_surface_frame(window->surface->surface);
//    wl_callback_add_listener(window->surface->frame_callback,
//        &window_listener, (void*)(window->surface));
//    wl_surface_commit(window->surface->surface);

    window->title_bar->surface->frame_callback =
        wl_surface_frame(window->title_bar->surface->surface);
    wl_callback_add_listener(window->title_bar->surface->frame_callback,
        &listener, (void*)(window->title_bar->surface));
    wl_surface_commit(window->title_bar->surface->surface);
    wl_surface_commit(window->surface->surface);
}

void bl_window_free(bl_window *window)
{
    bl_title_bar_free(window->title_bar);
    bl_surface_free(window->surface);

    free(window);
}
