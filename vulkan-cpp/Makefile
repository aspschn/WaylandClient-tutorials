PKG_CONFIG=`pkg-config --cflags --libs cairo`
default: xdg-shell.o
	g++ -std=c++17 -fPIC xdg-shell.o main.cpp -lwayland-client -lvulkan $(PKG_CONFIG)

wayland-protocols:
	wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.h
	wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.c

xdg-shell.o: wayland-protocols
	gcc -c -fPIC xdg-shell.c -o xdg-shell.o

shaders:
	glslc shader.vert -o vert.spv
	glslc shader.frag -o frag.spv

