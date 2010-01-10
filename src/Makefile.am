

noinst_LTLIBRARIES = libcapa_backend.la libcapa_frontend.la

if WITH_JAVASCRIPT
plugin_LTLIBRARIES = javascript.la
plugindir = $(libdir)/capa/plugins
endif # WITH_JAVASCRIPT

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
	backend/capa-colour-profile-enums.h backend/capa-colour-profile-enums.c \
	backend/capa-app.h backend/capa-app.c \
	backend/capa-camera.h backend/capa-camera.c \
	backend/capa-camera-list.h backend/capa-camera-list.c \
	backend/capa-colour-profile.h backend/capa-colour-profile.c \
	backend/capa-control.h backend/capa-control.c \
	backend/capa-control-button.h backend/capa-control-button.c \
	backend/capa-control-choice.h backend/capa-control-choice.c \
	backend/capa-control-date.h backend/capa-control-date.c \
	backend/capa-control-group.h backend/capa-control-group.c \
	backend/capa-control-range.h backend/capa-control-range.c \
	backend/capa-control-text.h backend/capa-control-text.c \
	backend/capa-control-toggle.h backend/capa-control-toggle.c \
	backend/capa-debug.h backend/capa-debug.c \
	backend/capa-device-manager.h backend/capa-device-manager.c \
	backend/capa-image.h backend/capa-image.c \
	backend/capa-image-loader.h backend/capa-image-loader.c \
	backend/capa-params.h backend/capa-params.c \
	backend/capa-pixbuf-loader.h backend/capa-pixbuf-loader.c \
	backend/capa-plugin.h backend/capa-plugin.c \
	backend/capa-plugin-native.h backend/capa-plugin-native.c \
	backend/capa-plugin-manager.h backend/capa-plugin-manager.c \
	backend/capa-preferences.h backend/capa-preferences.c \
	backend/capa-preferences-gconf.h backend/capa-preferences-gconf.c \
	backend/capa-progress.h backend/capa-progress.c \
	backend/capa-session.h backend/capa-session.c \
	backend/capa-thumbnail-loader.h backend/capa-thumbnail-loader.c

libcapa_backend_la_LDFLAGS = \
	$(GLIB_LIBS) \
	$(GMODULE_LIBS) \
	$(GTHREAD_LIBS) \
	$(GDK_PIXBUF_LIBS) \
	$(GPHOTO2_LIBS) \
	$(GCONF2_LIBS) \
	$(LCMS_LIBS) \
	$(HAL_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(WARNING_CFLAGS)

libcapa_backend_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS) \
	$(GPHOTO2_CFLAGS) \
	$(GCONF2_CFLAGS) \
	$(LCMS_CFLAGS) \
	$(HAL_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(WARNING_CFLAGS) \
	-DPKGDATADIR="\"$(pkgdatadir)\"" \
	-DLIBDIR="\"$(libdir)\""

if WITH_JAVASCRIPT
javascript_la_SOURCES = \
	backend/capa-plugin-javascript.h backend/capa-plugin-javascript.c \
	backend/capa-plugin-javascript-module.c

javascript_la_LDFLAGS = \
	-avoid-version -module \
	$(GJS_LIBS) \
	$(GJS_GI_LIBS) \
	$(WARNING_CFLAGS)

javascript_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS) \
	$(GJS_CFLAGS) \
	$(GJS_GI_CFLAGS) \
	$(WARNING_CFLAGS)

EXTRA_DIST += javascript.cfg

install-data-local:
	install -d $(pkgdatadir)/plugins
	install -d $(pkgdatadir)/plugins/javascript
	install $(srcdir)/javascript.cfg $(pkgdatadir)/plugins/javascript/plugin.cfg

uninstall-local:
	rm -f $(pkgdatadir)/plugins/javascript/plugin.cfg
else # WITH_JAVASCRIPT
EXTRA_DIST += \
	javascript.cfg \
	backend/capa-plugin-javascript.h backend/capa-plugin-javascript.c \
	backend/capa-plugin-javascript-module.c
endif # WITH_JAVASCRIPT

libcapa_frontend_la_SOURCES = \
	frontend/capa-app-display.h frontend/capa-app-display.c \
	frontend/capa-camera-info.h frontend/capa-camera-info.c \
	frontend/capa-camera-manager.h frontend/capa-camera-manager.c \
	frontend/capa-camera-picker.h frontend/capa-camera-picker.c \
	frontend/capa-control-panel.h frontend/capa-control-panel.c \
	frontend/capa-help-about.h frontend/capa-help-about.c \
	frontend/capa-image-display.h frontend/capa-image-display.c \
	frontend/capa-image-polaroid.h frontend/capa-image-polaroid.c \
	frontend/capa-preferences-display.h frontend/capa-preferences-display.c \
	frontend/capa-session-browser.h frontend/capa-session-browser.c

libcapa_frontend_la_LIBADD = libcapa_backend.la

libcapa_frontend_la_LDFLAGS = \
	$(GTHREAD_LIBS) \
	$(GMODULE_LIBS) \
	$(GTK_LIBS) \
	$(GLADE2_LIBS) \
	$(UNIQUE_LIBS) \
	$(WARNING_CFLAGS)

libcapa_frontend_la_CFLAGS = \
	-I$(srcdir)/backend \
	$(GTHREAD_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLADE2_CFLAGS) \
	$(UNIQUE_CFLAGS) \
	$(WARNING_CFLAGS) \
	-DPKGDATADIR="\"$(pkgdatadir)\""



capa_SOURCES = capa-main.c

capa_LDADD = libcapa_frontend.la

capa_LDFLAGS = \
	$(GTHREAD_LIBS) \
	$(GMODULE_LIBS) \
	$(GTK_LIBS) \
	$(GOBJECT_INTROSPECTION_LIBS) \
	$(STARTUP_LIBS) \
	$(WARNING_CFLAGS)

capa_CFLAGS = \
	-I$(srcdir)/backend \
	-I$(srcdir)/frontend \
	$(GTHREAD_FLAGS) \
	$(GMODULE_FLAGS) \
	$(GTK_CFLAGS) \
	$(GOBJECT_INTROSPECTION_CFLAGS) \
	$(STARTUP_CFLAGS) \
	$(WARNING_CFLAGS)


%.desktop: $(srcdir)/%.desktop.in
	sed -e "s,::PACKAGE::,$(PACKAGE),g" -e "s,::DATADIR::,$(datadir),g" < $< > $@

backend/capa-colour-profile-enums.c: backend/capa-colour-profile.h backend/capa-colour-profile-enums.h Makefile.am
	$(AM_V_GEN)glib-mkenums    --fhead "#include \"capa-colour-profile-enums.h\"\n\n" \
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

backend/capa-colour-profile-enums.h: backend/capa-colour-profile.h Makefile.am
	$(AM_V_GEN)glib-mkenums    --fhead "#ifndef __CAPA_COLOUR_PROFILE_ENUMS_H__\n" \
                        --fhead "#define __CAPA_COLOUR_PROFILE_ENUMS_H__\n\n" \
                        --fhead "#include <glib-object.h>\n" \
                        --fhead "G_BEGIN_DECLS\n\n" \
                        --ftail "G_END_DECLS\n\n" \
                        --ftail "#endif /* __CAPA_COLOUR_PROFILE_ENUMS_H__ */\n" \
                        --fprod "\n/* --- @filename@ --- */" \
                        --eprod "#define CAPA_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
                        --eprod "GType @enum_name@_get_type (void);\n" \
                $< >  $@

CLEANFILES = *~ $(desktop_DATA) backend/capa-colour-profile-enums.c backend/capa-colour-profile-enums.h




if WITH_GOBJECT_INTROSPECTION
Capa-0.1.gir: capa GPhoto-2.0.typelib $(G_IR_SCANNER) Makefile.am
	$(G_IR_SCANNER) -v \
                --namespace Capa \
                --nsversion 0.1 \
		--include GObject-2.0 \
		--include Gtk-2.0 \
                --include GPhoto-2.0 \
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
gir_DATA = GPhoto-2.0.gir Capa-0.1.gir

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = GPhoto-2.0.typelib Capa-0.1.typelib

%.typelib: %.gir
	g-ir-compiler \
		--includedir=$(srcdir) \
		--includedir=$(builddir) \
		-o $@ $<

EXTRA_DIST += GPhoto-2.0.gir
CLEANFILES += Capa-0.1.gir $(typelibs_DATA)

endif # WITH_GOBJECT_INTROSPECTION

-include $(top_srcdir)/git.mk
