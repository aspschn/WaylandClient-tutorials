#include <example/gl/object.h>

// C
#include <stdio.h>

#include <example/surface.h>

#define WINDOW_WIDTH 480
#define WINDOW_HEIGHT 360

namespace gl {

Object::Object(Surface *surface, int32_t x, int32_t y,
        uint32_t width, uint32_t height)
{
    this->_surface = surface;

    this->_x = x;
    this->_y = y;
    this->_width = width;
    this->_height = height;

    this->_image_data = nullptr;
    this->_image_width = 0;
    this->_image_height = 0;
    this->_texture = 0;

    this->_surface->add_child(this);
}

void Object::set_image(const uint8_t *image_data,
        uint64_t width, uint64_t height)
{
    this->_image_data = image_data;
    this->_image_width = width;
    this->_image_height = height;
}

void Object::init_texture()
{
    if (this->_image_data == nullptr) {
        fprintf(stderr, "[WARN] Image is null!\n");
    }

    glGenTextures(1, &this->_texture);
    glBindTexture(GL_TEXTURE_2D, this->_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        this->_image_width,
        this->_image_height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        this->_image_data
    );
    glGenerateMipmap(GL_TEXTURE_2D);

    fprintf(stderr, "Object::init_texture() - texture: %d\n", this->_texture);
}

int32_t Object::x() const
{
    return this->_x;
}

int32_t Object::y() const
{
    return this->_y;
}

int32_t Object::viewport_x() const
{
    return this->_x;
}

int32_t Object::viewport_y() const
{
    return this->_surface->scaled_height()
        - (this->_y)
        - (this->_height * this->_surface->scale());
}

uint32_t Object::width() const
{
    return this->_width;
}

uint32_t Object::height() const
{
    return this->_height;
}

void Object::set_x(int32_t x)
{
    if (this->_x != x) {
        this->_x = x;
    }
}

void Object::set_y(int32_t y)
{
    if (this->_y != y) {
        this->_y = y;
    }
}

uint32_t Object::scaled_width() const
{
    return this->_width * this->_surface->scale();
}

uint32_t Object::scaled_height() const
{
    return this->_height * this->_surface->scale();
}

GLuint Object::texture() const
{
    return this->_texture;
}

std::vector<glm::vec3> Object::vertices() const
{
    std::vector<glm::vec3> v;

    // Top right.
    v.push_back(glm::vec3(
        this->_width / WINDOW_WIDTH,
        this->_height / WINDOW_HEIGHT,
        0.0f
    ));

    // Bottom right.
    v.push_back(glm::vec3(
        this->_width / WINDOW_WIDTH,
        -(this->_height / WINDOW_HEIGHT),
        0.0f
    ));

    // Bottom left.
    v.push_back(glm::vec3(
        -(this->_width / WINDOW_WIDTH),
        -(this->_height / WINDOW_HEIGHT),
        0.0f
    ));

    // Top left.
    v.push_back(glm::vec3(
        -(1.0f - ((float)this->_x / WINDOW_WIDTH)),
        (1.0f - ((float)this->_y / WINDOW_HEIGHT)),
        0.0f
    ));

    return v;
}

} // namespace gl
