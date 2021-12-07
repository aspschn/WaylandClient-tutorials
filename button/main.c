#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <linux/input.h>

#include "xdg-shell.h"

#include <cairo/cairo.h>
#include <cairo/cairo-gl.h>
#include <pango/pangocairo.h>

#include "utils.h"

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct zxdg_shell_v6 *xdg_shell = NULL;
struct zxdg_surface_v6 *xdg_surface;
struct zxdg_toplevel_v6 *xdg_toplevel;
struct wl_egl_window *egl_window;
//struct wl_region *region;
struct wl_egl_window *button;
struct wl_surface *button_surface;
struct wl_region *button_region;

EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;

cairo_surface_t *cairo_surface;
cairo_device_t *cairo_device;

// Input devices
struct wl_seat *seat = NULL;
struct wl_pointer *pointer;
struct wl_keyboard *keyboard;

// Cursor
struct wl_shm *shm;
struct wl_cursor_theme *cursor_theme;
struct wl_cursor *default_cursor;
struct wl_surface *cursor_surface;

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

//============
// Keyboard
//============
static void keyboard_keymap_handler(void *data, struct wl_keyboard *keyboard,
        uint32_t format, int fd, uint32_t size)
{
}

static void keyboard_enter_handler(void *data, struct wl_keyboard *keyboard,
        uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
    fprintf(stderr, "Keyboard gained focus\n");
}

static void keyboard_leave_handler(void *data, struct wl_keyboard *keyboard,
        uint32_t serial, struct wl_surface *surface)
{
    fprintf(stderr, "Keyboard lost focus\n");
}

static void keyboard_key_handler(void *data, struct wl_keyboard *keyboard,
        uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    fprintf(stderr, "Key is %d, state is %d\n", key, state);
}

static void keyboard_modifiers_handler(void *data, struct wl_keyboard *keyboard,
        uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
        uint32_t mods_locked, uint32_t group)
{
    fprintf(stderr, "Modifiers depressed %d, latched %d, locked %d, group %d\n",
        mods_depressed, mods_latched, mods_locked, group);
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap_handler,
    .enter = keyboard_enter_handler,
    .leave = keyboard_leave_handler,
    .key = keyboard_key_handler,
    .modifiers = keyboard_modifiers_handler,
};

//==============
// Seat
//==============
static void pointer_enter_handler(void *data, struct wl_pointer *pointer,
        uint32_t serial, struct wl_surface *surface,
        wl_fixed_t sx, wl_fixed_t sy)
{
    fprintf(stderr, "Pointer entered surface %p at %d %d\n", surface, sx, sy);

    struct wl_buffer *buffer;
    struct wl_cursor_image *image;

    image = default_cursor->images[0];
    buffer = wl_cursor_image_get_buffer(image);
    wl_pointer_set_cursor(pointer, serial, cursor_surface,
        image->hotspot_x, image->hotspot_y);
    wl_surface_attach(cursor_surface, buffer, 0, 0);
    wl_surface_damage(cursor_surface, 0, 0, image->width, image->height);
    wl_surface_commit(cursor_surface);
}

static void pointer_leave_handler(void *data, struct wl_pointer *pointer,
        uint32_t serial, struct wl_surface *surface)
{
    fprintf(stderr, "Pointer left surface %p\n", surface);
}

static void pointer_motion_handler(void *data, struct wl_pointer *pointer,
        uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
//    fprintf(stderr, "Pointer moved at %d %d\n", sx, sy);
}

static void pointer_button_handler(void *data, struct wl_pointer *wl_pointer,
        uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    fprintf(stderr, "Pointer button\n");
    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED) {
        fprintf(stderr, "Move! wl_pointer: %p, xdg_toplevel: %p\n",
            wl_pointer, xdg_toplevel);
        zxdg_toplevel_v6_move(xdg_toplevel, seat, serial);
    }
}

static void pointer_axis_handler(void *data, struct wl_pointer *wl_pointer,
        uint32_t time, uint32_t axis, wl_fixed_t value)
{
    fprintf(stderr, "Pointer handle axis\n");
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter_handler,
    .leave = pointer_leave_handler,
    .motion = pointer_motion_handler,
    .button = pointer_button_handler,
    .axis = pointer_axis_handler,
};

static void seat_handle_capabilities(void *data, struct wl_seat *seat,
        enum wl_seat_capability caps)
{
    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        printf("Display has a pointer.\n");
    }
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        printf("Display has a keyboard.\n");
        keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(keyboard, &keyboard_listener, NULL);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        fprintf(stderr, "Destroy keyboard.\n");
        wl_keyboard_destroy(keyboard);
        keyboard = NULL;
    }
    if (caps & WL_SEAT_CAPABILITY_TOUCH) {
        printf("Display has a touch screen.\n");
    }

    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !pointer) {
        pointer  =wl_seat_get_pointer(seat);
        wl_pointer_add_listener(pointer, &pointer_listener, NULL);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && pointer) {
        wl_pointer_destroy(pointer);
        pointer = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
};

//==============
// EGL
//==============
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

    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
        fprintf(stderr, "Made current.\n");
    } else {
        fprintf(stderr, "Made current failed!\n");
    }

    cairo_gl_surface_swapbuffers(cairo_surface);

    cairo_surface_destroy(cairo_surface);
    cairo_surface = NULL;
}

static void create_button()
{
    /*
    button = wl_egl_window_create(surface, 100, 30);
    if (button == EGL_NO_SURFACE) {
        exit(1);
    }

    egl_surface =
//        eglCreatePbufferSurface(egl_display, egl_conf, NULL);
        eglCreateWindowSurface(egl_display, egl_conf, button, NULL);

    cairo_surface = cairo_gl_surface_create_for_egl(cairo_device, egl_surface,
        100, 30);
    cairo_t *cr = cairo_create(cairo_surface);
    cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
    cairo_paint(cr);
    draw_text(cr, 0, 0);

    cairo_gl_surface_swapbuffers(cairo_surface);
    */
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
    } else if (strcmp(interface, "wl_seat") == 0) {
        seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    } else if (strcmp(interface, "wl_shm") == 0) {
        shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
        cursor_theme = wl_cursor_theme_load(NULL, 32, shm);
        if (cursor_theme == NULL) {
            fprintf(stderr, "Can't get cursor theme.\n");
        }
        default_cursor = wl_cursor_theme_get_cursor(cursor_theme, "left_ptr");
        if (default_cursor == NULL) {
            fprintf(stderr, "Can't get default cursor.\n");
            exit(1);
        }
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
    display = wl_display_connect(NULL);

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    surface = wl_compositor_create_surface(compositor);

    zxdg_shell_v6_add_listener(xdg_shell, &xdg_shell_listener, NULL);

    xdg_surface = zxdg_shell_v6_get_xdg_surface(xdg_shell, surface);
    zxdg_surface_v6_add_listener(xdg_surface, &xdg_surface_listener, NULL);

    xdg_toplevel = zxdg_surface_v6_get_toplevel(xdg_surface);
    zxdg_toplevel_v6_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);

    wl_surface_commit(surface);

    // Button surface
    struct wl_buffer *button_buffer;

    button_surface = wl_compositor_create_surface(compositor);
    button_region = wl_compositor_create_region(compositor);
    wl_surface_set_input_region(button_surface, button_region);
    wl_region_add(button_region, 0, 0, 100, 100);
    wl_surface_commit(button_surface);

    // Wait for the surface to be configured.
    wl_display_roundtrip(display);

    init_egl();
    init_cairo();
    create_window();
    create_button();

    cursor_surface = wl_compositor_create_surface(compositor);

    wl_surface_commit(surface);

    while (wl_display_dispatch(display) != -1) {
        ;
    }

    wl_display_disconnect(display);

    return 0;
}
