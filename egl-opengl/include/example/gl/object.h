#ifndef _GL_OBJECT_H
#define _GL_OBJECT_H

// C
#include <stdint.h>

// C++
#include <vector>

// GLEW
#define GLEW_EGL
#include <GL/glew.h>

// GLM
#include <glm/glm.hpp>

class Surface;

namespace gl {

class Object
{
public:
    Object(Surface *surface, int32_t x, int32_t y,
            uint32_t width, uint32_t height);

    void set_image(const uint8_t *image_data,
            uint64_t width, uint64_t height);

    void init_texture();

    int32_t x() const;
    int32_t y() const;
    int32_t viewport_x() const;
    int32_t viewport_y() const;
    uint32_t width() const;
    uint32_t height() const;

    uint32_t scaled_width() const;
    uint32_t scaled_height() const;

    void set_x(int32_t x);
    void set_y(int32_t y);

    GLuint texture() const;

    std::vector<glm::vec3> vertices() const;

private:
    Surface *_surface;

    int32_t _x;
    int32_t _y;
    uint32_t _width;
    uint32_t _height;

    const uint8_t *_image_data;
    uint64_t _image_width;
    uint64_t _image_height;
    GLuint _texture;
};

} // namespace gl

#endif /* _GL_OBJECT_H */
