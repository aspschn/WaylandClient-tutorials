#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

#include "xdg-shell.h"

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct zxdg_shell_v6 *xdg_shell = NULL;
struct zxdg_surface_v6 *xdg_surface;
struct zxdg_toplevel_v6 *xdg_toplevel;
struct wl_shm *shm;
struct wl_buffer *buffer;

void *shm_data;

int WIDTH = 480;
int HEIGHT = 360;

//===============
// Xdg
//===============
static void xdg_shell_ping_handler(void *data, struct zxdg_shell_v6 *xdg_shell,
        uint32_t serial)
{
    zxdg_shell_v6_pong(xdg_shell, serial);
    printf("ping pong!\n");
}

static const struct zxdg_shell_v6_listener xdg_shell_listener = {
    .ping = xdg_shell_ping_handler,
};

static void xdg_toplevel_configure_handler(void *data,
        struct zxdg_toplevel_v6 *xdg_toplevel, int32_t width, int32_t height,
        struct wl_array *states)
{
    printf("Configure: %dx%d\n", width, height);
}

static void xdg_toplevel_close_handler(void *data,
        struct zxdg_toplevel_v6 *xdg_toplevel)
{
    printf("Close\n");
}

static const struct zxdg_toplevel_v6_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler,
};

static void xdg_surface_configure_handler(void *data,
        struct zxdg_surface_v6 *xdg_surface, uint32_t serial)
{
    fprintf(stderr, "xdg_surface_configure_handler().\n");
    zxdg_surface_v6_ack_configure(xdg_surface, serial);
}

static const struct zxdg_surface_v6_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler,
};

//=================
// File system
//=================
static int set_cloexec_or_close(int fd)
{
    long flags;

    if (fd == -1) {
        return -1;
    }

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
    fd=  mkostemp(tmpname, O_CLOEXEC);
    if (fd >= 0) {
        unlink(tmpname);
    }
#else
    fd = mkstemp(tmpname);
    if (fd >= 0) {
        fd = set_cloexec_or_close(fd);
        unlink(tmpname);
    }
#endif

    return fd;
}

int os_create_anonymous_file(off_t size)
{
    static const char template[] = "/blusher-shared-XXXXXX";
    const char *path;
    char *name;
    int fd;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path) {
        errno = ENOENT;
        return -1;
    }

    name = malloc(strlen(path) + sizeof(template));
    if (!name) {
        return -1;
    }
    strcpy(name, path);
    strcat(name, template);

    fd = create_tmpfile_cloexec(name);

    free(name);

    if (fd < 0) {
        return -1;
    }

    if (ftruncate(fd, size) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

//==============
// Painting
//==============
static void paint_pixels()
{
    uint32_t *pixel = shm_data;

    fprintf(stderr, "Painting pixels.\n");
    for (int n = 0; n < WIDTH * HEIGHT; ++n) {
        if (n > 1100 && n < 1200) {
            pixel[n] = 0xff0000;
        } else {
            pixel[n] = 0x00ff00;
        }
    }
}

static struct wl_buffer* create_buffer()
{
    struct wl_shm_pool *pool;
    int stride = WIDTH * 4; // 4 bytes per pixel
    int size = stride * HEIGHT;
    int fd;
    struct wl_buffer *buff;

    fd = os_create_anonymous_file(size);
    if (fd < 0) {
        fprintf(stderr, "Creating a buffer file for %d B filed: %m\n",
            size);
        exit(1);
    }

    shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_data == MAP_FAILED) {
        fprintf(stderr, "mmap filed: %m\n");
        close(fd);
        exit(1);
    }

    pool = wl_shm_create_pool(shm, fd, size);
    buff = wl_shm_pool_create_buffer(
        pool,
        0,
        WIDTH,
        HEIGHT,
        stride,
        WL_SHM_FORMAT_XRGB8888
    );
    wl_shm_pool_destroy(pool);
    return buff;
}

static void create_window()
{
    buffer = create_buffer();

    wl_surface_attach(surface, buffer, 0, 0);
    // wl_surface_damage(surface, 0, 0, WIDTH, HEIGHT);
    wl_surface_commit(surface);
}

static void shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    // struct display *d = data;

    // d->formats |= (1 << format);
    fprintf(stderr, "Format %d\n", format);
}

struct wl_shm_listener shm_listener = {
    shm_format,
};

//==============
// Global
//==============
static void global_registry_handler(void *data, struct wl_registry *registry,
        uint32_t id, const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0) {
        fprintf(stderr, "Interface is <wl_compositor>.\n");
        compositor = wl_registry_bind(
            registry,
            id,
            &wl_compositor_interface,
            version
        );
    } else if (strcmp(interface, "zxdg_shell_v6") == 0) {
        fprintf(stderr, "Interface is <zxdg_shell_v6>.\n");
        xdg_shell = wl_registry_bind(registry, id, &zxdg_shell_v6_interface, 1);
    } else if (strcmp(interface, "wl_shm") == 0) {
        fprintf(stderr, "Interface is <wl_shm>.\n");
        shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
        wl_shm_add_listener(shm, &shm_listener, NULL);
    } else {
//        printf("(%d) Got a registry event for <%s> id <%d>\n",
//            version, interface, id);
    }
}

static void global_registry_remover(void *data, struct wl_registry *registry,
        uint32_t id)
{
    printf("Got a registry losing event for <%d>\n", id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};

//==============
// Main
//==============
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Can't connect to display.\n");
        exit(1);
    }
    printf("Connected to display.\n");

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    // Check compositor.
    fprintf(stderr, " - Checking compositor...\n");
    if (compositor == NULL) {
        fprintf(stderr, "Can't find compositor.\n");
        exit(1);
    }

    // Check surface.
    fprintf(stderr, " - Checking surface...\n");
    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
        fprintf(stderr, "Can't create surface.\n");
        exit(1);
    }

    if (xdg_shell == NULL) {
        fprintf(stderr, "Haven't got a Xdg shell.\n");
        exit(1);
    }
    zxdg_shell_v6_add_listener(xdg_shell, &xdg_shell_listener, NULL);

    // Check shell surface.
    fprintf(stderr, " - Checking Xdg surface...\n");
    xdg_surface =
        zxdg_shell_v6_get_xdg_surface(xdg_shell, surface);
    if (xdg_surface == NULL) {
        fprintf(stderr, "Can't create Xdg surface.\n");
        exit(1);
    }
    zxdg_surface_v6_add_listener(xdg_surface, &xdg_surface_listener, NULL);

    xdg_toplevel = zxdg_surface_v6_get_toplevel(xdg_surface);
    zxdg_toplevel_v6_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);

    // Signal that the surface is ready to be configured.
    wl_surface_commit(surface);

    create_window();
    paint_pixels();

    // Wait for the surface to be configured.
    wl_display_roundtrip(display);
    fprintf(stderr, "After roundtrip\n");

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);

    fprintf(stderr, "Before loop\n");
    while (wl_display_dispatch(display) != -1) {
        ;
    }

    wl_display_disconnect(display);
    printf("Disconnected from display.\n");

    return 0;
}
