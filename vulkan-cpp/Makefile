PKG_CONFIG=`pkg-config --cflags --libs cairo`
default: xdg-shell.o
	wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.h
	wayland-scanner public-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.c
	g++ -std=c++17 -fPIC xdg-shell.o main.cpp -lwayland-client -lvulkan $(PKG_CONFIG)

xdg-shell.o:
	gcc -c -fPIC xdg-shell.c -o xdg-shell.o

shaders:
	glslc shader.vert -o vert.spv
	glslc shader.frag -o frag.spv

