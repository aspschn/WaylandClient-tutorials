#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "xdg-shell.h"

#include <cairo.h>
#include <pango/pangocairo.h>

#include "utils.h"

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct zxdg_shell_v6 *xdg_shell = NULL;
struct zxdg_surface_v6 *xdg_surface;
struct wl_egl_window *egl_window;
struct wl_region *region;

EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;

//==============
// Xdg
//==============
void xdg_toplevel_configure_handler(void *data,
        struct zxdg_toplevel_v6 *xdg_toplevel, int32_t width, int32_t height,
        struct wl_array *states)
{
    printf("Configure: %dx%d\n", width, height);
}

void xdg_toplevel_close_handler(void *data,
        struct zxdg_toplevel_v6 *xdg_toplevel)
{
    printf("Close.\n");
}

const struct zxdg_toplevel_v6_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler,
};

void xdg_surface_configure_handler(void *data,
        struct zxdg_surface_v6 *xdg_surface, uint32_t serial)
{
    fprintf(stderr, " = xdg_surface_configure_handler(). serial: %d\n",
        serial);
    zxdg_surface_v6_ack_configure(xdg_surface, serial);
}

const struct zxdg_surface_v6_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler,
};

void xdg_shell_ping_handler(void *data, struct zxdg_shell_v6 *xdg_shell,
        uint32_t serial)
{
    zxdg_shell_v6_pong(xdg_shell, serial);
    printf("Pong!\n");
}

const struct zxdg_shell_v6_listener xdg_shell_listener = {
    .ping = xdg_shell_ping_handler,
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
        xdg_shell = wl_registry_bind(
            registry, id, &zxdg_shell_v6_interface, 1);
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

//================
// Cairo / Pango
//================
static void draw_text(cairo_t *cr)
{
    PangoLayout *layout;
    PangoFontDescription *desc;

    layout = pango_cairo_create_layout(cr);

    pango_layout_set_text(layout, "おはよう！", -1);

    desc = pango_font_description_from_string("serif");
    pango_font_description_set_size(desc, pixel_to_pango_size(16));

    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    cairo_save(cr);

    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

    pango_cairo_update_layout(cr, layout);

    // pango_layout_get_size(layout, &width, &height);
    cairo_move_to(cr, 0, 0);
    pango_cairo_show_layout(cr, layout);

    cairo_restore(cr);

    g_object_unref(layout);
}

//==============
// EGL
//==============

static void create_opaque_region()
{
    fprintf(stderr, " = Begin create_opaque_region()\n");

    region = wl_compositor_create_region(compositor);
    wl_region_add(
        region,
        0,
        0,
        480,
        360
    );
    wl_surface_set_opaque_region(surface, region);

    fprintf(stderr, " = End create_opaque_region()\n");
}

static void create_window()
{
    egl_window = wl_egl_window_create(surface, 480, 360);

    if (egl_window == EGL_NO_SURFACE) {
        exit(1);
    }

    egl_surface =
        eglCreateWindowSurface(egl_display, egl_conf, egl_window, NULL);
    if (egl_surface == NULL) {
        fprintf(stderr, "Can't create EGL window surface.\n");
    }

    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
        fprintf(stderr, "Made current.\n");
    } else {
        fprintf(stderr, "Made current failed!\n");
    }

    if (eglSwapBuffers(egl_display, egl_surface)) {
        fprintf(stderr, "Swapped buffers.\n");
    } else {
        fprintf(stderr, "Swapped buffers failed!\n");
    }
}

static void init_egl()
{
    EGLint major, minor, count ,n, size;
    EGLConfig *configs;
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_NONE,
    };

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION,
        2,
        EGL_NONE,
    };

    egl_display = eglGetDisplay((EGLNativeDisplayType)display);
    if (egl_display == EGL_NO_DISPLAY) {
        exit(1);
    }

    if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE) {
        exit(1);
    }
    printf("EGL major: %d, minor: %d.\n", major, minor);

    eglGetConfigs(egl_display, NULL, 0, &count);
    printf("EGL has %d configs.\n", count);

    configs = calloc(count, sizeof *configs);

    eglChooseConfig(egl_display, config_attribs, configs, count, &n);

    for (int i = 0; i < n; ++i) {
        eglGetConfigAttrib(
            egl_display,
            configs[i],
            EGL_BUFFER_SIZE,
            &size
        );
        printf("Buffer size for config %d is %d.\n", i, size);

        // Just choose the first one
        egl_conf = configs[i];
        break;
    }

    egl_context = eglCreateContext(
        egl_display,
        egl_conf,
        EGL_NO_CONTEXT,
        context_attribs
    );
    if (egl_context == NULL) {
        fprintf(stderr, "Failed create EGL context.\n");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    cairo_t *cr;
    cairo_status_t status;
    cairo_surface_t *surface;

    surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
        240, 80);
    cr = cairo_create(surface);

    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.0);
    cairo_paint(cr);
    draw_text(cr);

    cairo_destroy(cr);
    status = cairo_surface_write_to_png(surface, "hello.png");
    cairo_surface_destroy(surface);

    if (status != CAIRO_STATUS_SUCCESS) {
        return 1;
    }

    return 0;
}
