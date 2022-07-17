SOURCES += main.cpp \
    vulkan/instance.cpp \
    vulkan/surface.cpp

HEADERS += vulkan/instance.h \
    vulkan/surface.h

CONFIG += link_pkgconfig

PKGCONFIG += cairo
