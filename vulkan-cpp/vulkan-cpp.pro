SOURCES += main.cpp \
    vulkan/instance.cpp \
    vulkan/surface.cpp \
    vulkan/device.cpp

HEADERS += vulkan/instance.h \
    vulkan/surface.h \
    vulkan/device.h

CONFIG += link_pkgconfig

PKGCONFIG += cairo
