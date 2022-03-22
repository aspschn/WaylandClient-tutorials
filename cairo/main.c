#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

// Cairo
#include <cairo.h>

#include <wayland-client.h>

#include "xdg-shell.h"

struct wl_display *display = NULL;
struct wl_registry *registry = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface = NULL;

struct wl_shm *shm = NULL;
struct wl_buffer *buffer = NULL;
void *shm_data;

struct xdg_wm_base *xdg_wm_base = NULL;
struct xdg_surface *xdg_surface = NULL;
struct xdg_toplevel *xdg_toplevel = NULL;

//=================
// Shared Memory
//=================
static int set_cloexec_or_close(int fd)
{
    long flags;

    if (fd == -1)
        return -1;

    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
        goto err;

    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
        goto err;

    return fd;

err:
    close(fd);
    return -1;
}

static int create_tmpfile_cloexec(char *tmpname)
{
    int fd;

#ifdef HAVE_MKOSTEMP
    fd = mkostemp(tmpname, O_CLOEXEC);
    if (fd >= 0)
        unlink(tmpname);
#else
    fd = mkstemp(tmpname);
    if (fd >= 0) {
        fd = set_cloexec_or_close(fd);
        unlink(tmpname);
    }
#endif

    return fd;
}

/*
 * Create a new, unique, anonymous file of the given size, and
 * return the file descriptor for it. The file descriptor is set
 * CLOEXEC. The file is immediately suitable for mmap()'ing
 * the given size at offset zero.
 *
 * The file should not have a permanent backing store like a disk,
 * but may have if XDG_RUNTIME_DIR is not properly implemented in OS.
 *
 * The file name is deleted from the file system.
 *
 * The file is suitable for buffer sharing between processes by
 * transmitting the file descriptor over Unix sockets using the
 * SCM_RIGHTS methods.
 */
int os_create_anonymous_file(off_t size)
{
    static const char template[] = "/tutorial-shared-XXXXXX";
    const char *path;
    char *name;
    int fd;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        errno = ENOENT;
        return -1;
    }

    name = malloc(strlen(path) + sizeof(template));
    if (!name)
        return -1;
    strcpy(name, path);
    strcat(name, template);

    fd = create_tmpfile_cloexec(name);

    free(name);

    if (fd < 0)
        return -1;

    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

//=============
// Shm
//=============
static void shm_format_handler(void *data,
        struct wl_shm *shm, uint32_t format)
{
    (void)data;
    (void)shm;

    fprintf(stderr, "Format %d\n", format);
}

static const struct wl_shm_listener shm_listener = {
    .format = shm_format_handler,
};

//==============
// XDG
//==============
static void xdg_wm_base_ping_handler(void *data,
        struct xdg_wm_base *wm_base, uint32_t serial)
{
    (void)data;

    xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping_handler,
};

static void xdg_surface_configure_handler(void *data,
        struct xdg_surface *xdg_surface, uint32_t serial)
{
    (void)data;

    xdg_surface_ack_configure(xdg_surface, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler,
};

static void xdg_toplevel_configure_handler(void *data,
        struct xdg_toplevel *toplevel, int32_t width, int32_t height,
        struct wl_array *states)
{
    (void)data;
    (void)toplevel;
    (void)states;

    fprintf(stderr, "XDG toplevel configure: %dx%d\n", width, height);
}

static void xdg_toplevel_close_handler(void *data,
        struct xdg_toplevel *toplevel)
{
    (void)data;
    (void)toplevel;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler,
};

//==============
// Global
//==============
static void global_registry_handler(void *data, struct wl_registry *registry,
        uint32_t id, const char *interface, uint32_t version)
{
    printf("Got a registry event for <%s>, id: %d, version: %d.\n",
        interface, id, version);
    if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(registry,
            id, &wl_compositor_interface, 4);
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        xdg_wm_base = wl_registry_bind(registry,
            id, &xdg_wm_base_interface, 1);
    } else if (strcmp(interface, "wl_shm") == 0) {
        shm = wl_registry_bind(registry,
            id, &wl_shm_interface, 1);
        wl_shm_add_listener(shm, &shm_listener, NULL);
    }
}

static void global_registry_remove_handler(void *data,
        struct wl_registry *registry, uint32_t id)
{
    printf("Got a registry losing event for <%d>\n", id);
}

static const struct wl_registry_listener registry_listener = {
    .global = global_registry_handler,
    .global_remove = global_registry_remove_handler,
};

//==========
// Buffer
//==========
static struct wl_buffer* create_buffer(int width, int height)
{
    struct wl_shm_pool *pool;
    int stride = width * 4; // 4 bytes per pixel in our ARGB8888 format.
    int size = stride * height;
    int fd;
    struct wl_buffer *buff;

    fd = os_create_anonymous_file(size);
    if (fd < 0) {
        fprintf(stderr, "Failed to create a buffer. size: %d\n", size);
        exit(1);
    }

    shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_data == MAP_FAILED) {
        fprintf(stderr, "mmap failed!\n");
        close(fd);
        exit(1);
    }

    pool = wl_shm_create_pool(shm, fd, size);
    buff = wl_shm_pool_create_buffer(pool, 0, width, height, stride,
        WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);

    return buff;
}

//==========
// Main
//==========
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Failed to connect to display.\n");
        exit(1);
    }
    printf("Connected to display.\n");

    registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    // Check compositor.
    fprintf(stderr, " - Checking compositor...\n");
    if (compositor == NULL) {
        fprintf(stderr, "Can't find compositor.\n");
        exit(1);
    } else {
        printf("Found compositor!\n");
    }

    // Surface
    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
        exit(1);
    }

    // XDG.
    xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);

    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);

    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);

    // This step is important on Weston.
    wl_surface_commit(surface);
    wl_display_roundtrip(display);

    // Create a buffer.
    buffer = create_buffer(480, 360);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);

    // Drawing pixels.
    uint32_t *pixel = shm_data;
    for (int i = 0; i < 480 * 360; ++i) {
        *pixel = 0xde000000;
        ++pixel;
    }

    // Cairo
    cairo_surface_t *cairo_surface = cairo_image_surface_create_from_png(
        "nyan-cat.png");
    if (cairo_surface_status(cairo_surface) != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to load PNG image.\n");
        exit(1);
    }
    unsigned char *image_data = cairo_image_surface_get_data(cairo_surface);
    int image_width = cairo_image_surface_get_width(cairo_surface);
    int image_height = cairo_image_surface_get_height(cairo_surface);

    uint32_t *image_pixel = (uint32_t*)image_data;
    uint32_t *target = (uint32_t*)shm_data;
    for (int i = 0; i < image_height; ++i) {
        for (int j = 0; j < image_width; ++j) {
            *target = *image_pixel++;
            ++target;
            // Because our surface is larget than image, skip to next line.
            if (j == image_width - 1) {
                target += (480 - image_width);
            }
        }
    }

    cairo_surface_destroy(cairo_surface);

    // Display loop.
    while (wl_display_dispatch(display) != -1) {
        ;
    }

    // Free resources.
    xdg_toplevel_destroy(xdg_toplevel);

    xdg_surface_destroy(xdg_surface);

    wl_surface_destroy(surface);

    wl_display_disconnect(display);
    printf("Disconnected from display.\n");

    return 0;
}
