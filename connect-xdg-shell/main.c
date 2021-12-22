#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>

#include "xdg-shell.h"

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
//struct wl_shell *shell = NULL;
struct xdg_wm_base *wm_base = NULL;
struct xdg_surface *xdg_surface;
struct wl_shm *shm;
struct wl_output *output = NULL;

//==============
// Xdg
//==============
void xdg_toplevel_configure_handler(void *data,
        struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height,
        struct wl_array *states)
{
    printf("Configure: %dx%d\n", width, height);
}

void xdg_toplevel_close_handler(void *data,
        struct xdg_toplevel *xdg_toplevel)
{
    printf("Close.\n");
}

const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler,
};

void xdg_surface_configure_handler(void *data,
        struct xdg_surface *xdg_surface, uint32_t serial)
{
    xdg_surface_ack_configure(xdg_surface, serial);
}

const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler,
};

//==============
// Output
//==============
void wl_output_geometry_handler(void *data, struct wl_output *output,
        int x, int y, int physical_width, int physical_height,
        int subpixel, const char *make, const char *model,
        int transform)
{
    enum wl_output_subpixel enum_subpixel = subpixel;
    enum wl_output_transform enum_transform = transform;

    fprintf(stderr, "wl_output_geometry_handler()\n");
    fprintf(stderr,
        " - x: %d, y: %d, physical_width: %d, physical_height: %d\n",
        x, y, physical_width, physical_height);
    fprintf(stderr,
        " - subpixel: %d\n", enum_subpixel);
    fprintf(stderr,
        " - make: %s, model: %s\n", make, model);
    fprintf(stderr,
        " - transform: %d\n", enum_transform);
}

void wl_output_mode_handler(void *data, struct wl_output *output,
        unsigned int flags,
        int width, int height, int refresh)
{
    fprintf(stderr, "wl_output_mode_handler()\n");
    fprintf(stderr, " - width: %d, height: %d, refresh: %d\n",
        width, height, refresh);
}

void wl_output_scale_handler(void *data, struct wl_output *output,
        int factor)
{
    fprintf(stderr, "wl_output_scale_handler()\n");
    fprintf(stderr, " - factor: %d\n", factor);
}

void wl_output_done_handler(void *data, struct wl_output *output)
{
    fprintf(stderr, "wl_output_done_handler()\n");
}

const struct wl_output_listener wl_output_listener = {
    .geometry = wl_output_geometry_handler,
    .mode = wl_output_mode_handler,
    .done = wl_output_done_handler,
    .scale = wl_output_scale_handler,
};

//==============
// Global
//==============
static void global_registry_handler(void *data, struct wl_registry *registry,
        uint32_t id, const char *interface, uint32_t version)
{
    printf("(%d) Got a registry event for <%s> id <%d>\n",
        version, interface, id);
    if (strcmp(interface, "wl_compositor") == 0) {
        fprintf(stderr, "Interface is <wl_compositor>.\n");
        compositor = wl_registry_bind(
            registry,
            id,
            &wl_compositor_interface,
            version
        );
    } else if (strcmp(interface, "zxdg_shell_v6") == 0) {
        /*
        shell = wl_registry_bind(
            registry, id, &zxdg_shell_v6_interface, 1);
        */
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
    } else if (strcmp(interface, "wl_shm") == 0) {
        shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    } else if (strcmp(interface, "wl_output") == 0) {
        output = wl_registry_bind(registry, id, &wl_output_interface, 3);
        fprintf(stderr, "output: %p\n", output);
        wl_output_add_listener(output, &wl_output_listener, NULL);
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

    if (wm_base == NULL) {
        fprintf(stderr, "Haven't got a Wayland wm_base.\n");
        exit(1);
    }

    // Check shell surface.
    fprintf(stderr, " - Checking xdg surface...\n");
//    shell_surface = wl_shell_get_shell_surface(shell, surface);
    xdg_surface = xdg_wm_base_get_xdg_surface(wm_base, surface);
    if (xdg_surface == NULL) {
        fprintf(stderr, "Can't create xdg surface.\n");
        exit(1);
    } else {
        fprintf(stderr, "Created xdg surface!\n");
    }
//    wl_shell_surface_set_toplevel(xdg_surface);
    struct xdg_toplevel *xdg_toplevel =
        xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);

    // Signal that the surface is ready to be configured.
    wl_surface_commit(surface);

    // Wait for the surface to be configured.
    wl_display_roundtrip(display);

    while (1) {
        wl_display_dispatch(display);
    }

    wl_display_disconnect(display);
    printf("Disconnected from display.\n");

    return 0;
}
