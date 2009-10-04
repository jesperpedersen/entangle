

noinst_LTLIBRARIES = libcapa_backend.la libcapa_frontend.la

bin_PROGRAMS = capa

gladedir = $(pkgdatadir)
glade_DATA = capa.glade

desktopdir = $(datadir)/applications
desktop_DATA_IN = capa.desktop.in
desktop_DATA = capa.desktop

icondir = $(pkgdatadir)
icon_DATA = $(PACKAGE).svg $(PACKAGE).png

EXTRA_DIST = $(glade_DATA) $(desktop_DATA_IN) $(icon_DATA)

libcapa_backend_la_SOURCES = \
	backend/app.h backend/app.c \
	backend/camera.h backend/camera.c \
	backend/camera-list.h backend/camera-list.c \
	backend/control.h backend/control.c \
	backend/control-button.h backend/control-button.c \
	backend/control-choice.h backend/control-choice.c \
	backend/control-date.h backend/control-date.c \
	backend/control-group.h backend/control-group.c \
	backend/control-range.h backend/control-range.c \
	backend/control-text.h backend/control-text.c \
	backend/control-toggle.h backend/control-toggle.c \
	backend/device-manager.h backend/device-manager.c \
	backend/image.h backend/image.c \
	backend/params.h backend/params.c \
	backend/preferences.h backend/preferences.c \
	backend/progress.h backend/progress.c \
	backend/session.h backend/session.c

libcapa_backend_la_LDFLAGS = \
	$(GLIB_LIBS) \
	$(GDK_PIXBUF_LIBS) \
	$(GPHOTO2_LIBS) \
	$(HAL_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(WARNING_CFLAGS)

libcapa_backend_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS) \
	$(GPHOTO2_CFLAGS) \
	$(HAL_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(WARNING_CFLAGS)



libcapa_frontend_la_SOURCES = \
	frontend/app-display.h frontend/app-display.c \
	frontend/camera-info.h frontend/camera-info.c \
	frontend/camera-manager.h frontend/camera-manager.c \
	frontend/camera-progress.h frontend/camera-progress.c \
	frontend/camera-picker.h frontend/camera-picker.c \
	frontend/control-panel.h frontend/control-panel.c \
	frontend/help-about.h frontend/help-about.c \
	frontend/image-display.h frontend/image-display.c \
	frontend/session-browser.h frontend/session-browser.c

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
	$(WARNING_CFLAGS) \
	-DPKGDATADIR="\"$(pkgdatadir)\""



capa_SOURCES = main.c internal.h

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


%.desktop: $(srcdir)/%.desktop.in
	sed -e "s,::PACKAGE::,$(PACKAGE),g" -e "s,::DATADIR::,$(datadir),g" < $< > $@


CLEANFILES = *~ \\\#* $(desktop_DATA)

-include $(top_srcdir)/git.mk
