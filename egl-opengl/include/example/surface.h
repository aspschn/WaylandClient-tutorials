#ifndef _SURFACE_H
#define _SURFACE_H

// C
#include <stdint.h>

// EGL
#include <EGL/egl.h>

// Wayland core
#include <wayland-client.h>
#include <wayland-egl.h>

// Wayland protocols
#include <wayland-protocols/stable/xdg-shell.h>

#include <example/gl/context.h>
#include <example/keyboard-state.h>

class Surface
{
public:
    enum class Type {
        Normal,
        Subsurface,
        Toplevel,
    };

public:
    Surface(Surface::Type type, uint32_t width, uint32_t height);

    uint32_t width() const;
    uint32_t height() const;
    uint32_t scale() const;

    void set_size(uint32_t width, uint32_t height);

    uint32_t scaled_width() const;
    uint32_t scaled_height() const;

    struct wl_surface* wl_surface();
    struct xdg_surface* xdg_surface();
    struct xdg_toplevel* xdg_toplevel();

    void swap_buffers();

    KeyboardState keyboard_state;

private:
    Surface::Type _type;

    uint32_t _width;
    uint32_t _height;
    uint32_t _scale;

    //============
    // Wayland
    //============
    struct wl_surface *_wl_surface;
    struct xdg_surface *_xdg_surface;
    struct xdg_toplevel *_xdg_toplevel;

    EGLSurface _egl_surface;
    struct wl_egl_window *_wl_egl_window;

    gl::Context *_context;
};

#endif /* _SURFACE_H */
