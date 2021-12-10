SOURCES += main.c \
    utils.c \
    application.c \
    surface.c \
    window.c \
    color.c \
    pointer-event.c

HEADERS += utils.h \
    application.h \
    surface.h \
    window.h \
    color.h \
    pointer-event.h \
    blusher-collections/include/blusher-collections.h

INCLUDEPATH += wayland-protocols blusher-collections/include

DISTFILES += Makefile
