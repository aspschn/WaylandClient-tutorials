#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <cairo.h>

#include "xdg-shell.h"

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_subcompositor *subcompositor = NULL;
struct wl_surface *surface;
struct wl_egl_window *egl_window;
struct wl_region *region;
struct wl_output *output;

struct xdg_wm_base *xdg_wm_base = NULL;
struct xdg_surface *xdg_surface = NULL;
struct xdg_toplevel *xdg_toplevel = NULL;

EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;
GLuint program_object;

struct wl_surface *surface2;
struct wl_egl_window *egl_window2;
EGLSurface egl_surface2;
struct wl_subsurface *subsurface;

uint32_t image_width;
uint32_t image_height;
uint32_t image_size;
uint32_t *image_data;

void load_image()
{
    cairo_surface_t *cairo_surface = cairo_image_surface_create_from_png(
        "miku@2x.png");
    cairo_t *cr = cairo_create(cairo_surface);
    image_width = cairo_image_surface_get_width(cairo_surface);
    image_height = cairo_image_surface_get_height(cairo_surface);
    image_size = sizeof(uint32_t) * (image_width * image_height);
    image_data = malloc(image_size);
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

GLuint load_shader(const char *shader_src, GLenum type)
{
    GLuint shader;
    GLint compiled;

    // Create the shader object.
    shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }

    // Load the shader source.
    glShaderSource(shader, 1, &shader_src, NULL);

    // Compile the shader.
    glCompileShader(shader);

    // Check the compile status.
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint info_len = 0;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1) {
            char *info_log = malloc(sizeof(char));
            glGetShaderInfoLog(shader, info_len, NULL, info_log);
            fprintf(stderr, "Error compiling shader: %s\n", info_log);
            free(info_log);
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

int init(GLuint *program_object)
{
    GLbyte vertex_shader_str[] =
        "#version 300 es                \n"
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

    GLbyte fragment_shader_str[] =
        "#version 300 es                \n"
        "precision mediump float;       \n"
        "out vec4 fragColor;            \n"
        "in vec3 ourColor;              \n"
        "in vec2 TexCoord;              \n"
        "uniform sampler2D ourTexture;  \n"
        "void main()                    \n"
        "{                              \n"
//        "    fragColor = texture(ourTexture, TexCoord) * vec4(ourColor, 1.0); \n"
        "    fragColor = texture(ourTexture, TexCoord); \n"
        "}                              \n";

    GLuint vertex_shader;
    GLuint fragment_shader;
    GLint linked;

    vertex_shader = load_shader(vertex_shader_str, GL_VERTEX_SHADER);
    fragment_shader = load_shader(fragment_shader_str, GL_FRAGMENT_SHADER);

    *program_object = glCreateProgram();
    if (*program_object == 0) {
        fprintf(stderr, "glCreateProgram() - program_object is 0\n");
        return 0;
    }

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
            char *info_log = malloc(sizeof(char) * info_len);

            glGetProgramInfoLog(*program_object, info_len, NULL, info_log);
            fprintf(stderr, "Error linking program: %s\n", info_log);
            free(info_log);
        }

        glDeleteProgram(*program_object);
        return 0;
    }

    load_image();

    glClearColor(0.0f, 0.0f, 0.0f, 0.8f);
    return 1;
}

//===========
// XDG
//===========
static void xdg_wm_base_ping_handler(void *data,
        struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    GLfloat vVertices[] = {
         1.0f,  1.0f,  0.0f,    1.0f, 0.0f, 0.0f,   1.0f, 1.0f,     // top right
         1.0f, -1.0f,  0.0f,    0.0f, 0.0f, 1.0f,   1.0f, 0.0f,     // bottom right
        -1.0f, -1.0f,  0.0f,    0.0f, 0.0f, 1.0f,   0.0f, 0.0f,     // bottom left
        -1.0f,  1.0f,  0.0f,    1.0f, 0.0f, 0.0f,   0.0f, 1.0f,     // top left
    };
    GLfloat tVertices[] = {
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,
    };
    GLuint indices[] = {
        0, 1, 3,    // first triangle
        1, 2, 3,    // second triangle
    };

    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    // Set the viewport.
    glViewport(0, 0, 128, 128);

    // Clear the color buffer.
    glClearColor(0.0, 0.0, 0.0, 0.8f);
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

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices), vVertices, GL_STATIC_DRAW);

    // Position attribute.
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    // Color attribute.
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    GLuint t_vbo;
    glGenBuffers(1, &t_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, t_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tVertices), tVertices, GL_STATIC_DRAW);
    // Texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(2);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        image_width,
        image_height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        image_data
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

    eglSwapBuffers(egl_display, egl_surface);

    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping_handler,
};

static void xdg_surface_configure_handler(void *data,
        struct xdg_surface *xdg_surface, uint32_t serial)
{
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
}

static void xdg_toplevel_close_handler(void *data,
        struct xdg_toplevel *xdg_toplevel)
{
}

const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler,
};

//==============
// Output
//==============

static void output_geometry_handler(void *data,
        struct wl_output *output, int32_t x, int32_t y,
        int32_t physical_width, int32_t physical_height, int subpixel,
        const char *make, const char *model, int transform)
{
    (void)data;

    fprintf(stderr, "output_geometry_handler() - output: %p\n", output);
    fprintf(stderr, " |- x: %d, y: %d\n", x, y);
    fprintf(stderr, " |- phy width: %d, phy height: %d\n", physical_width, physical_height);
    fprintf(stderr, " |- subpixel: %d\n", subpixel);
    fprintf(stderr, " |- make: %s\n", make);
    fprintf(stderr, " |- model: %s\n", model);
    fprintf(stderr, " +- transform: %d\n", transform);
}

static void output_mode_handler(void *data,
        struct wl_output *output, uint32_t flags,
        int32_t width, int32_t height, int32_t refresh)
{
    (void)data;
    (void)output;
}

static void output_done_handler(void *data,
        struct wl_output *output)
{
    (void)data;
    (void)output;
}

static void output_scale_handler(void *data,
        struct wl_output *output, int32_t scale)
{
    (void)data;

    fprintf(stderr, "output_scale_handler() - output: %p, scale: %d\n", output, scale);
}

static void output_name_handler(void *data,
        struct wl_output *output, const char *name)
{
    (void)data;
    (void)output;
    fprintf(stderr, "output_name_handler() - output: %p, name: %s\n", output, name);
}

static void output_description_handler(void *data,
        struct wl_output *output, const char *description)
{
    (void)data;
    (void)output;
    fprintf(stderr, "output_description_handler() - description: %s\n",
        description);
}

static const struct wl_output_listener output_listener = {
    .geometry = output_geometry_handler,
    .mode = output_mode_handler,
    .done = output_done_handler,
    .scale = output_scale_handler,
    .name = output_name_handler,
    .description = output_description_handler,
};

//==============
// Global
//==============
static void global_registry_handler(void *data, struct wl_registry *registry,
        uint32_t id, const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0) {
        fprintf(stderr, "Interface is <wl_compositor>.\n");
        compositor = wl_registry_bind(
            registry,
            id,
            &wl_compositor_interface,
            version
        );
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        xdg_wm_base = wl_registry_bind(registry,
            id, &xdg_wm_base_interface, 1);
    } else if (strcmp(interface, "wl_subcompositor") == 0) {
        subcompositor = wl_registry_bind(registry,
            id, &wl_subcompositor_interface, 1);
    } else if (strcmp(interface, "wl_output") == 0) {
        fprintf(stderr, "Interface is <wl_output> v%d\n", version);
        output = wl_registry_bind(registry,
            id, &wl_output_interface, 2);
        wl_output_add_listener(output, &output_listener, NULL);
    } else {
        printf("(%d) Got a registry event for <%s> id <%d>\n",
            version, interface, id);
    }
}

static void global_registry_remover(void *data, struct wl_registry *registry,
        uint32_t id)
{
    printf("Got a registry losing event for <%d>\n", id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};

static void create_window()
{
    egl_window = wl_egl_window_create(surface, 480, 360);
    if (egl_window == EGL_NO_SURFACE) {
        fprintf(stderr, "Can't create egl window.\n");
        exit(1);
    } else {
        fprintf(stderr, "Created egl window.\n");
    }

    egl_surface = eglCreateWindowSurface(egl_display, egl_conf, egl_window,
        NULL);
    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context)) {
        fprintf(stderr, "Made current.\n");
    } else {
        fprintf(stderr, "Made current failed.\n");
    }

    glClearColor(1.0, 1.0, 0.0, 0.5);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();

    if (eglSwapBuffers(egl_display, egl_surface)) {
        fprintf(stderr, "Swapped buffers.\n");
    } else {
        fprintf(stderr, "Swapped buffers failed.\n");
    }
}

static void create_window2()
{
    egl_window2 = wl_egl_window_create(surface2, 100, 100);
    if (egl_window2 == EGL_NO_SURFACE) {
        exit(1);
    }

    egl_surface2 = eglCreateWindowSurface(egl_display, egl_conf,
        egl_window2, NULL);
    if (eglMakeCurrent(egl_display, egl_surface2, egl_surface2, egl_context)) {
        fprintf(stderr, "Made current for egl_surface2.\n");
    }

    glClearColor(0.0, 0.0, 1.0, 0.5);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();

    if (eglSwapBuffers(egl_display, egl_surface2)) {
        fprintf(stderr, "Swapped buffers for egl_surface2.\n");
    }
}

static void init_egl()
{
    EGLint major, minor, count, n, size;
    EGLConfig *configs;
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,
        EGL_WINDOW_BIT,
        EGL_RED_SIZE,
        8,
        EGL_GREEN_SIZE,
        8,
        EGL_BLUE_SIZE,
        8,
        EGL_ALPHA_SIZE,
        8,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_NONE,
    };

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION,
        2,
        EGL_NONE,
    };

    egl_display = eglGetDisplay((EGLNativeDisplayType)display);
    if (egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Can't create egl display\n");
        exit(1);
    } else {
        fprintf(stderr, "Created egl display.\n");
    }

    if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE) {
        fprintf(stderr, "Can't initialise egl display.\n");
        exit(1);
    }
    printf("EGL major: %d, minor %d\n", major, minor);

    eglGetConfigs(egl_display, NULL, 0, &count);
    printf("EGL has %d configs.\n", count);

    configs = calloc(count, sizeof *configs);

    eglChooseConfig(egl_display, config_attribs, configs, count, &n);

    for (int i = 0; i < n; ++i) {
        eglGetConfigAttrib(egl_display, configs[i], EGL_BUFFER_SIZE, &size);
        printf("Buffersize for config %d is %d\n", i, size);
        eglGetConfigAttrib(egl_display, configs[i], EGL_RED_SIZE, &size);
        printf("Red size for config %d is %d.\n", i, size);

        // Just choose the first one.
        egl_conf = configs[i];
        break;
    }

    egl_context = eglCreateContext(egl_display, egl_conf, EGL_NO_CONTEXT,
        context_attribs);
}

static void get_server_references()
{
    display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Can't connect to display.\n");
        exit(1);
    }
    printf("Connected to display.\n");

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (compositor == NULL || xdg_wm_base == NULL) {
        fprintf(stderr, "Can't find compositor or xdg_wm_base.\n");
        exit(1);
    } else {
        fprintf(stderr, "Found compositor and xdg_wm_base.\n");
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    get_server_references();

    // Check surface.
    fprintf(stderr, " - Checking surface...\n");
    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
        fprintf(stderr, "Can't create surface.\n");
        exit(1);
    } else {
        fprintf(stderr, "Created surface!\n");
    }
    surface2 = wl_compositor_create_surface(compositor);
    subsurface = wl_subcompositor_get_subsurface(subcompositor,
        surface2, surface);
    wl_subsurface_set_position(subsurface, -10, -10);

    xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);

    // Output.
    fprintf(stderr, " - Checking output...\n");
    if (output == NULL) {
        fprintf(stderr, "Output is NULL!\n");
        exit(1);
    }

    // Check xdg surface.
    fprintf(stderr, " - Checking xdg surface...\n");
    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
    if (xdg_surface == NULL) {
        fprintf(stderr, "Can't create xdg surface.\n");
        exit(1);
    } else {
        fprintf(stderr, "Created xdg surface!\n");
    }
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);

    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    if (xdg_toplevel == NULL) {
        exit(1);
    }
    xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);


    // MUST COMMIT! or not working on weston.
    wl_surface_commit(surface2);
    wl_surface_commit(surface);

    wl_display_roundtrip(display);

    // create_opaque_region();
    init_egl();
    create_window();
    if (init(&program_object) == 0) {
        fprintf(stderr, "Error init!\n");
    }
    create_window2();

    wl_surface_commit(surface);

    int res = wl_display_dispatch(display);
    while (res != -1) {
        res = wl_display_dispatch(display);
    }
    fprintf(stderr, "wl_display_dispatch() - res: %d\n", res);

    wl_display_disconnect(display);
    printf("Disconnected from display.\n");

    return 0;
}
