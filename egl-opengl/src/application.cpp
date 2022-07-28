#include <example/application.h>

// C
#include <stdio.h>
#include <string.h>

// Wayland core
#include <wayland-client.h>

// Wayland protocols
#include <wayland-protocols/stable/xdg-shell.h>

#include <example/surface.h>

//=============
// Pointer
//=============

static void pointer_enter_handler(void *data,
          struct wl_pointer *wl_pointer,
          uint32_t serial,
          struct wl_surface *surface,
          wl_fixed_t surface_x,
          wl_fixed_t surface_y)
{
    (void)data;
    (void)wl_pointer;
    (void)serial;
    (void)surface;
    (void)surface_x;
    (void)surface_y;
}

static void pointer_leave_handler(void *data,
          struct wl_pointer *wl_pointer,
          uint32_t serial,
          struct wl_surface *surface)
{
    (void)data;
    (void)wl_pointer;
    (void)serial;
    (void)surface;
}

static void pointer_motion_handler(void *data,
           struct wl_pointer *wl_pointer,
           uint32_t time,
           wl_fixed_t surface_x,
           wl_fixed_t surface_y)
{
    (void)data;
    (void)wl_pointer;
    (void)time;
    (void)surface_x;
    (void)surface_y;
}

static void pointer_button_handler(void *data,
           struct wl_pointer *wl_pointer,
           uint32_t serial,
           uint32_t time,
           uint32_t button,
           uint32_t state)
{
    (void)data;
    (void)wl_pointer;
    (void)serial;
    (void)time;
    (void)button;
    (void)state;
}

static void pointer_axis_handler(void *data,
         struct wl_pointer *wl_pointer,
         uint32_t time,
         uint32_t axis,
         wl_fixed_t value)
{
    (void)data;
    (void)wl_pointer;
    (void)time;
    (void)axis;
    (void)value;
}

static void pointer_frame_handler(void *data,
          struct wl_pointer *wl_pointer)
{
    (void)data;
    (void)wl_pointer;
}

static void pointer_axis_source_handler(void *data,
            struct wl_pointer *wl_pointer,
            uint32_t axis_source)
{
    (void)data;
    (void)wl_pointer;
    (void)axis_source;
}

static void pointer_axis_stop_handler(void *data,
          struct wl_pointer *wl_pointer,
          uint32_t time,
          uint32_t axis)
{
    (void)data;
    (void)wl_pointer;
    (void)time;
    (void)axis;
}

static void pointer_axis_discrete_handler(void *data,
              struct wl_pointer *wl_pointer,
              uint32_t axis,
              int32_t discrete)
{
    (void)data;
    (void)wl_pointer;
    (void)axis;
    (void)discrete;
}

static const wl_pointer_listener pointer_listener = {
    .enter = pointer_enter_handler,
    .leave = pointer_leave_handler,
    .motion = pointer_motion_handler,
    .button = pointer_button_handler,
    .axis = pointer_axis_handler,
    .frame = pointer_frame_handler,
    .axis_source = pointer_axis_source_handler,
    .axis_stop = pointer_axis_stop_handler,
    .axis_discrete = pointer_axis_discrete_handler,
};

//=============
// Keyboard
//=============

static void keyboard_keymap_handler(void *data, struct wl_keyboard *keyboard,
        uint32_t format, int fd, uint32_t size)
{
    (void)data;
    (void)keyboard;
    (void)format;
    (void)fd;
    (void)size;
}

static void keyboard_enter_handler(void *data, struct wl_keyboard *keyboard,
        uint32_t serial, struct wl_surface *wl_surface, struct wl_array *keys)
{
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)wl_surface;
    (void)keys;
    fprintf(stderr, "Keyboard enter: wl_surface: %p\n", wl_surface);
    // Find surface by wl_surface.
    for (uint32_t i = 0; i < app->surface_list().size(); ++i) {
        if (wl_surface == app->surface_list()[i]->wl_surface()) {
            app->set_keyboard_focus_surface(app->surface_list()[i]);
            break;
        }
    }
}

static void keyboard_leave_handler(void *data, struct wl_keyboard *keyboard,
        uint32_t serial, struct wl_surface *surface)
{
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)surface;
}

static void keyboard_key_handler(void *data, struct wl_keyboard *keyboard,
        uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)time;
    (void)key;
    (void)state;
    fprintf(stderr, "Key! %d\n", key);
}

static void keyboard_modifiers_handler(void *data, struct wl_keyboard *keyboard,
        uint32_t serial,
        uint32_t mods_depressed,
        uint32_t mods_latched,
        uint32_t mods_locked,
        uint32_t group)
{
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)mods_depressed;
    (void)mods_latched;
    (void)mods_locked;
    (void)group;
}

static void keyboard_repeat_info_handler(void *data,
        struct wl_keyboard *keyboard,
        int rate, int delay)
{
    (void)data;
    (void)keyboard;
    fprintf(stderr, "keyboard_repeat_info_handler()\n");
    fprintf(stderr, " - rate: %d\n", rate);
    fprintf(stderr, " - delay: %d\n", delay);

    app->set_keyboard_rate(rate);
    app->set_keyboard_delay(delay);
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_keymap_handler,
    .enter = keyboard_enter_handler,
    .leave = keyboard_leave_handler,
    .key = keyboard_key_handler,
    .modifiers = keyboard_modifiers_handler,
    .repeat_info = keyboard_repeat_info_handler,
};

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

    // Init.
    this->_keyboard_rate = 0;
    this->_keyboard_delay = 0;

    this->_keyboard_focus_surface = nullptr;

    app = this;

    auto display = wl_display_connect(NULL);
    this->set_wl_display(display);
    if (display == NULL) {
        fprintf(stderr, "Can't connect to display.\n");
        exit(1);
    }
    printf("Connected to display.\n");

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (this->wl_compositor() == NULL ||
            this->xdg_wm_base() == NULL) {
        fprintf(stderr, "Can't find compositor or xdg_wm_base.\n");
        exit(1);
    } else {
        fprintf(stderr, "Found compositor and xdg_wm_base.\n");
    }

    xdg_wm_base_add_listener(this->xdg_wm_base(),
        &xdg_wm_base_listener, NULL);

    wl_display_roundtrip(this->wl_display());
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

uint32_t Application::keyboard_rate() const
{
    return this->_keyboard_rate;
}

uint32_t Application::keyboard_delay() const
{
    return this->_keyboard_delay;
}

void Application::set_keyboard_rate(uint32_t rate)
{
    this->_keyboard_rate = rate;
}

void Application::set_keyboard_delay(uint32_t delay)
{
    this->_keyboard_delay = delay;
}

void Application::add_surface(Surface *surface)
{
    this->_surface_list.push_back(surface);
}

void Application::remove_surface(Surface *surface)
{
    (void)surface;
    // TODO: Impl.
}

std::vector<Surface*> Application::surface_list() const
{
    return this->_surface_list;
}

void Application::set_keyboard_focus_surface(Surface *surface)
{
    this->_keyboard_focus_surface = surface;
}

Application *app = nullptr;
