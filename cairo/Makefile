PKG_CONFIGS=`pkg-config --cflags --libs cairo` `pkg-config --cflags --libs pangocairo`
default:
	wayland-scanner client-header /usr/share/wayland-protocols/unstable/xdg-shell/xdg-shell-unstable-v6.xml xdg-shell.h
	wayland-scanner public-code /usr/share/wayland-protocols/unstable/xdg-shell/xdg-shell-unstable-v6.xml xdg-shell.c
	gcc $(PKG_CONFIGS) -lwayland-client -lwayland-egl -lEGL main.c utils.c xdg-shell.c
