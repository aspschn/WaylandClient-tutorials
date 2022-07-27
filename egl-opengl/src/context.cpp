#include <example/gl/context.h>

// C
#include <stdlib.h>
#include <stdio.h>

namespace gl {

Context::Context(EGLDisplay display)
{
    this->_egl_display = display;

    this->_egl_configs = nullptr;

    EGLint major, minor, count, n, size;
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

    eglBindAPI(EGL_OPENGL_API);

    if (eglInitialize(this->_egl_display, &major, &minor) != EGL_TRUE) {
        fprintf(stderr, "Can't initialise egl display.\n");
        return;
    }
    printf("EGL major: %d, minor %d\n", major, minor);

    eglGetConfigs(this->_egl_display, NULL, 0, &count);
    printf("EGL has %d configs.\n", count);

    this->_egl_configs = new EGLConfig[count];

    eglChooseConfig(this->_egl_display,
        config_attribs, this->_egl_configs, count, &n);

    for (int i = 0; i < n; ++i) {
        eglGetConfigAttrib(this->_egl_display,
            this->_egl_configs[i], EGL_BUFFER_SIZE, &size);
        printf("Buffersize for config %d is %d\n", i, size);
        eglGetConfigAttrib(this->_egl_display,
            this->_egl_configs[i], EGL_RED_SIZE, &size);
        printf("Red size for config %d is %d.\n", i, size);
    }
    // Just choose the first one.
    this->_egl_config = this->_egl_configs[0];

    this->_egl_context = eglCreateContext(this->_egl_display,
        this->_egl_config, EGL_NO_CONTEXT,
        context_attribs);

    eglMakeCurrent(this->_egl_display,
        EGL_NO_SURFACE, EGL_NO_SURFACE,
        this->_egl_context);
}

EGLConfig Context::egl_config()
{
    return this->_egl_config;
}

EGLContext Context::egl_context()
{
    return this->_egl_context;
}

} // namespace gl
