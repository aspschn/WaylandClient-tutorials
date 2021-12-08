#include "application.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "window.h"

//==============
// Global
//==============
static void global_registry_handler(void *data, struct wl_registry *registry,
        uint32_t id, const char *interface, uint32_t version)
{
    bl_application *application = (bl_application*)data;
    (void)version;

    if (strcmp(interface, "wl_seat") == 0) {
        /*
        seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
        */
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

    return 0;
}

void bl_application_free(bl_application *application)
{
    free(application);
    application = NULL;
}
