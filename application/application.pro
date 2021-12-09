SOURCES += main.c \
    utils.c \
    application.c \
    surface.c \
    window.c \
    pointer-event.c

HEADERS += utils.h \
    application.h \
    surface.h \
    window.h \
    pointer-event.h \
    blusher-collections/include/blusher-collections.h

INCLUDEPATH += wayland-protocols
