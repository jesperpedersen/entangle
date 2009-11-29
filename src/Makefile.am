

noinst_LTLIBRARIES = libcapa_backend.la libcapa_frontend.la

bin_PROGRAMS = capa

gladedir = $(pkgdatadir)
glade_DATA = capa.glade

desktopdir = $(datadir)/applications
desktop_DATA_IN = capa.desktop.in
desktop_DATA = capa.desktop

icondir = $(pkgdatadir)
icon_DATA = $(PACKAGE).svg $(wildcard *.png)

iccdir = $(pkgdatadir)
icc_DATA = sRGB.icc

EXTRA_DIST = $(glade_DATA) $(desktop_DATA_IN) $(icon_DATA) $(icc_DATA)

libcapa_backend_la_SOURCES = \
	backend/app.h backend/app.c \
	backend/camera.h backend/camera.c \
	backend/camera-list.h backend/camera-list.c \
	backend/colour-profile.h backend/colour-profile.c \
	backend/colour-profile-enums.h backend/colour-profile-enums.c \
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
	backend/image-loader.h backend/image-loader.c \
	backend/params.h backend/params.c \
	backend/preferences.h backend/preferences.c \
	backend/preferences-gconf.h backend/preferences-gconf.c \
	backend/progress.h backend/progress.c \
	backend/session.h backend/session.c

libcapa_backend_la_LDFLAGS = \
	$(GLIB_LIBS) \
	$(GDK_PIXBUF_LIBS) \
	$(GPHOTO2_LIBS) \
	$(GCONF2_LIBS) \
	$(LCMS_LIBS) \
	$(HAL_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(WARNING_CFLAGS)

libcapa_backend_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS) \
	$(GPHOTO2_CFLAGS) \
	$(GCONF2_CFLAGS) \
	$(LCMS_CFLAGS) \
	$(HAL_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(WARNING_CFLAGS) \
	-DPKGDATADIR="\"$(pkgdatadir)\""



libcapa_frontend_la_SOURCES = \
	frontend/app-display.h frontend/app-display.c \
	frontend/camera-info.h frontend/camera-info.c \
	frontend/camera-manager.h frontend/camera-manager.c \
	frontend/camera-progress.h frontend/camera-progress.c \
	frontend/camera-picker.h frontend/camera-picker.c \
	frontend/control-panel.h frontend/control-panel.c \
	frontend/help-about.h frontend/help-about.c \
	frontend/image-display.h frontend/image-display.c \
	frontend/image-polaroid.h frontend/image-polaroid.c \
	frontend/preferences-display.h frontend/preferences-display.c \
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
	$(GOBJECT_INTROSPECTION_LIBS) \
	$(WARNING_CFLAGS)

capa_CFLAGS = \
	-I$(srcdir)/backend \
	-I$(srcdir)/frontend \
	$(GTHREAD_FLAGSS) \
	$(GTK_CFLAGS) \
	$(GOBJECT_INTROSPECTION_CFLAGS) \
	$(WARNING_CFLAGS)


%.desktop: $(srcdir)/%.desktop.in
	sed -e "s,::PACKAGE::,$(PACKAGE),g" -e "s,::DATADIR::,$(datadir),g" < $< > $@

backend/colour-profile-enums.c: backend/colour-profile.h backend/colour-profile-enums.h
	glib-mkenums    --fhead "#include <glib-object.h>\n" \
                        --fhead "#include \"colour-profile-enums.h\"\n\n" \
                        --fprod "\n/* enumerations from \"@filename@\" */" \
                        --fprod "\n#include \"@filename@\"\n" \
                        --vhead "static const G@Type@Value _@enum_name@_values[] = {" \
                        --vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
                        --vtail "  { 0, NULL, NULL }\n};\n\n" \
                        --vtail "GType\n@enum_name@_get_type (void)\n{\n" \
                        --vtail "  static GType type = 0;\n\n" \
                        --vtail "  if (!type)\n" \
                        --vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
                        --vtail "  return type;\n}\n\n" \
                $< > $@

backend/colour-profile-enums.h: backend/colour-profile.h
	glib-mkenums    --fhead "#ifndef __CAPA_COLOUR_PROFILE_ENUMS_H__\n" \
                        --fhead "#define __CAPA_COLOUR_PROFILE_ENUMS_H__\n\n" \
                        --fhead "G_BEGIN_DECLS\n\n" \
                        --ftail "G_END_DECLS\n\n" \
                        --ftail "#endif /* __CAPA_COLOUR_PROFILE_ENUMS_H__ */\n" \
                        --fprod "\n/* --- @filename@ --- */" \
                        --eprod "#define CAPA_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
                        --eprod "GType @enum_name@_get_type (void);\n" \
                $< >  $@

CLEANFILES = *~ $(desktop_DATA) backend/colour-profile-enums.c backend/colour-profile-enums.h




if WITH_GOBJECT_INTROSPECTION
capa-0.1.gir: capa gphoto-2.0.typelib $(G_IR_SCANNER) Makefile.am
	$(G_IR_SCANNER) -v \
                --namespace capa \
                --nsversion 0.1 \
		--include GObject-2.0 \
		--include Gtk-2.0 \
                --include gphoto-2.0 \
                --program=$(builddir)/capa \
                --add-include-path=$(srcdir) \
                --add-include-path=$(builddir) \
                --output $@ \
                -I$(srcdir)/backend \
                -I$(srcdir)/frontend \
		--verbose \
                --pkg=glib-2.0 \
                --pkg=gthread-2.0 \
                --pkg=gdk-pixbuf-2.0 \
                --pkg=gobject-2.0 \
                --pkg=gtk+-2.0 \
                --pkg=libgphoto2 \
                --pkg=libglade-2.0 \
                --pkg=hal \
                --pkg=dbus-glib-1 \
                $(libcapa_backend_la_SOURCES:%=$(srcdir)/%) \
                $(libcapa_frontend_la_SOURCES:%=$(srcdir)/%) \
                $(capa_SOURCES:%=$(srcdir)/%)

girdir = $(datadir)/gir-1.0
gir_DATA = gphoto-2.0.gir capa-0.1.gir

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = gphoto-2.0.typelib capa-0.1.typelib

%.typelib: %.gir
	g-ir-compiler \
		--includedir=$(srcdir) \
		--includedir=$(builddir) \
		-o $@ $<

EXTRA_DIST += gphoto-2.0.gir
CLEANFILES += capa-0.1.gir $(typelibs_DATA)

endif # WITH_GOBJECT_INTROSPECTION

-include $(top_srcdir)/git.mk
