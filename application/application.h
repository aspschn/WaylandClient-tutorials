#ifndef _BLUSHER_APPLICATION_H
#define _BLUSHER_APPLICATION_H

#include <stdint.h>

#include <wayland-client.h>

typedef struct bl_window bl_window;
typedef struct bl_ptr_btree bl_ptr_btree;

typedef struct bl_application {
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct wl_subcompositor *subcompositor;
    struct wl_registry *registry;
    struct wl_shm *shm;

    struct wl_seat *seat;
    struct wl_keyboard *keyboard;
    struct wl_pointer *pointer;

    bl_window **toplevel_windows;
    uint32_t toplevel_windows_length;

    /// \brief Structure that map wl_surface to bl_surface.
    bl_ptr_btree *surface_map;
    struct wl_surface *pointer_surface;
    /// \brief Store surface local x because press event not have x.
    int32_t pointer_x;
    /// \brief Store surface local y because press event not have y.
    int32_t pointer_y;
    /// \brief Store pointer state when pressed or released.
    uint32_t pointer_state;
} bl_application;

extern bl_application *bl_app;  // Singleton object.

bl_application* bl_application_new();

void bl_application_add_window(bl_application *application, bl_window *window);

void bl_application_remove_window(bl_application *application,
        bl_window *window);

int bl_application_exec(bl_application *application);

void bl_application_free(bl_application *application);

#endif /* _BLUSHER_APPLICATION_H */
