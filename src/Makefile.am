

bin_PROGRAMS = capa

capa_SOURCES = \
	app.h app.c \
	app-display.h app-display.c \
	camera.h camera.c \
	camera-list.h camera-list.c \
	camera-manager.h camera-manager.c \
	camera-picker.h camera-picker.c \
	main.c \
	params.h params.c

capa_LDFLAGS = $(GTK_LIBS) $(GPHOTO2_LIBS) $(GLADE2_LIBS) $(WARNING_CFLAGS)
capa_CFLAGS = $(GTK_CFLAGS) $(GPHOTO2_CFLAGS) $(GLADE2_CFLAGS) $(WARNING_CFLAGS)

-include $(top_srcdir)/git.mk
