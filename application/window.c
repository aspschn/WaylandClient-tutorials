#include "window.h"

#include <stdio.h>

#include <sys/mman.h>
#include <unistd.h>

#include "application.h"
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
        void *shm_data, struct wl_shm *shm)
{
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

    shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_data == MAP_FAILED) {
        close(fd);
        exit(1);
    }

    pool = wl_shm_create_pool(shm, fd, size);
    buff = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
        WL_SHM_FORMAT_XRGB8888);

    wl_shm_pool_destroy(pool);
    return buff;
}

static void paint_pixels(int size, void *shm_data)
{
    uint32_t *pixel = shm_data;

    for (int n = 0; n < size; ++n) {
        *pixel++ = 0xff0000;
    }
}

static void create_window_surface(bl_window *window,
        struct wl_buffer *buffer, void *shm_data, struct wl_shm *shm)
{
    buffer = create_buffer(window->width, window->height, shm_data, shm);

    wl_surface_attach(window->surface, buffer, 0, 0);
    wl_surface_commit(window->surface);
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
