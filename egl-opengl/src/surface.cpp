#include <example/surface.h>

// C
#include <stdio.h>

// OpenGL
#define GLEW_EGL
#include <GL/glew.h>

// GLM
#include <glm/glm.hpp>

#include <example/application.h>
#include <example/gl/object.h>

static GLuint indices[] = {
    0, 1, 3,    // first triangle
    1, 2, 3,    // second triangle
};

static std::vector<glm::vec3> full_vertices = {
    {  1.0f,  1.0f, 0.0f },
    {  1.0f, -1.0f, 0.0f },
    { -1.0f, -1.0f, 0.0f },
    { -1.0f,  1.0f, 0.0f },
};

static GLfloat tex_coords[] = {
    1.0f, 0.0f,
    1.0f, -1.0f,
    0.0f, -1.0f,
    0.0f, 0.0f,
};

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

    fprintf(stderr, "xdg_toplevel_configure_handler()\n");
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

    if (this->_scale > 1) {
        wl_surface_set_buffer_scale(this->_wl_surface, this->_scale);
    }

    if (this->_type == Surface::Type::Toplevel) {
        fprintf(stderr, "Surface is Toplevel.\n");
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
        fprintf(stderr, "Created egl window. %dx%d\n",
            this->scaled_width(), this->scaled_height());
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

    app->add_surface(this);
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

    wl_egl_window_resize(this->_wl_egl_window,
        this->scaled_width(), this->scaled_height(), 0, 0);
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
    EGLBoolean result;
    result = eglSwapBuffers(this->_context->egl_display(), this->_egl_surface);
    if (result == EGL_FALSE) {
        fprintf(stderr, "Failed to swap buffers!\n");
        return;
    }
}

void Surface::draw_frame(GLuint program_object)
{
    eglMakeCurrent(this->_context->egl_display(),
        this->_egl_surface, this->_egl_surface,
        this->_context->egl_context());

    glViewport(0, 0, this->scaled_width(), this->scaled_height());

    // Clear the color buffer.
    glClearColor(0.5, 0.5, 0.5, 0.8);
    glClear(GL_COLOR_BUFFER_BIT);

    // Use the program object.
    glUseProgram(program_object);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLuint vbo[2];
    glGenBuffers(2, vbo);

    for (auto& object: this->_children) {
        glBindVertexArray(vao);

        // Position attribute.
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER,
            sizeof(glm::vec3) * full_vertices.size(),
            full_vertices.data(),
            GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(0);

        // Texture coord attribute
        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coords), tex_coords, GL_STATIC_DRAW);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
        glEnableVertexAttribArray(1);

//        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, object->texture());

        glBindVertexArray(vao);

        glViewport(object->viewport_x(), object->viewport_y(),
            object->scaled_width(), object->scaled_height());
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
    }

    this->swap_buffers();

    // Free.
    glDeleteBuffers(2, vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vao);
}

void Surface::add_child(gl::Object *child)
{
    this->_children.push_back(child);
}

std::vector<gl::Object*> Surface::children() const
{
    return this->_children;
}
