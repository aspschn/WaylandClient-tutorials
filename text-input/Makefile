default:
	wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.h
	wayland-scanner public-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.c
	wayland-scanner client-header /usr/share/wayland-protocols/unstable/text-input/text-input-unstable-v3.xml text-input.h
	wayland-scanner public-code /usr/share/wayland-protocols/unstable/text-input/text-input-unstable-v3.xml text-input.c
	gcc -lwayland-client -lwayland-egl -lEGL -lGLESv2 main.c xdg-shell.c text-input.c
