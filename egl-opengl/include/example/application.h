#ifndef _APPLICATION_H
#define _APPLICATION_H

// C
#include <stdint.h>

// Wayland core
#include <wayland-client.h>

// Wayland protocols
#include <wayland-protocols/stable/xdg-shell.h>

class Application
{
public:
    Application(int argc, char *argv[]);

private:
    struct wl_display *_wl_display;
    struct wl_compositor *_wl_compositor;
    struct wl_subcompositor *_wl_subcompositor;
    struct wl_seat *_wl_seat;
    struct wl_keyboard *_wl_keyboard;
    struct wl_pointer *_wl_pointer;
    struct xdg_wm_base *_xdg_wm_base;
};

// Singleton object.
extern Application *app;

#endif /* _APPLICATION_H */
