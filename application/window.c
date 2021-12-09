#include "window.h"

#include <stdio.h>

#include <sys/mman.h>
#include <unistd.h>

#include "application.h"
#include "surface.h"
#include "utils.h"

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
    fprintf(stderr, "xdg_surface_configure_handler.\n");
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
// Drawing
//=============
static struct wl_buffer* create_buffer(int width, int height,
        void **shm_data, struct wl_shm *shm)
{
    fprintf(stderr, "create_buffer: %dx%d\n", width, height);
    struct wl_shm_pool *pool;
    int stride = width * 4;
    int size = stride * height;
    int fd;
    struct wl_buffer *buff;

    fd = os_create_anonymous_file(size);
    if (fd < 0) {
        fprintf(stderr, "Creating a buffer file for %d B failed: %m\n",
            size);
        exit(1);
    }

    fprintf(stderr, "Before mmap.");
    fprintf(stderr, " shm_data: %p\n", *shm_data);
    *shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    fprintf(stderr, "After mmap. shm_data: %p\n", *shm_data);
    if (*shm_data == MAP_FAILED) {
        close(fd);
        exit(1);
    }

    pool = wl_shm_create_pool(shm, fd, size);
    buff = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
        WL_SHM_FORMAT_XRGB8888);

    wl_shm_pool_destroy(pool);
    return buff;
}

static void paint_pixels(int size, uint32_t color, void **shm_data)
{
    fprintf(stderr, "paint_pixels. shm_data: %p\n", *shm_data);
    uint32_t *pixel = *shm_data;

    for (int n = 0; n < size; ++n) {
        *pixel++ = color;
    }
}

static void create_window_surface(bl_surface *window,
        struct wl_buffer *buffer, void **shm_data, struct wl_shm *shm)
{
    buffer = create_buffer(480, 360, shm_data, shm);
    fprintf(stderr, "create_window_surface. shm_data: %p\n", *shm_data);

    wl_surface_attach(window->surface, buffer, 0, 0);
    wl_surface_commit(window->surface);

//    wl_buffer_destroy(buffer);
}

static void create_title_bar_surface(bl_surface *title_bar,
        struct wl_buffer *buffer, void **shm_data, struct wl_shm *shm)
{
    fprintf(stderr, "create_title_bar_surface: %dx%d\n",
        (int)title_bar->width, (int)title_bar->height);
    buffer = create_buffer(title_bar->width, title_bar->height, shm_data, shm);

    wl_surface_attach(title_bar->surface, buffer, 0, 0);
    wl_surface_commit(title_bar->surface);

//    wl_buffer_destroy(buffer);
}

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
    window->surface = bl_surface_new();

    window->width = 480;
    window->height = 360;

    window->title_bar = NULL;

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

    void *shm_data = NULL;
    struct wl_buffer *window_buffer = NULL;
    create_window_surface(window_surface,
        window_buffer, &shm_data, bl_app->shm);
    paint_pixels(480 * 360, 0xff0000, &shm_data);

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
    wl_surface_damage(title_bar->surface, 0, 0, 100, 100);
    fprintf(stderr, "DRAW!!\n");

    void *shm_data = NULL;
    struct wl_buffer *title_bar_buffer = NULL;
    create_title_bar_surface(title_bar,
        title_bar_buffer, &shm_data, bl_app->shm);
    paint_pixels(title_bar->width * title_bar->height, 0x00ff00, &shm_data);

    title_bar->frame_callback = wl_surface_frame(title_bar->surface);
    wl_surface_attach(title_bar->surface, title_bar_buffer, 0, 0);
    wl_callback_add_listener(title_bar->frame_callback,
        &listener, (void*)(title_bar));
    wl_surface_commit(title_bar->surface);
}
// TEST END!!

void bl_window_show(bl_window *window)
{
    zxdg_shell_v6_add_listener(window->xdg_shell,
        &(window->xdg_shell_listener), NULL);

    window->xdg_surface = zxdg_shell_v6_get_xdg_surface(window->xdg_shell,
        window->surface->surface);
    zxdg_surface_v6_add_listener(window->xdg_surface,
        &(window->xdg_surface_listener), NULL);

    window->xdg_toplevel = zxdg_surface_v6_get_toplevel(window->xdg_surface);
    zxdg_toplevel_v6_add_listener(window->xdg_toplevel,
        &(window->xdg_toplevel_listener), NULL);

    // Signal that the surface is ready to be configured.
    wl_surface_commit(window->surface->surface);

    // Wait for the surface to be configured.
    wl_display_roundtrip(bl_app->display);

    // Draw window surface.
    void *shm_data = NULL;
    struct wl_buffer *buffer = NULL;
    create_window_surface(window->surface, buffer, &shm_data, bl_app->shm);
    paint_pixels(window->width * window->height, 0xff0000, &shm_data);

    // Draw title bar.
    struct wl_buffer *title_bar_buffer = NULL;
    window->title_bar = bl_surface_new();
    bl_surface_set_geometry(window->title_bar, 0, 0, window->width, 30);
    wl_surface_commit(window->title_bar->surface);
    window->title_bar->subsurface =
        wl_subcompositor_get_subsurface(bl_app->subcompositor,
            window->title_bar->surface, window->surface->surface);
    create_title_bar_surface(window->title_bar,
        title_bar_buffer, &shm_data, bl_app->shm);
    paint_pixels(window->width * 30, 0x00ff00, &shm_data);

    // TEST!
//    window->surface->frame_callback =
//        wl_surface_frame(window->surface->surface);
//    wl_callback_add_listener(window->surface->frame_callback,
//        &window_listener, (void*)(window->surface));
//    wl_surface_commit(window->surface->surface);

    window->title_bar->frame_callback =
        wl_surface_frame(window->title_bar->surface);
    wl_callback_add_listener(window->title_bar->frame_callback,
        &listener, (void*)(window->title_bar));
    wl_surface_commit(window->title_bar->surface);
    wl_surface_commit(window->surface->surface);
}
