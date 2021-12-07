#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <unistd.h>

#include "xdg-shell.h"

#include "utils.h"

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_egl_window *egl_window;
struct wl_region *region;
struct wl_callback *callback;
struct wl_callback *frame_callback;
struct zxdg_shell_v6 *xdg_shell;
struct zxdg_surface_v6 *xdg_surface;
struct zxdg_toplevel_v6 *xdg_toplevel;

struct wl_surface *surface2;
struct wl_buffer *buffer;

void *shm_data;

// Input devices
struct wl_seat *seat = NULL;
struct wl_pointer *pointer;
struct wl_keyboard *keyboard;

// Cursor
struct wl_shm *shm;
struct wl_cursor_theme *cursor_theme;
struct wl_cursor *default_cursor;
struct wl_surface *cursor_surface;

// EGL
EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;

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
// Pointer
//==============
static void pointer_enter_handler(void *data, struct wl_pointer *pointer,
        uint32_t serial, struct wl_surface *surface,
        wl_fixed_t sx, wl_fixed_t sy)
{
    fprintf(stderr, "Pointer entered surface\t%p at %d %d\n", surface, sx, sy);

    struct wl_buffer *buffer;
    struct wl_cursor_image *image;

    image = default_cursor->images[0];
    buffer = wl_cursor_image_get_buffer(image);
    wl_pointer_set_cursor(pointer, serial, cursor_surface,
        image->hotspot_x, image->hotspot_y);
    wl_surface_attach(cursor_surface, buffer, 0, 0);
    wl_surface_damage(cursor_surface, 0, 0, image->width, image->height);
    wl_surface_commit(cursor_surface);

    wl_surface_attach(surface2, buffer, 0, 0);
    wl_surface_commit(surface2);
}

static void pointer_leave_handler(void *data, struct wl_pointer *pointer,
        uint32_t serial, struct wl_surface *surface)
{
    fprintf(stderr, "Pointer left surface\t%p\n", surface);
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

//============
// EGL
//============
static void paint_pixels();
static void redraw();

static const struct wl_callback_listener frame_listener = {
    redraw,
};

static void redraw(void *data, struct wl_callback *callback, uint32_t time)
{
    printf("Redrawing\n");
    wl_callback_destroy(frame_callback);
    wl_surface_damage(surface, 0, 0, 480, 360);
    paint_pixels();
    frame_callback = wl_surface_frame(surface);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_callback_add_listener(frame_callback, &frame_listener, NULL);
    wl_surface_commit(surface);
}

static void configure_callback(void *data, struct wl_callback *callback,
        uint32_t time)
{
    fprintf(stderr, "configure_callback()\n");
    if (callback == NULL) {
        redraw(data, NULL, time);
    }
}

static struct wl_callback_listener configure_callback_listener = {
    configure_callback,
};

static struct wl_buffer* create_buffer();

static void create_window()
{
    /*
    egl_window = wl_egl_window_create(surface, 480, 360);
    if (egl_window == EGL_NO_SURFACE) {
        fprintf(stderr, "Can't create egl window\n");
        exit(1);
    }

    egl_surface = eglCreateWindowSurface(egl_display, egl_conf, egl_window,
        NULL);

    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
        fprintf(stderr, "Made current\n");
    }

    glClearColor(1.0, 1.0, 0.0, 0.1);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();

    if (eglSwapBuffers(egl_display, egl_surface)) {
        fprintf(stderr, "Swapped buffers\n");
    } else {
        fprintf(stderr, "Swapped buffers failed\n");
        exit(1);
    }

    wl_display_dispatch(display);
    wl_display_roundtrip(display);
    */
    buffer = create_buffer();

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);
}

static void change_color()
{
    fprintf(stderr, "on change_color()\n");

    glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();

    eglSwapBuffers(egl_display, egl_surface);
}

static void init_egl()
{
    EGLint major, minor, count, n, size;
    EGLConfig *configs;
    EGLBoolean ret;
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
    printf("EGL major: %d, minor: %d\n", major, minor);

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        fprintf(stderr, "Can't bind API\n");
        exit(1);
    }

    eglGetConfigs(egl_display, NULL, 0, &count);
    printf("EGL has %d configs\n", count);

    configs = calloc(count, sizeof *configs);

    ret = eglChooseConfig(egl_display, config_attribs, configs, count, &n);
    fprintf(stderr, "eglChooseConfig ret: %d\n", ret);

    for (int i = 0; i < n; ++i) {
        eglGetConfigAttrib(egl_display, configs[i], EGL_BUFFER_SIZE, &size);
        printf("Buffer size for config %d is %d\n", i, size);
        eglGetConfigAttrib(egl_display, configs[i], EGL_RED_SIZE, &size);
        printf("Red size for config %d is %d\n", i, size);

        egl_conf = configs[i];
        break;
    }

    egl_context = eglCreateContext(egl_display, egl_conf, EGL_NO_CONTEXT,
        context_attribs);
}

//==============
// Xdg
//==============

// Xdg shell
static void ping_handler(void *data,
        struct zxdg_shell_v6 *xdg_shell, uint32_t serial)
{
    zxdg_shell_v6_pong(xdg_shell, serial);
    fprintf(stderr, "Ping pong.\n");
}

const struct zxdg_shell_v6_listener xdg_shell_listener = {
    .ping = ping_handler,
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
    printf("TOPLEVEL Configure: %dx%d\n", width, height);
}

static void xdg_toplevel_close_handler(void *data,
        struct zxdg_toplevel_v6 *xdg_toplevel)
{
    printf("TOPLEVEL Close %p\n", xdg_toplevel);
}

static const struct zxdg_toplevel_v6_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler,
};

//==============
// Paint
//==============
uint32_t pixel_value= 0x000000;

static void paint_pixels()
{
    uint32_t *pixel = shm_data;

    for (int n = 0; n < 480 * 360; ++n) {
        *pixel++ = pixel_value;
    }

    pixel_value += 0x010101;

    if (pixel_value > 0xffffff) {
        pixel_value = 0x0;
    }
}

static struct wl_buffer* create_buffer()
{
    struct wl_shm_pool *pool;
    int stride = 480 * 4;
    int size = stride * 360;
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
    buff = wl_shm_pool_create_buffer(pool, 0, 480, 360, stride,
        WL_SHM_FORMAT_XRGB8888);

    wl_shm_pool_destroy(pool);
    return buff;
}

//==============
// Global
//==============
static void global_registry_handler(void *data, struct wl_registry *registry,
        uint32_t id, const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_seat") == 0) {
        seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    } else if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(registry, id, &wl_compositor_interface,
            1);
    } else if (strcmp(interface, "zxdg_shell_v6") == 0) {
        xdg_shell = wl_registry_bind(registry, id, &zxdg_shell_v6_interface, 1);
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

//===========
// Main
//===========
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

    if (compositor == NULL || xdg_shell == NULL) {
        fprintf(stderr, "Can't find compositor or xdg_shell.\n");
        exit(1);
    }

    // Check surface.
    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
        exit(1);
    } else {
        fprintf(stderr, " = surface:     %p\n", surface);
    }

    zxdg_shell_v6_add_listener(xdg_shell, &xdg_shell_listener, NULL);

    // Check xdg surface.
    xdg_surface = zxdg_shell_v6_get_xdg_surface(xdg_shell, surface);
    if (xdg_surface == NULL) {
        exit(1);
    } else {
        fprintf(stderr, " = xdg_surface: %p\n", xdg_surface);
    }

    zxdg_surface_v6_add_listener(xdg_surface, &xdg_surface_listener, NULL);

    xdg_toplevel =
        zxdg_surface_v6_get_toplevel(xdg_surface);
    if (xdg_toplevel == NULL) {
        exit(1);
    }
    zxdg_toplevel_v6_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);

    wl_surface_commit(surface);

    // Wait for the surface to be configured.
    wl_display_roundtrip(display);

    callback = wl_display_sync(display);
    wl_callback_add_listener(callback, &configure_callback_listener, NULL);
    frame_callback = wl_surface_frame(surface);
    wl_callback_add_listener(frame_callback, &frame_listener, NULL);

    init_egl();
    create_window();
    paint_pixels();
    redraw(NULL, NULL, 0);

    cursor_surface = wl_compositor_create_surface(compositor);


    surface2 = wl_compositor_create_surface(compositor);
    if (surface2 == NULL) {
        exit(1);
    } else {
        fprintf(stderr, "surface2 created: %p\n", surface2);
    }


    wl_surface_commit(surface);

//    change_color();

    while (wl_display_dispatch(display) != -1) {
        ;
    }

    wl_display_disconnect(display);
    printf("Disconnected from display.\n");

    return 0;
}
