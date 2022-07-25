#ifndef _GL_CONTEXT_H
#define _GL_CONTEXT_H

// C
#include <stdint.h>

// EGL
#include <EGL/egl.h>

namespace gl {

class Context
{
public:
    Context(EGLDisplay display);

    EGLConfig egl_config();
    EGLContext egl_context();

private:
    EGLDisplay _egl_display;

    EGLConfig _egl_config;
    EGLContext _egl_context;
};

} // namespace gl

#endif /* _GL_CONTEXT_H */
