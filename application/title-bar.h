#ifndef _BLUSHER_TITLE_BAR_H
#define _BLUSHER_TITLE_BAR_H

#define BLUSHER_TITLE_BAR_HEIGHT 30.0

typedef struct bl_surface bl_surface;
typedef struct bl_window bl_window;

typedef struct bl_title_bar {
    bl_surface *surface;
    bl_window *window;

    bl_surface *close_button;
} bl_title_bar;

bl_title_bar* bl_title_bar_new(bl_window *window);

void bl_title_bar_show(bl_title_bar *title_bar);

void bl_title_bar_free(bl_title_bar *title_bar);

#endif /* _BLUSHER_TITLE_BAR_H */
