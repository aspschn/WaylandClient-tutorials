SOURCES += src/application.cpp \
    src/object.cpp \
    src/context.cpp \
    src/surface.cpp \
    src/keyboard-state.cpp \
    main.cpp

INCLUDEPATH += ./include

HEADERS += include/example/application.h \
    include/example/surface.h \
    include/example/keyboard-state.h \
    include/example/gl/context.h \
    include/example/gl/object.h

CONFIG += link_pkgconfig

PKGCONFIG += cairo
