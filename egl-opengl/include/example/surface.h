#ifndef _SURFACE_H
#define _SURFACE_H

// C
#include <stdint.h>

class Surface
{
public:
    Surface(uint32_t width, uint32_t height);

    uint32_t width() const;
    uint32_t height() const;
    uint32_t scale() const;

    void set_size(uint32_t width, uint32_t height);

    uint32_t scaled_width() const;
    uint32_t scaled_height() const;

private:
    uint32_t _width;
    uint32_t _height;
    uint32_t _scale;
};

#endif /* _SURFACE_H */
