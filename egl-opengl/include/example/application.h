#ifndef _APPLICATION_H
#define _APPLICATION_H

// C
#include <stdint.h>

// C++
#include <vector>

// Wayland core
#include <wayland-client.h>

// Wayland protocols
#include <wayland-protocols/stable/xdg-shell.h>

class Surface;

class Application
{
public:
    Application(int argc, char *argv[]);

    struct wl_display* wl_display();
    void set_wl_display(struct wl_display *display);

    struct wl_compositor* wl_compositor();
    void set_wl_compositor(struct wl_compositor *compositor);

    struct wl_subcompositor* wl_subcompositor();
    void set_wl_subcompositor(struct wl_subcompositor *subcompositor);

    struct wl_seat* wl_seat();
    void set_wl_seat(struct wl_seat *seat);

    struct wl_keyboard* wl_keyboard();
    void set_wl_keyboard(struct wl_keyboard *keyboard);

    struct wl_pointer* wl_pointer();
    void set_wl_pointer(struct wl_pointer *pointer);

    struct xdg_wm_base* xdg_wm_base();
    void set_xdg_wm_base(struct xdg_wm_base *wm_base);

    uint32_t keyboard_rate() const;
    uint32_t keyboard_delay() const;
    void set_keyboard_rate(uint32_t rate);
    void set_keyboard_delay(uint32_t delay);

    void add_surface(Surface *surface);
    void remove_surface(Surface *surface);
    std::vector<Surface*> surface_list() const;

private:
    struct wl_display *_wl_display;
    struct wl_compositor *_wl_compositor;
    struct wl_subcompositor *_wl_subcompositor;
    struct wl_seat *_wl_seat;
    struct wl_keyboard *_wl_keyboard;
    struct wl_pointer *_wl_pointer;
    struct xdg_wm_base *_xdg_wm_base;

    uint32_t _keyboard_rate;
    uint32_t _keyboard_delay;

    std::vector<Surface*> _surface_list;
};

// Singleton object.
extern Application *app;

#endif /* _APPLICATION_H */
