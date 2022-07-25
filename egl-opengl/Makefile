.PHONY: wayland-protocols

C_OBJ=wayland-protocols/stable/xdg-shell.o
OBJ=src/surface.o \
	src/object.o

PKG_CONFIG=`pkg-config --cflags --libs cairo`

default: $(C_OBJ) $(OBJ)
	g++ -Iinclude main.cpp $^ -lwayland-client -lwayland-egl -lEGL -lGL -lGLEW $(PKG_CONFIG)

src/%.o: src/%.cpp
	$(CXX) -c -Iinclude -fPIC -o $@ $<

wayland-protocols/stable/%.o: wayland-protocols/stable/%.c
	$(CC) -c -fPIC -o $@ $<

wayland-protocols:
	wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml wayland-protocols/stable/xdg-shell.h
	wayland-scanner private-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml wayland-protocols/stable/xdg-shell.c

clean:
	rm -f a.out
	rm -f *.o
	rm -f wayland-protocols/stable/*.h
	rm -f wayland-protocols/stable/*.c
	rm -f wayland-protocols/stable/*.o
