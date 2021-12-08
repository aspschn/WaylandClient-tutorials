#include "application.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <linux/input.h>

#include "window.h"

//==============
// Seat
//==============

// Keyboard
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

// Pointer
static void pointer_enter_handler(void *data, struct wl_pointer *pointer,
        uint32_t serial, struct wl_surface *surface,
        wl_fixed_t sx, wl_fixed_t sy)
{
    fprintf(stderr, "Pointer entered surface\t%p at %d %d\n", surface, sx, sy);

    struct wl_buffer *buffer;
    struct wl_cursor_image *image;

    /*
    image = default_cursor->images[0];
    buffer = wl_cursor_image_get_buffer(image);
    wl_pointer_set_cursor(pointer, serial, cursor_surface,
        image->hotspot_x, image->hotspot_y);
    wl_surface_attach(cursor_surface, buffer, 0, 0);
    wl_surface_damage(cursor_surface, 0, 0, image->width, image->height);
    wl_surface_commit(cursor_surface);

    wl_surface_attach(surface2, buffer, 0, 0);
    wl_surface_commit(surface2);
    */
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
//    if (sx > 49152 && sy < 2816) {
//        zxdg_surface_v6_destroy(xdg_surface);
//    }
}

static void pointer_button_handler(void *data, struct wl_pointer *wl_pointer,
        uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
    fprintf(stderr, "Pointer button\n");
    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED) {
        /*
        fprintf(stderr, "Move! wl_pointer: %p, xdg_toplevel: %p\n",
            wl_pointer, xdg_toplevel);
        zxdg_toplevel_v6_move(xdg_toplevel, seat, serial);
        */
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

// Capabilities
static void seat_handle_capabilities(void *data, struct wl_seat *seat,
        enum wl_seat_capability caps)
{
    bl_application *application = (bl_application*)data;

    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        printf("Display has a pointer.\n");
    }
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        printf("Display has a keyboard.\n");
        application->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(application->keyboard,
            &keyboard_listener, NULL);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        fprintf(stderr, "Destroy keyboard.\n");
        wl_keyboard_destroy(application->keyboard);
        application->keyboard = NULL;
    }
    if (caps & WL_SEAT_CAPABILITY_TOUCH) {
        printf("Display has a touch screen.\n");
    }
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !(application->pointer)) {
        application->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(application->pointer,
            &pointer_listener, NULL);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && application->pointer) {
        wl_pointer_destroy(application->pointer);
        application->pointer = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
};

//==============
// Global
//==============
static void global_registry_handler(void *data, struct wl_registry *registry,
        uint32_t id, const char *interface, uint32_t version)
{
    bl_application *application = (bl_application*)data;
    (void)version;

    if (strcmp(interface, "wl_seat") == 0) {
        if (application->seat == NULL) {
            application->seat = wl_registry_bind(registry,
                id, &wl_seat_interface, 1);
            wl_seat_add_listener(application->seat,
                &seat_listener, (void*)application);
        }
    } else if (strcmp(interface, "wl_compositor") == 0) {
        if (application->compositor == NULL) {
            application->compositor = wl_registry_bind(registry,
                id, &wl_compositor_interface, 1);
        }
    } else if (strcmp(interface, "zxdg_shell_v6") == 0) {
        if (application->toplevel_windows_length >= 1) {
            uint32_t len = application->toplevel_windows_length;
            for (uint32_t i = 0; i < len; ++i) {
                bl_window *window = application->toplevel_windows[i];
                if (window->xdg_shell != NULL) {
                    continue;
                }
                window->xdg_shell = wl_registry_bind(
                    registry,
                    id,
                    &zxdg_shell_v6_interface,
                    1
                );
            }
        }
    } else if (strcmp(interface, "wl_shm") == 0) {
        /*
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
        */
    } else if (strcmp(interface, "wl_subcompositor") == 0) {
        if (application->subcompositor == NULL) {
            application->subcompositor = wl_registry_bind(registry,
                id, &wl_subcompositor_interface, 1);
        }
    } else {
        fprintf(stderr, "Interface <%s>\n", interface);
    }
}

static void global_registry_remover(void *data, struct wl_registry *registry,
        uint32_t id)
{
    (void)data;
    (void)registry;
    (void)id;
    fprintf(stderr, "global_registry_remover()\n");
}

static const struct wl_registry_listener registry_listener = {
    .global = global_registry_handler,
    .global_remove = global_registry_remover,
};

//=================
// Application
//=================

bl_application *bl_app = NULL;

bl_application* bl_application_new()
{
    if (bl_app != NULL) {
        fprintf(stderr, "Blusher app already created.\n");
        return NULL;
    }
    bl_application *application = malloc(sizeof(bl_application));

    application->display = wl_display_connect(NULL);
    if (application->display == NULL) {
        bl_application_free(application);

        return NULL;
    }

    application->compositor = NULL;
    application->subcompositor = NULL;
    application->registry = NULL;

    application->seat = NULL;
    application->keyboard = NULL;
    application->pointer = NULL;

    application->registry = wl_display_get_registry(application->display);
    wl_registry_add_listener(application->registry,
        &registry_listener, (void*)application);

    wl_display_dispatch(application->display);
    wl_display_roundtrip(application->display);

    application->toplevel_windows =
        (bl_window**)malloc(sizeof(bl_window*) * 1); // Currently support only
                                                      // one window.
    application->toplevel_windows_length = 0;

    // Set singleton.
    bl_app = application;

    return application;
}

void bl_application_add_window(bl_application *application, bl_window *window)
{
    application->toplevel_windows[0] = window;
    application->toplevel_windows_length = 1;

    // Registry.
    if (application->registry != NULL) {
        wl_registry_destroy(application->registry);
    }
    application->registry = wl_display_get_registry(application->display);
    wl_registry_add_listener(application->registry,
        &registry_listener, (void*)application);

    wl_display_dispatch(application->display);
}

int bl_application_exec(bl_application *application)
{
    if (application->compositor == NULL) {
        fprintf(stderr, "Can't find compositor.\n");
        return 1;
    }
    if (application->subcompositor == NULL) {
        fprintf(stderr, "Can't find subcompositor.\n");
        return 1;
    }

    while (wl_display_dispatch(application->display) != -1) {
        ;
    }

    return 0;
}

void bl_application_free(bl_application *application)
{
    free(application);
    application = NULL;
}
