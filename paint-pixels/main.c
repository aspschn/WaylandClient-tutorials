#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_shell *shell = NULL;
struct wl_shell_surface *shell_surface;
struct wl_shm *shm;
struct wl_buffer *buffer;

void *shm_data;

int WIDTH = 480;
int HEIGHT = 360;

//==========================
// Shell surface listener
//==========================
static void handle_ping(void *data, struct wl_shell_surface *shell_surface,
        uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
    fprintf(stderr, "Ping pong!\n");
}

static void handle_configure(void *data, struct wl_shell_surface *shell_surface,
        uint32_t edges, int32_t width, int32_t height)
{
    fprintf(stderr, "handle_configure() - edges: %d, width: %d, height %d.\n",
        edges, width, height);
}

static void handle_popup_done(void *data,
        struct wl_shell_surface *shell_surface)
{
    fprintf(stderr, "handle_popup_done()\n");
}

static const struct wl_shell_surface_listener shell_surface_listener = {
    handle_ping,
    handle_configure,
    handle_popup_done,
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
    } else if (strcmp(interface, "wl_shell") == 0) {
        shell = wl_registry_bind(registry, id, &wl_shell_interface, 1);
    } else if (strcmp(interface, "wl_shm") == 0) {
        fprintf(stderr, "Interface is <wl_shm>.\n");
        shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
        wl_shm_add_listener(shm, &shm_listener, NULL);
    } else {
        printf("(%d) Got a registry event for <%s> id <%d>\n",
            version, interface, id);
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
    } else {
        fprintf(stderr, "Found compositor!\n");
    }

    // Check surface.
    fprintf(stderr, " - Checking surface...\n");
    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
        fprintf(stderr, "Can't create surface.\n");
        exit(1);
    } else {
        fprintf(stderr, "Created surface!\n");
    }

    if (shell == NULL) {
        fprintf(stderr, "Haven't got a Wayland shell.\n");
        exit(1);
    }

    // Check shell surface.
    fprintf(stderr, " - Checking shell surface...\n");
    shell_surface = wl_shell_get_shell_surface(shell, surface);
    if (shell_surface == NULL) {
        fprintf(stderr, "Can't create shell surface.\n");
        exit(1);
    } else {
        fprintf(stderr, "Created shell surface!\n");
    }
    wl_shell_surface_set_toplevel(shell_surface);

    wl_shell_surface_add_listener(shell_surface, &shell_surface_listener, NULL);

    create_window();
    paint_pixels();

    while (wl_display_dispatch(display) != -1) {
        ;
    }

    wl_display_disconnect(display);
    printf("Disconnected from display.\n");

    return 0;
}
