#include <example/application.h>

// C
#include <stdio.h>
#include <string.h>

// Wayland core
#include <wayland-client.h>

// Wayland protocols
#include <wayland-protocols/stable/xdg-shell.h>

//=============
// Seat
//=============

static void seat_capabilities_handler(void *data, struct wl_seat *wl_seat,
        uint32_t caps_uint)
{
    (void)data;
    (void)wl_seat;
    enum wl_seat_capability caps = (enum wl_seat_capability)caps_uint;

    fprintf(stderr, "seat_capabilities_handler()\n");

    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        fprintf(stderr, " - Has keyboard!\n");
        auto keyboard = wl_seat_get_keyboard(wl_seat);
        wl_keyboard_add_listener(keyboard, &keyboard_listener, NULL);
        app->set_wl_keyboard(keyboard);
    }
    if (caps & WL_SEAT_CAPABILITY_POINTER) {
        auto pointer = wl_seat_get_pointer(wl_seat);
        wl_pointer_add_listener(pointer, &pointer_listener, NULL);
        app->set_wl_pointer(pointer);
    }
}

static void seat_name_handler(void *data, struct wl_seat *seat,
        const char *name)
{
    (void)data;
    (void)seat;
    (void)name;
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities_handler,
    .name = seat_name_handler,
};

//===========
// XDG
//===========
static void xdg_wm_base_ping_handler(void *data,
        struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    (void)data;
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping_handler,
};

//==============
// Global
//==============
static void global_registry_handler(void *data, struct wl_registry *registry,
        uint32_t id, const char *interface, uint32_t version)
{
    (void)data;

    if (strcmp(interface, "wl_compositor") == 0) {
        fprintf(stderr, "Interface is <wl_compositor>.\n");
        auto compositor = (struct wl_compositor*)wl_registry_bind(
            registry,
            id,
            &wl_compositor_interface,
            version
        );
        app->set_wl_compositor(compositor);
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        auto xdg_wm_base = (struct xdg_wm_base*)wl_registry_bind(registry,
            id, &xdg_wm_base_interface, 1);
        app->set_xdg_wm_base(xdg_wm_base);
    } else if (strcmp(interface, "wl_subcompositor") == 0) {
        auto subcompositor = (struct wl_subcompositor*)wl_registry_bind(
            registry,
            id,
            &wl_subcompositor_interface,
            1
        );
        app->set_wl_subcompositor(subcompositor);
    } else if (strcmp(interface, "wl_seat") == 0) {
        auto seat = (struct wl_seat*)wl_registry_bind(registry,
            id, &wl_seat_interface, 5);
        wl_seat_add_listener(seat, &seat_listener, NULL);
        app->set_wl_seat(seat);
    } else {
        printf("(%d) Got a registry event for <%s> id <%d>\n",
            version, interface, id);
    }
}

static void global_registry_remover(void *data, struct wl_registry *registry,
        uint32_t id)
{
    (void)data;
    (void)registry;
    (void)id;
    printf("Got a registry losing event for <%d>\n", id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};

//================
// Application
//================

Application::Application(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    app = this;
}

struct wl_display* Application::wl_display()
{
    return this->_wl_display;
}

void Application::set_wl_display(struct wl_display *display)
{
    this->_wl_display = display;
}

struct wl_compositor* Application::wl_compositor()
{
    return this->_wl_compositor;
}

void Application::set_wl_compositor(struct wl_compositor *compositor)
{
    this->_wl_compositor = compositor;
}

struct wl_subcompositor* Application::wl_subcompositor()
{
    return this->_wl_subcompositor;
}

void Application::set_wl_subcompositor(struct wl_subcompositor *subcompositor)
{
    this->_wl_subcompositor = subcompositor;
}

struct wl_seat* Application::wl_seat()
{
    return this->_wl_seat;
}

void Application::set_wl_seat(struct wl_seat *seat)
{
    this->_wl_seat = seat;
}

struct wl_keyboard* Application::wl_keyboard()
{
    return this->_wl_keyboard;
}

void Application::set_wl_keyboard(struct wl_keyboard *keyboard)
{
    this->_wl_keyboard = keyboard;
}

struct wl_pointer* Application::wl_pointer()
{
    return this->_wl_pointer;
}

void Application::set_wl_pointer(struct wl_pointer *pointer)
{
    this->_wl_pointer = pointer;
}

struct xdg_wm_base* Application::xdg_wm_base()
{
    return this->_xdg_wm_base;
}

void Application::set_xdg_wm_base(struct xdg_wm_base *wm_base)
{
    this->_xdg_wm_base = wm_base;
}

Application *app = nullptr;
