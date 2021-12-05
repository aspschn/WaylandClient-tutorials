default:
	wayland-scanner client-header /usr/share/wayland-protocols/unstable/xdg-shell/xdg-shell-unstable-v6.xml xdg-shell.h
	wayland-scanner public-code /usr/share/wayland-protocols/unstable/xdg-shell/xdg-shell-unstable-v6.xml xdg-shell.c
	gcc -lwayland-client -lwayland-egl -lEGL -lGLESv2 main.c xdg-shell.c
