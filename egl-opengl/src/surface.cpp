#include <example/surface.h>

Surface::Surface(uint32_t width, uint32_t height)
{
    this->_width = width;
    this->_height = height;
    this->_scale = 2;
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
}

uint32_t Surface::scaled_width() const
{
    return this->_width * this->_scale;
}

uint32_t Surface::scaled_height() const
{
    return this->_height * this->_scale;
}
