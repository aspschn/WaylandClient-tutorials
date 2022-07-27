#include <example/surface.h>

// C
#include <stdio.h>

#include <example/application.h>

//==========
// XDG
//==========
static void xdg_surface_configure_handler(void *data,
        struct xdg_surface *xdg_surface, uint32_t serial)
{
    (void)data;
    fprintf(stderr, "xdg_surface_configure_handler()\n");
    xdg_surface_ack_configure(xdg_surface, serial);
}

const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler,
};

static void xdg_toplevel_configure_handler(void *data,
        struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height,
        struct wl_array *states)
{
    (void)data;
    (void)xdg_toplevel;
    (void)width;
    (void)height;
    (void)states;
    void *p_state;
    enum xdg_toplevel_state state;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
    wl_array_for_each(p_state, states) {
        state = *((enum xdg_toplevel_state*)(p_state));
        //
    }
#pragma GCC diagnostic pop
}

static void xdg_toplevel_close_handler(void *data,
        struct xdg_toplevel *xdg_toplevel)
{
    (void)data;
    (void)xdg_toplevel;
}

const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler,
};

//=============
// Surface
//=============
Surface::Surface(Surface::Type type, uint32_t width, uint32_t height)
{
    this->_type = type;

    this->_width = width;
    this->_height = height;
    this->_scale = 2;

    this->_wl_surface = nullptr;
    this->_xdg_surface = nullptr;
    this->_xdg_toplevel = nullptr;

    this->keyboard_state.rate = app->keyboard_rate();
    this->keyboard_state.delay = app->keyboard_delay();

    this->_context = nullptr;

    // Wayland.
    this->_wl_surface = wl_compositor_create_surface(app->wl_compositor());
    if (this->_type == Surface::Type::Toplevel) {
        this->_xdg_surface = xdg_wm_base_get_xdg_surface(app->xdg_wm_base(),
            this->_wl_surface);
        xdg_surface_add_listener(this->_xdg_surface, &xdg_surface_listener,
            nullptr);

        this->_xdg_toplevel = xdg_surface_get_toplevel(this->_xdg_surface);
        xdg_toplevel_add_listener(this->_xdg_toplevel, &xdg_toplevel_listener,
            nullptr);
    }
    wl_surface_commit(this->_wl_surface);

    // GL context.
    auto egl_display = eglGetDisplay((EGLNativeDisplayType)app->wl_display());;
    this->_context = new gl::Context(egl_display);

    // EGL init.
    this->_wl_egl_window = wl_egl_window_create(this->_wl_surface,
        this->scaled_width(), this->scaled_height());
    if (this->_wl_egl_window == EGL_NO_SURFACE) {
        fprintf(stderr, "Can't create egl window.\n");
        return;
    } else {
        fprintf(stderr, "Created egl window.\n");
    }

    this->_egl_surface = eglCreateWindowSurface(egl_display,
        this->_context->egl_config(),
        (EGLNativeWindowType)this->_wl_egl_window,
        NULL);
    EGLBoolean made = eglMakeCurrent(egl_display,
        this->_egl_surface, this->_egl_surface, this->_context->egl_context());
    if (made) {
        fprintf(stderr, "Made current.\n");
    } else {
        fprintf(stderr, "Made current failed.\n");
    }
}

uint32_t Surface::width() const
{
    return this->_width;
}

uint32_t Surface::height() const
{
    return this->_height;
}

uint32_t Surface::scale() const
{
    return this->_scale;
}

void Surface::set_size(uint32_t width, uint32_t height)
{
    this->_width = width;
    this->_height = height;
}

uint32_t Surface::scaled_width() const
{
    return this->_width * this->_scale;
}

uint32_t Surface::scaled_height() const
{
    return this->_height * this->_scale;
}

struct wl_surface* Surface::wl_surface()
{
    return this->_wl_surface;
}

struct xdg_surface* Surface::xdg_surface()
{
    return this->_xdg_surface;
}

struct xdg_toplevel* Surface::xdg_toplevel()
{
    return this->_xdg_toplevel;
}

void Surface::swap_buffers()
{
    eglSwapBuffers(this->_context->egl_context(), this->_egl_surface);
}
