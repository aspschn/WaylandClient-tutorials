OBJ=xdg-shell.o

PKG_CONFIG=`pkg-config --cflags --libs cairo`

default: $(OBJ)
	g++ main.cpp $^ -lwayland-client -lwayland-egl -lEGL -lGL -lGLEW $(PKG_CONFIG)

wayland-protocols:
	wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.h
	wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.c
