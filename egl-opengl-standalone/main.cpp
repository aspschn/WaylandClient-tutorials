#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// C++
#include <vector>
#include <memory>

#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
//#include <GLES3/gl3.h>

#include <glm/glm.hpp>

#include <sys/time.h>
#include <linux/input.h>

#include <cairo.h>

#include <example/application.h>
#include <example/surface.h>
#include <example/gl/context.h>
#include <example/gl/object.h>

#include "wayland-protocols/stable/xdg-shell.h"

#define WINDOW_WIDTH 480
#define WINDOW_HEIGHT 360

struct wl_egl_window *egl_window = NULL;

GLuint program_object;

//struct wl_subsurface *subsurface;

uint32_t image_width;
uint32_t image_height;
uint32_t image_size;
uint32_t *image_data;

uint32_t cursor_width;
uint32_t cursor_height;
uint32_t cursor_size;
uint32_t *cursor_data;

GLuint indices[] = {
    0, 1, 3,    // first triangle
    1, 2, 3,    // second triangle
};

//std::shared_ptr<Surface> surface = nullptr;
Surface *surface = nullptr;

std::vector<glm::ivec3> vectors;

//std::shared_ptr<gl::Object> cursor_object = nullptr;
gl::Object *cursor_object = nullptr;
glm::ivec3 cursor_vector;

/*
GLbyte vertex_shader_str[] =
    "#version 330 core              \n"
//        "attribute vec4 vPosition;      \n"
    "layout (location = 0) in vec3 aPos;        \n"
    "layout (location = 1) in vec3 aColor;      \n"
    "layout (location = 2) in vec2 aTexCoord;   \n"
    "out vec3 ourColor;             \n"
    "out vec2 TexCoord;             \n"
    "void main()                            \n"
    "{                                      \n"
    "    gl_Position = vec4(aPos, 1.0);     \n"
    "    ourColor = aColor;                 \n"
    "    TexCoord = aTexCoord;              \n"
    "}                                      \n";
*/

const uint32_t x = 100;
const uint32_t y = 100;
const uint32_t width = 50;
const uint32_t height = 100;

std::vector<glm::vec3> vertices = {
    {  ((float)width / WINDOW_WIDTH),  1.0f - ((float)height / WINDOW_HEIGHT), 0.0f }, // Top right
    {  ((float)width / WINDOW_WIDTH), -(1.0f - ((float)height / WINDOW_HEIGHT)), 0.0f }, // Bottom right
    { -(1.0f - ((float)x / WINDOW_WIDTH)), -(1.0f - ((float)height / WINDOW_HEIGHT)), 0.0f }, // Bottom left
    { -(1.0f - ((float)x / WINDOW_WIDTH)),  1.0f - ((float)y / WINDOW_HEIGHT), 0.0f }, // Top left
};

std::vector<glm::vec3> full_vertices = {
    {  1.0f,  1.0f, 0.0f },
    {  1.0f, -1.0f, 0.0f },
    { -1.0f, -1.0f, 0.0f },
    { -1.0f,  1.0f, 0.0f },
};

glm::vec3 colors[] = {
    { 1.0f, 1.0f, 1.0f, },
    { 0.0f, 1.0f, 0.0f, },
    { 0.0f, 0.0f, 1.0f, },
};

GLfloat tex_coords[] = {
    1.0f, 0.0f,
    1.0f, -1.0f,
    0.0f, -1.0f,
    0.0f, 0.0f,
};

void load_image()
{
    cairo_surface_t *cairo_surface = cairo_image_surface_create_from_png(
        "miku@2x.png");
    cairo_t *cr = cairo_create(cairo_surface);
    image_width = cairo_image_surface_get_width(cairo_surface);
    image_height = cairo_image_surface_get_height(cairo_surface);
    image_size = sizeof(uint32_t) * (image_width * image_height);
    image_data = (uint32_t*)malloc(image_size);
    memcpy(image_data, cairo_image_surface_get_data(cairo_surface), image_size);

    // Color correct.
    for (uint32_t i = 0; i < (image_width * image_height); ++i) {
        uint32_t color = 0x00000000;
        color += ((image_data[i] & 0xff000000));  // Set alpha.
        color += ((image_data[i] & 0x00ff0000) >> 16);   // Set blue.
        color += ((image_data[i] & 0x0000ff00));   // Set green.
        color += ((image_data[i] & 0x000000ff) << 16);  // Set red.
        image_data[i] = color;
    }

    cairo_surface_destroy(cairo_surface);
    cairo_destroy(cr);
}

void load_image2()
{
    cairo_surface_t *cairo_surface = cairo_image_surface_create_from_png(
        "cursor.png");
    cairo_t *cr = cairo_create(cairo_surface);
    cursor_width = cairo_image_surface_get_width(cairo_surface);
    cursor_height = cairo_image_surface_get_height(cairo_surface);
    cursor_size = sizeof(uint32_t) * (cursor_width * cursor_height);
    cursor_data = (uint32_t*)malloc(cursor_size);
    memcpy(cursor_data, cairo_image_surface_get_data(cairo_surface), cursor_size);

    // Color correct.
    for (uint32_t i = 0; i < (cursor_width * cursor_height); ++i) {
        uint32_t color = 0x00000000;
        color += ((cursor_data[i] & 0xff000000));  // Set alpha.
        color += ((cursor_data[i] & 0x00ff0000) >> 16);   // Set blue.
        color += ((cursor_data[i] & 0x0000ff00));   // Set green.
        color += ((cursor_data[i] & 0x000000ff) << 16);  // Set red.
        cursor_data[i] = color;
    }

    cairo_surface_destroy(cairo_surface);
    cairo_destroy(cr);
}

GLuint load_shader(const char *path, GLenum type)
{
    GLuint shader;
    GLint compiled;

    // Read file.
    uint8_t *code = nullptr;
    int64_t size;
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size = ftell(f) + 1;
    fseek(f, 0, SEEK_SET);

    code = new uint8_t[size];
    fread(code, size - 1, 1, f);
    code[size] = '\0';

    // Create the shader object.
    shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }
    fprintf(stderr, "Shader: %d\n", shader);

    // Load the shader source.
    glShaderSource(shader, 1, (const GLchar* const*)&code, NULL);

    // Compile the shader.
    glCompileShader(shader);

    // Check the compile status.
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint info_len = 0;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1) {
            char *info_log = (char*)malloc(sizeof(char));
            glGetShaderInfoLog(shader, info_len, NULL, info_log);
            fprintf(stderr, "Error compiling shader: %s\n", info_log);
            free(info_log);
        }

        glDeleteShader(shader);
        return 0;
    }

    delete[] code;

    return shader;
}

int init(GLuint *program_object)
{
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLint linked;

    vertex_shader = load_shader("shaders/shader.vert", GL_VERTEX_SHADER);
    fragment_shader = load_shader("shaders/shader.frag", GL_FRAGMENT_SHADER);

    fprintf(stderr, "Vertex shader: %d\n", vertex_shader);
    fprintf(stderr, "Fragment shader: %d\n", fragment_shader);

    *program_object = glCreateProgram();
    if (*program_object == 0) {
        fprintf(stderr, "glCreateProgram() - program_object is 0\n");
        return 0;
    }
    fprintf(stderr, "Program object created.\n");

    glAttachShader(*program_object, vertex_shader);
    glAttachShader(*program_object, fragment_shader);

    // Bind vPosition to attribute 0.
//    glBindAttribLocation(*program_object, 0, "vPosition");

    // Link the program.
    glLinkProgram(*program_object);

    // Check the link status.
    glGetProgramiv(*program_object, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint info_len = 0;
        glGetProgramiv(*program_object, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1) {
            char *info_log = (char*)malloc(sizeof(char) * info_len);

            glGetProgramInfoLog(*program_object, info_len, NULL, info_log);
            fprintf(stderr, "Error linking program: %s\n", info_log);
            free(info_log);
        }

        glDeleteProgram(*program_object);
        return 0;
    }

    load_image();
    load_image2();

    return 1;
}

static void recreate_window();

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
        if (state == XDG_TOPLEVEL_STATE_RESIZING) {
            fprintf(stderr, "Resizing! %dx%d\n", width, height);
            if (width != 0 && height != 0) {
                surface->set_size(width, height);
                recreate_window();
            }
        }
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

//==============
// Global
//==============
static void global_registry_handler(void *data, struct wl_registry *registry,
        uint32_t id, const char *interface, uint32_t version)
{
    (void)data;
    (void)registry;
    (void)id;
    (void)interface;
    (void)version;
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

static void gl_info()
{
    const GLubyte *version = glGetString(GL_VERSION);
    fprintf(stderr, "OpenGL version: %s\n", version);
}

static void recreate_window()
{
//    eglDestroySurface(egl_display, egl_surface);
    /*
    wl_egl_window_resize(egl_window,
        surface->scaled_width(), surface->scaled_height(), 0, 0);
    */
}

static void create_objects()
{
    // Object 1.
    gl::Object *obj = new gl::Object(surface, 0, 0, 200, 100);
    obj->set_image((const uint8_t*)image_data, image_width, image_height);
    obj->init_texture();
    vectors.push_back(glm::ivec3(2, 4, 0));

    // Object 2.
    gl::Object *obj2 = new gl::Object(surface, 50, 150, 100, 100);
    obj2->set_image((const uint8_t*)image_data, image_width, image_height);
    obj2->init_texture();
    vectors.push_back(glm::ivec3(4, 2, 0));

    // Object 3.
    gl::Object *obj3 = new gl::Object(surface, 0, 0, 64, 64);
    obj3->set_image((const uint8_t*)image_data, image_width, image_height);
    obj3->init_texture();
    vectors.push_back(glm::ivec3(6, 4, 0));
}

static void move_objects()
{
    for (uint64_t i = 0; i < surface->children().size(); ++i) {
        auto object = surface->children()[i];

        if (object == cursor_object) {
            continue;
        }

        auto& vector = vectors[i - 1];

        object->set_x(object->x() + vector.x);
        object->set_y(object->y() + vector.y);
        // Bottom bound.
        if (object->y() + object->scaled_height() >= surface->scaled_height()) {
            vector.y = -(vector.y);
        }
        // Right bound.
        if (object->x() + object->scaled_width() >= surface->scaled_width()) {
            vector.x = -(vector.x);
        }
        // Top bound.
        if (object->y() <= 0) {
            vector.y = -(vector.y);
        }
        // Left bound.
        if (object->x() <= 0) {
            vector.x = -(vector.x);
        }
    }

    // Cursor.
    cursor_object->set_x(cursor_object->x() + cursor_vector.x);
    cursor_object->set_y(cursor_object->y() + cursor_vector.y);
}

static void process_keyboard()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

    auto surface = app->keyboard_focus_surface();
    if (surface == nullptr) {
        return;
    }
    auto& keyboard_state = surface->keyboard_state;

    if (keyboard_state.pressed == true) {
        if (!keyboard_state.processed) {
            keyboard_state.pressed_time = ms;
            keyboard_state.elapsed_time = ms;
        } else {
            keyboard_state.elapsed_time = ms;
        }
        if (keyboard_state.repeating()) {
            fprintf(stderr, "Key repeat!\n");
        }
        if (keyboard_state.should_processed(KEY_ENTER)) {
            fprintf(stderr, "Enter.\n");
            gl::Object *obj = new gl::Object(surface, 0, 0, 128, 128);
            obj->set_image((const uint8_t*)image_data, image_width, image_height);
            obj->init_texture();
            vectors.push_back(glm::ivec3(2, 2, 0));
        } else if (keyboard_state.key == KEY_RIGHT && !keyboard_state.processed) {
            cursor_vector.x = 2;
            cursor_vector.y = 0;
        } else if (keyboard_state.key == KEY_LEFT && !keyboard_state.processed) {
            cursor_vector.x = -2;
            cursor_vector.y = 0;
        } else if (keyboard_state.key == KEY_DOWN && !keyboard_state.processed) {
            cursor_vector.y = 2;
            cursor_vector.x = 0;
        } else if (keyboard_state.key == KEY_UP && !keyboard_state.processed) {
            cursor_vector.y = -2;
            cursor_vector.x = 0;
        } else if (keyboard_state.key == KEY_MINUS && !keyboard_state.processed) {
            surface->set_size(surface->width() - 10, surface->height() - 10);
            recreate_window();
        } else if (keyboard_state.key == KEY_EQUAL && !keyboard_state.processed) {
            surface->set_size(surface->width() + 10, surface->height() + 10);
            recreate_window();
        }
        keyboard_state.processed = true;
    } else {
        keyboard_state.processed = false;
        keyboard_state.pressed_time = 0;
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    Application application(argc, argv);

//    surface = std::make_shared<Surface>(Surface::Type::Toplevel,
//        WINDOW_WIDTH, WINDOW_HEIGHT);
    surface = new Surface(Surface::Type::Toplevel, WINDOW_WIDTH, WINDOW_HEIGHT);

    wl_display_roundtrip(app->wl_display());

    // create_opaque_region();
    if (!GL_VERSION_4_6) {
        fprintf(stderr, "No GL_VERSION_4_6!\n");
    }
    fprintf(stderr, "Has GL_VERSION_4_6\n");

    gl_info();

    // fprintf(stderr, "GLEW experimental: %d\n", glewExperimental);
    /*
    GLenum err = glewInit();
    if (err != GLEW_OK && err != GLEW_ERROR_NO_GLX_DISPLAY) {
        fprintf(stderr, "Failed to init GLEW! err: %d %s\n", err,
            glewGetErrorString(err));
        exit(1);
    }
    */

    if (init(&program_object) == 0) {
        fprintf(stderr, "Error init!\n");
    }
    //    cursor_object = std::make_shared<gl::Object>(surface, 0, 0, 12, 12);
    fprintf(stderr, "Creating cursor object.\n");
    cursor_object = new gl::Object(surface, 0, 0, 12, 12);
    cursor_object->set_image((uint8_t*)cursor_data, cursor_width, cursor_height);
    cursor_object->init_texture();
    fprintf(stderr, " - %p\n", cursor_object);

    wl_surface_commit(surface->wl_surface());

    create_objects();

    surface->draw_frame(program_object);

    int res = wl_display_dispatch(app->wl_display());
    while (res != -1) {
        // Process keyboard state.
        process_keyboard();
        // Move.
        move_objects();

        surface->draw_frame(program_object);
        res = wl_display_dispatch(app->wl_display());
    }
    fprintf(stderr, "wl_display_dispatch() - res: %d\n", res);

    wl_display_disconnect(app->wl_display());
    printf("Disconnected from display.\n");

    return 0;
}
