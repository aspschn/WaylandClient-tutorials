SOURCES += main.c \
    utils.c \
    application.c \
    surface.c \
    window.c \
    title-bar.c \
    color.c \
    label.c \
    pointer-event.c

HEADERS += utils.h \
    application.h \
    surface.h \
    window.h \
    title-bar.h \
    color.h \
    label.h \
    pointer-event.h \
    blusher-collections/include/blusher-collections.h

INCLUDEPATH += wayland-protocols blusher-collections/include \
    /usr/include/pango-1.0 \
    /usr/include/glib-2.0 \
    /usr/lib/glib-2.0/include \
    /usr/include/harfbuzz \
    /usr/include/freetype2 \
    /usr/include/libpng16 \
    /usr/include/libmount \
    /usr/include/blkid \
    /usr/include/fribidi \
    /usr/include/cairo \
    /usr/include/pixman-1

DISTFILES += Makefile
