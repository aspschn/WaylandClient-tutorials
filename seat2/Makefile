default:
	wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.h
	wayland-scanner public-code /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell.c
	gcc -lwayland-client -lwayland-egl -lwayland-cursor -lEGL -lGLESv2 main.c utils.c xdg-shell.c
