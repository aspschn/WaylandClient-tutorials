#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "xdg-shell.h"

#include <cairo/cairo.h>
#include <cairo/cairo-gl.h>
#include <pango/pangocairo.h>

#include "utils.h"

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct xdg_wm_base *xdg_wm_base = NULL;
struct xdg_surface *xdg_surface;
struct xdg_toplevel *xdg_toplevel;
struct wl_egl_window *egl_window;
struct wl_region *region;

EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;

cairo_surface_t *cairo_surface;
cairo_device_t *cairo_device;

const char* egl_error_string(int err)
{
    switch (err) {
    case EGL_SUCCESS:
        return "EGL_SUCCESS";
    case EGL_NOT_INITIALIZED:
        return "EGL_NOT_INITIALIZED";
    case EGL_BAD_ACCESS:
        return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC:
        return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE:
        return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONTEXT:
        return "EGL_BAD_CONTEXT";
    case EGL_BAD_CONFIG:
        return "EGL_BAD_CONFIG";
    case EGL_BAD_CURRENT_SURFACE:
        return "EGL_BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY:
        return "EGL_BAD_DISPLAY";
    case EGL_BAD_SURFACE:
        return "EGL_BAD_SURFACE";
    case EGL_BAD_MATCH:
        return "EGL_BAD_MATCH";
    case EGL_BAD_PARAMETER:
        return "EGL_BAD_PARAMETER";
    case EGL_BAD_NATIVE_PIXMAP:
        return "EGL_BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW:
        return "EGL_BAD_NATIVE_WINDOW";
    case EGL_CONTEXT_LOST:
        return "EGL_CONTEXT_LOST";
    default:
        return "UNKNOWN ERROR!!";
    }
}

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
    fprintf(stderr, " = xdg_surface_configure_handler(). serial: %d\n",
        serial);
    xdg_surface_ack_configure(xdg_surface, serial);
}

const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler,
};

void xdg_wm_base_ping_handler(void *data, struct xdg_wm_base *xdg_wm_base,
        uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
    printf("Pong!\n");
}

const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping_handler,
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
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        fprintf(stderr, "Interface is <xdg_wm_base>.\n");
        xdg_wm_base = wl_registry_bind(
            registry, id, &xdg_wm_base_interface, 1);
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
static void draw_text(cairo_t *cr, double x, double y)
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
    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, layout);

    cairo_restore(cr);

    g_object_unref(layout);
}

static void init_cairo()
{
    cairo_device = cairo_egl_device_create(egl_display, egl_context);
    if (cairo_device_status(cairo_device) != CAIRO_STATUS_SUCCESS) {
        exit(1);
    }
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

    // Cairo
//    cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 480, 360);
    cairo_surface = cairo_gl_surface_create_for_egl(cairo_device, egl_surface,
        480, 360);
    cairo_t *cr = cairo_create(cairo_surface);
    int err = cairo_status(cr);
    if (err != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "Cairo error on create %s\n",
            cairo_status_to_string(err));
    }

    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    cairo_paint(cr);
    draw_text(cr, 10, 10);
    draw_text(cr, 50, 50);

    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
        fprintf(stderr, "Made current.\n");
    } else {
        fprintf(stderr, "Made current failed!\n");
    }

    cairo_gl_surface_swapbuffers(cairo_surface);

//    if (eglSwapBuffers(egl_display, egl_surface)) {
//        fprintf(stderr, "Swapped buffers.\n");
//    } else {
//        fprintf(stderr, "Swapped buffers failed!\n");
//    }
}

static void init_egl()
{
    EGLint major, minor, count ,n, size;
    EGLConfig *configs;
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE,
    };

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
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
    display = wl_display_connect(NULL);
    if (display != NULL) {
        fprintf(stderr, "Connected to display!\n");
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    surface = wl_compositor_create_surface(compositor);

    xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);

    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);

    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);

    wl_surface_commit(surface);

    // Wait for the surface to be configured.
    wl_display_roundtrip(display);

    create_opaque_region();
    init_egl();
    init_cairo();
    create_window();

    wl_surface_commit(surface);

    while (wl_display_dispatch(display) != -1) {
        ;
    }

    wl_display_disconnect(display);

    return 0;
}
