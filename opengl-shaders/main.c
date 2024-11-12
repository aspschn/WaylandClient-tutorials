#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>

#include <sys/time.h>

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
// #include <GLES2/gl2.h>

#include "xdg-shell.h"

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_subcompositor *subcompositor = NULL;
struct wl_surface *surface;
struct wl_egl_window *egl_window;
struct wl_region *region;
struct wl_callback *frame_callback;
struct wl_callback *animation_callback;

struct xdg_wm_base *xdg_wm_base = NULL;
struct xdg_surface *xdg_surface = NULL;
struct xdg_toplevel *xdg_toplevel = NULL;

EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;
GLuint program_object;

int window_width = 480;
int window_height = 360;
int frame_done = 0;
float pos_x = 1.0f;
float pos_y = 1.0f;


GLuint load_shader(const char *shader_path, GLenum type)
{
    GLuint shader;
    GLint compiled;

    // Create the shader object.
    shader = glCreateShader(type);
    if (shader == 0) {
        return 0;
    }

    char *shader_s;
    // Read shader from source.
    {
        FILE *f = fopen(shader_path, "r");
        fseek(f, 0, SEEK_END);
        long file_size = ftell(f);
        rewind(f);

        shader_s = malloc(file_size + 1);
        fread(shader_s, 1, file_size, f);
        shader_s[file_size] = '\0';

        fclose(f);
    }

    // Load the shader source.
    glShaderSource(shader, 1, (const char**)&shader_s, NULL);

    // Compile the shader.
    glCompileShader(shader);

    // Check the compile status.
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint info_len = 0;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1) {
            char *info_log = malloc(sizeof(char));
            fprintf(stderr, "Error compiling shader: %s\n", info_log);
            free(info_log);
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

void program_shaders(GLuint program, GLuint vert_shader, GLuint frag_shader)
{
    glAttachShader(program, vert_shader);
    glAttachShader(program, frag_shader);

    glLinkProgram(program);

    // Check the link status.
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint info_len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_len);
        if (info_len > 1) {
            char *info_log = malloc(sizeof(char) * info_len);

            glGetProgramInfoLog(program, info_len, NULL, info_log);
            fprintf(stderr, "Error linking program: %s\n", info_log);
            free(info_log);
        }

        glDeleteProgram(program);
    }
}

void set_vert_uniforms(GLuint program)
{
    GLuint pos = glGetUniformLocation(program, "resolution");
    float resolution[2] = { (float)window_width, (float)window_height };
    glUniform2fv(pos, 1, resolution);
}

void set_uniform_color(GLuint program, float *color)
{
    GLuint pos = glGetUniformLocation(program, "color");
    float col[4] = { color[0], color[1], color[2], color[3] };
    glUniform4fv(pos, 1, col);
}

void set_uniform_validGeometry(GLuint program,
                               float x,
                               float y,
                               float width,
                               float height)
{
    GLuint pos = glGetUniformLocation(program, "validGeometry");
    float geo[4] = { x, y, width, height };
    glUniform4fv(pos, 1, geo);
}

void set_uniform_geometry(GLuint program,
                          float x,
                          float y,
                          float width,
                          float height)
{
    GLuint pos = glGetUniformLocation(program, "geometry");
    float geo[4] = { x, y, width, height };
    glUniform4fv(pos, 1, geo);
}

void calc_points(float x, float y, float width, float height, float *points)
{
    float half_width = window_width / 2;
    float half_height = window_height / 2;

    // Top-left
    points[0] = x;
    points[1] = y;
    // Bottom-left
    points[3] = x;
    points[4] = y + height;
    // Bottom-right
    points[6] = x + width;
    points[7] = y + height;
    // Top-right
    points[9] = x + width;
    points[10] = y;
}

void draw_frame()
{
    GLfloat vVertices[] = {
        0.0f, 0.5f, 0.0f,   // top left
        -0.5f, -0.5f, 0.0f, // bottom left
        0.5f, -0.5f, 0.0f,  // bottom right
        0.5f, 0.5f, 0.0f,   // top right
    };
    GLuint indices[] = {
        0, 1, 3,        // Top left, bottom left, bottom right
        // 3, 2, 1,
        1, 2, 3,        // Bottom left, top right, bottom right
    };

    fprintf(stderr, "Draw frame.\n");

    GLuint vert_shader;
    GLuint frag_shader;
    vert_shader = load_shader("basic_vert_shader.vert", GL_VERTEX_SHADER);
    frag_shader = load_shader("color_frag_shader.frag", GL_FRAGMENT_SHADER);
    program_shaders(program_object, vert_shader, frag_shader);

    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

    // Set the viewport.
    glViewport(0, 0, window_width, window_height);

    // Clear the color buffer.
    glClearColor(1.0, 0.0, 0.0, 0.8);
    // Blend.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClear(GL_COLOR_BUFFER_BIT);
    // Use the program object.
    glUseProgram(program_object);
    set_vert_uniforms(program_object);


   float color[] = { 1.0f, 0.0f, 0.0f, 0.9f };

    // VAO.
    GLuint vao;
    glGenVertexArrays(1, &vao);

    // EBO.
    GLuint ebo;
    glGenBuffers(1, &ebo);

    // VBO.
    GLuint vbo;
    glGenBuffers(1, &vbo);


    // 1st Rectangle.
    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
        GL_STATIC_DRAW);

    calc_points(10.0f, 10.0f, 100.0f, 100.0f, vVertices);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        sizeof(vVertices),
        vVertices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    set_uniform_color(program_object, color);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);


    // 2nd Rectangle.
    glBindVertexArray(vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
        GL_STATIC_DRAW);

    calc_points(30.0f, 30.0f, 300.0f, 300.0f, vVertices);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        sizeof(vVertices),
        vVertices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    color[0] = 0.0f;
    color[1] = 1.0f;
    color[2] = 0.0f;
    color[3] = 0.5f;
    set_uniform_color(program_object, color);
    set_uniform_validGeometry(program_object, 10.0f, 10.0f, 100.0f, 100.0f);
    set_uniform_geometry(program_object, 30.0f, 30.0f, 300.0f, 300.0f);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

    struct timeval start;
    gettimeofday(&start, NULL);

    eglSwapBuffers(egl_display, egl_surface);

    struct timeval end;
    gettimeofday(&end, NULL);
    fprintf(stderr, "[%f] eglSwapBuffers\n",
        (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0);

    // Clean up.
    glDetachShader(program_object, vert_shader);
    glDetachShader(program_object, frag_shader);
}

//===========
// XDG
//===========
static void xdg_wm_base_ping_handler(void *data,
        struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
    draw_frame();

    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping_handler,
};

static void xdg_surface_configure_handler(void *data,
        struct xdg_surface *xdg_surface, uint32_t serial)
{
    // fprintf(stderr, "xdg_surface_configure_handler()\n");
    xdg_surface_ack_configure(xdg_surface, serial);
}

const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler,
};

static void xdg_toplevel_configure_handler(void *data,
        struct xdg_toplevel *xdg_toplevel, int32_t width, int32_t height,
        struct wl_array *states)
{
    void *it;

    wl_array_for_each(it, states) {
        enum xdg_toplevel_state state = *((int*)it);
        switch (state) {
        case XDG_TOPLEVEL_STATE_RESIZING:
            fprintf(stderr, "XDG toplevel resizing: %dx%d\n", width, height);
            wl_egl_window_resize(egl_window, width, height, 0, 0);
            xdg_surface_set_window_geometry(xdg_surface, 50, 50, width - 50, height - 50);
            wl_surface_commit(surface);
            window_width = width;
            window_height = height;
            if (frame_done == 1) {
                draw_frame();
                frame_done = 0;
            }
            break;
        default:
            break;
        }
    }
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
// Frame
//==============

static void frame_callback_handler(void *data,
                                   struct wl_callback*,
                                   uint32_t);

static struct wl_callback_listener frame_callback_listener = {
    .done = frame_callback_handler,
};

static void frame_callback_handler(void *data,
                                   struct wl_callback *wl_callback,
                                   uint32_t time)
{
    wl_callback_destroy(wl_callback);
    frame_callback = wl_surface_frame(surface);
    wl_callback_add_listener(frame_callback,
        &frame_callback_listener, NULL);

    frame_done = 1;

    fprintf(stderr, "Frame callback done.\n");
}

static void animation_callback_handler(void *data,
                                       struct wl_callback*,
                                       uint32_t);

static struct wl_callback_listener animation_callback_listener = {
    .done = animation_callback_handler,
};

static void animation_callback_handler(void *data,
                                       struct wl_callback *wl_callback,
                                       uint32_t time)
{
    wl_callback_destroy(wl_callback);
    animation_callback = wl_surface_frame(surface);
    wl_callback_add_listener(animation_callback,
        &animation_callback_listener, NULL);

    wl_surface_commit(surface);
    fprintf(stderr, "Animation callback\n");
}

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
        EGL_OPENGL_BIT,
        EGL_NONE,
    };

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION,
        4,
        EGL_CONTEXT_MINOR_VERSION,
        6,
        EGL_NONE,
    };

    // Important for OpenGL 4.6, not ES.
    eglBindAPI(EGL_OPENGL_API);

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
    free(configs);

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

    // Frame callback.
    frame_callback = wl_surface_frame(surface);
    wl_callback_add_listener(frame_callback,
        &frame_callback_listener, NULL);

    animation_callback = wl_surface_frame(surface);
    wl_callback_add_listener(animation_callback,
        &animation_callback_listener, NULL);

    xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);

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
    wl_surface_commit(surface);

    wl_display_roundtrip(display);

    // Test window geometry.
    xdg_surface_set_window_geometry(xdg_surface, 50, 50,
        window_width - 50, window_height - 50);
    wl_surface_commit(surface);

    // create_opaque_region();
    init_egl();
    create_window();

    program_object = glCreateProgram();
    if (program_object == 0) {
        fprintf(stderr, "glCreateProgram() - program_object is 0\n");
        return 0;
    }

    wl_surface_commit(surface);

    int res = wl_display_dispatch(display);
    while (res != -1) {
        res = wl_display_dispatch(display);
        fprintf(stderr, "wl_display_dispatch()\n");
    }
    fprintf(stderr, "wl_display_dispatch() - res: %d\n", res);

    wl_display_disconnect(display);
    printf("Disconnected from display.\n");

    return 0;
}
