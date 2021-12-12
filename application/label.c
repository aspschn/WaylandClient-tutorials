#include "label.h"

// Std libs
#include <stdlib.h>
#include <string.h>

// Cairo / Pango
#include <pango/pangocairo.h>

// Blusher
#include "utils.h"

//=================
// Cairo / Pango
//=================
static void draw_text(bl_label *label,
        cairo_surface_t *cairo_surface, cairo_t *cr)
{
    PangoLayout *layout;
    PangoFontDescription *desc;

    layout = pango_cairo_create_layout(cr);

    pango_layout_set_text(layout, label->text, -1);

    desc = pango_font_description_from_string("serif");
    pango_font_description_set_size(desc,
        pixel_to_pango_size(label->font_size));

    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    cairo_save(cr);

    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

    pango_cairo_update_layout(cr, layout);

    // pango_layout_get_size(layout, &width, &height);
    cairo_move_to(cr, 0, 0);
    pango_cairo_show_layout(cr, layout);

    cairo_restore(cr);

    g_object_unref(layout);

    // Copy cairo data to the shm_data.
    int width = label->surface->width;
    int height = label->surface->height;
    int stride = width * 4;
    int size= stride * height;

    unsigned char *data = cairo_image_surface_get_data(cairo_surface);
    for (int i = 0; i < size; ++i) {
        ((unsigned char*)(label->surface->shm_data))[i] = data[i];
    }
}

//==============
// Label
//==============
bl_label* bl_label_new(bl_surface *parent, const char *text)
{
    bl_label *label = malloc(sizeof(bl_label));

    label->surface = bl_surface_new(parent);

    label->text = malloc(sizeof(char) * strlen(text) + 1);
    strncpy(label->text, text, strlen(text));
    label->text[strlen(text)] = '\0';
    label->font_size = 13;
    label->font_color = bl_color_from_rgb(0, 0, 0);

    return label;
}

void bl_label_show(bl_label *label)
{
    bl_surface_set_geometry(label->surface, 0, 0, 100, 50);

    cairo_surface_t *cairo_surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, label->surface->width, label->surface->height);
    cairo_t *cr = cairo_create(cairo_surface);

    draw_text(label, cairo_surface, cr);

//    bl_surface_show(label->surface);
    wl_surface_attach(label->surface->surface, label->surface->buffer, 0, 0);
    wl_surface_commit(label->surface->surface);

    wl_surface_commit(label->surface->parent->surface);
}

void bl_label_free(bl_label *label)
{
    bl_surface_free(label->surface);

    free(label->text);

    free(label);
}
