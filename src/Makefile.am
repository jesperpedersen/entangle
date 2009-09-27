

noinst_LTLIBRARIES = libcapa_backend.la libcapa_frontend.la

bin_PROGRAMS = capa

libcapa_backend_la_SOURCES = \
	backend/app.h backend/app.c \
	backend/camera.h backend/camera.c \
	backend/camera-list.h backend/camera-list.c \
	backend/control.h backend/control.c \
	backend/control-group.h backend/control-group.c \
	backend/control-layout.h backend/control-layout.c \
	backend/progress.h backend/progress.c \
	backend/params.h backend/params.c

libcapa_backend_la_LDFLAGS = \
	$(GLIB_LIBS) \
	$(GPHOTO2_LIBS) \
	$(WARNING_CFLAGS)

libcapa_backend_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GPHOTO2_CFLAGS) \
	$(WARNING_CFLAGS)



libcapa_frontend_la_SOURCES = \
	frontend/app-display.h frontend/app-display.c \
	frontend/camera-info.h frontend/camera-info.c \
	frontend/camera-manager.h frontend/camera-manager.c \
	frontend/camera-progress.h frontend/camera-progress.c \
	frontend/camera-picker.h frontend/camera-picker.c \
	frontend/image-display.h frontend/image-display.c \
	frontend/help-about.h frontend/help-about.c

libcapa_frontend_la_LIBADD = libcapa_backend.la

libcapa_frontend_la_LDFLAGS = \
	$(GTHREAD_LIBS) \
	$(GTK_LIBS) \
	$(GLADE2_LIBS) \
	$(WARNING_CFLAGS)

libcapa_frontend_la_CFLAGS = \
	-I$(srcdir)/backend \
	$(GTHREAD_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLADE2_CFLAGS) \
	$(WARNING_CFLAGS)



capa_SOURCES = main.c

capa_LDADD = libcapa_frontend.la

capa_LDFLAGS = \
	$(GTHREAD_LIBS) \
	$(GTK_LIBS) \
	$(WARNING_CFLAGS)

capa_CFLAGS = \
	-I$(srcdir)/backend \
	-I$(srcdir)/frontend \
	$(GTHREAD_FLAGSS) \
	$(GTK_CFLAGS) \
	$(WARNING_CFLAGS)


CLEANFILES = *~ \\\#*

-include $(top_srcdir)/git.mk
