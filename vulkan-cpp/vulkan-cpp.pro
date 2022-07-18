SOURCES += main.cpp \
    vulkan/instance.cpp \
    vulkan/surface.cpp \
    vulkan/device.cpp \
    vulkan/swapchain.cpp \
    vulkan/utils.cpp

HEADERS += vulkan/instance.h \
    vulkan/surface.h \
    vulkan/device.h \
    vulkan/swapchain.h \
    vulkan/utils.h

CONFIG += link_pkgconfig

PKGCONFIG += cairo
