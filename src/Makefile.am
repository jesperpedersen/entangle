

noinst_LTLIBRARIES = libentangle_backend.la libentangle_frontend.la

bin_PROGRAMS = entangle

gladedir = $(pkgdatadir)
glade_DATA = entangle.glade

desktopdir = $(datadir)/applications
desktop_DATA_IN = entangle.desktop.in
desktop_DATA = entangle.desktop

icondir = $(pkgdatadir)
icon_DATA = $(PACKAGE).svg $(wildcard *.png)

iccdir = $(pkgdatadir)
icc_DATA = sRGB.icc

BUILT_SOURCES = \
	backend/entangle-colour-profile-enums.h backend/entangle-colour-profile-enums.c \
	backend/entangle-config-entry-enums.h backend/entangle-config-entry-enums.c

EXTRA_DIST = $(glade_DATA) $(desktop_DATA_IN) $(icon_DATA) $(icc_DATA)

libentangle_backend_la_SOURCES = \
	backend/entangle-app.h backend/entangle-app.c \
	backend/entangle-camera.h backend/entangle-camera.c \
	backend/entangle-camera-file.h backend/entangle-camera-file.c \
	backend/entangle-camera-task.h backend/entangle-camera-task.c \
	backend/entangle-camera-task-capture.h backend/entangle-camera-task-capture.c \
	backend/entangle-camera-task-preview.h backend/entangle-camera-task-preview.c \
	backend/entangle-camera-task-monitor.h backend/entangle-camera-task-monitor.c \
	backend/entangle-camera-scheduler.h backend/entangle-camera-scheduler.c \
	backend/entangle-camera-list.h backend/entangle-camera-list.c \
	backend/entangle-cancellable.h backend/entangle-cancellable.c \
	backend/entangle-colour-profile.h backend/entangle-colour-profile.c \
	backend/entangle-colour-profile-enums.h backend/entangle-colour-profile-enums.c \
	backend/entangle-confirmable.h backend/entangle-confirmable.c \
	backend/entangle-control.h backend/entangle-control.c \
	backend/entangle-control-button.h backend/entangle-control-button.c \
	backend/entangle-control-choice.h backend/entangle-control-choice.c \
	backend/entangle-control-date.h backend/entangle-control-date.c \
	backend/entangle-control-group.h backend/entangle-control-group.c \
	backend/entangle-control-range.h backend/entangle-control-range.c \
	backend/entangle-control-text.h backend/entangle-control-text.c \
	backend/entangle-control-toggle.h backend/entangle-control-toggle.c \
	backend/entangle-config-entry.h backend/entangle-config-entry.c \
	backend/entangle-config-entry-enums.h backend/entangle-config-entry-enums.c \
	backend/entangle-config-set.h backend/entangle-config-set.c \
	backend/entangle-config-store.h backend/entangle-config-store.c \
	backend/entangle-config-store-gconf.h backend/entangle-config-store-gconf.c \
	backend/entangle-debug.h backend/entangle-debug.c \
	backend/entangle-device-manager.h backend/entangle-device-manager.c \
	backend/entangle-image.h backend/entangle-image.c \
	backend/entangle-image-loader.h backend/entangle-image-loader.c \
	backend/entangle-params.h backend/entangle-params.c \
	backend/entangle-pixbuf-loader.h backend/entangle-pixbuf-loader.c \
	backend/entangle-preferences.h backend/entangle-preferences.c \
	backend/entangle-preferences-gconf.h backend/entangle-preferences-gconf.c \
	backend/entangle-progress.h backend/entangle-progress.c \
	backend/entangle-session.h backend/entangle-session.c \
	backend/entangle-thumbnail-loader.h backend/entangle-thumbnail-loader.c

libentangle_backend_la_LDFLAGS = \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(GMODULE_LIBS) \
	$(GTHREAD_LIBS) \
	$(GDK_PIXBUF_LIBS) \
	$(GPHOTO2_LIBS) \
	$(GCONF2_LIBS) \
	$(LCMS_LIBS) \
	$(HAL_LIBS) \
	$(GUDEV_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(PEAS_LIBS) \
	$(WARNING_CFLAGS)

libentangle_backend_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS) \
	$(GPHOTO2_CFLAGS) \
	$(GCONF2_CFLAGS) \
	$(LCMS_CFLAGS) \
	$(HAL_CFLAGS) \
	$(GUDEV_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(PEAS_CFLAGS) \
	$(WARNING_CFLAGS) \
	-DPKGDATADIR="\"$(pkgdatadir)\"" \
	-DDATADIR="\"$(datadir)\"" \
	-DLIBDIR="\"$(libdir)\""

libentangle_frontend_la_SOURCES = \
	frontend/entangle-app-display.h frontend/entangle-app-display.c \
	frontend/entangle-camera-info.h frontend/entangle-camera-info.c \
	frontend/entangle-camera-manager.h frontend/entangle-camera-manager.c \
	frontend/entangle-camera-picker.h frontend/entangle-camera-picker.c \
	frontend/entangle-control-panel.h frontend/entangle-control-panel.c \
	frontend/entangle-help-about.h frontend/entangle-help-about.c \
	frontend/entangle-image-display.h frontend/entangle-image-display.c \
	frontend/entangle-image-popup.h frontend/entangle-image-popup.c \
	frontend/entangle-preferences-display.h frontend/entangle-preferences-display.c \
	frontend/entangle-session-browser.h frontend/entangle-session-browser.c

libentangle_frontend_la_LIBADD = libentangle_backend.la

libentangle_frontend_la_LDFLAGS = \
	$(GTHREAD_LIBS) \
	$(GMODULE_LIBS) \
	$(GTK_LIBS) \
	$(GLADE2_LIBS) \
	$(UNIQUE_LIBS) \
	$(PEAS_LIBS) \
	$(PEAS_UI_LIBS) \
	$(WARNING_CFLAGS)

libentangle_frontend_la_CFLAGS = \
	-I$(srcdir)/backend \
	$(GTHREAD_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GTK_CFLAGS) \
	$(GLADE2_CFLAGS) \
	$(UNIQUE_CFLAGS) \
	$(PEAS_CFLAGS) \
	$(PEAS_UI_CFLAGS) \
	$(WARNING_CFLAGS) \
	-DPKGDATADIR="\"$(pkgdatadir)\""



entangle_SOURCES = entangle-main.c

entangle_LDADD = libentangle_frontend.la

entangle_LDFLAGS = \
	$(GTHREAD_LIBS) \
	$(GMODULE_LIBS) \
	$(GTK_LIBS) \
	$(GOBJECT_INTROSPECTION_LIBS) \
	$(PEAS_LIBS) \
	$(STARTUP_LIBS) \
	$(WARNING_CFLAGS)

entangle_CFLAGS = \
	-I$(srcdir)/backend \
	-I$(srcdir)/frontend \
	$(GTHREAD_FLAGS) \
	$(GMODULE_FLAGS) \
	$(GTK_CFLAGS) \
	$(GOBJECT_INTROSPECTION_CFLAGS) \
	$(PEAS_CFLAGS) \
	$(STARTUP_CFLAGS) \
	$(WARNING_CFLAGS)


%.desktop: $(srcdir)/%.desktop.in
	$(AM_V_GEN)sed -e "s,::PACKAGE::,$(PACKAGE),g" -e "s,::DATADIR::,$(datadir),g" < $< > $@

backend/entangle-colour-profile-enums.c: backend/entangle-colour-profile.h backend/entangle-colour-profile-enums.h Makefile.am
	$(AM_V_GEN)glib-mkenums \
			--fhead "#include \"entangle-colour-profile-enums.h\"\n\n" \
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

backend/entangle-colour-profile-enums.h: backend/entangle-colour-profile.h Makefile.am
	$(AM_V_GEN)glib-mkenums \
			--fhead "#ifndef __ENTANGLE_COLOUR_PROFILE_ENUMS_H__\n" \
                        --fhead "#define __ENTANGLE_COLOUR_PROFILE_ENUMS_H__\n\n" \
                        --fhead "#include <glib-object.h>\n" \
                        --fhead "G_BEGIN_DECLS\n\n" \
                        --ftail "G_END_DECLS\n\n" \
                        --ftail "#endif /* __ENTANGLE_COLOUR_PROFILE_ENUMS_H__ */\n" \
                        --fprod "\n/* --- @filename@ --- */" \
                        --eprod "#define ENTANGLE_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
                        --eprod "GType @enum_name@_get_type (void);\n" \
                $< >  $@

backend/entangle-config-entry-enums.c: backend/entangle-config-entry.h backend/entangle-config-entry-enums.h Makefile.am
	$(AM_V_GEN)glib-mkenums \
			--fhead "#include \"entangle-config-entry-enums.h\"\n\n" \
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

backend/entangle-config-entry-enums.h: backend/entangle-config-entry.h Makefile.am
	$(AM_V_GEN)glib-mkenums \
			--fhead "#ifndef __ENTANGLE_COLOUR_PROFILE_ENUMS_H__\n" \
                        --fhead "#define __ENTANGLE_COLOUR_PROFILE_ENUMS_H__\n\n" \
                        --fhead "#include <glib-object.h>\n" \
                        --fhead "G_BEGIN_DECLS\n\n" \
                        --ftail "G_END_DECLS\n\n" \
                        --ftail "#endif /* __ENTANGLE_COLOUR_PROFILE_ENUMS_H__ */\n" \
                        --fprod "\n/* --- @filename@ --- */" \
                        --eprod "#define ENTANGLE_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
                        --eprod "GType @enum_name@_get_type (void);\n" \
                $< >  $@

CLEANFILES = *~ $(desktop_DATA) $(BUILT_SOURCES)

if HAVE_PLUGINS
PLUGIN_INC=--include Peas-1.0
PLUGIN_PKG=--pkg=libpeas-1.0

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(libdir)/entangle/plugins
	$(MKDIR_P) $(DESTDIR)$(datadir)/entangle/plugins

uninstall-local::
	rmdir $(DESTDIR)$(libdir)/entangle/plugins ||:
	rmdir $(DESTDIR)$(libdir)/entangle ||:
	rmdir $(DESTDIR)$(datadir)/entangle/plugins ||:
	rmdir $(DESTDIR)$(datadir)/entangle ||:

else
PLUGIN_INC=
PLUGIN_PKG=
endif

if WITH_GOBJECT_INTROSPECTION
Entangle-0.1.gir: entangle GPhoto-2.0.typelib $(G_IR_SCANNER) Makefile.am
	$(G_IR_SCANNER) -v \
                --namespace Entangle \
                --nsversion 0.1 \
		--include GObject-2.0 \
		--include Gtk-2.0 \
                --include GPhoto-2.0 \
                --include GPhoto-2.0 \
		$(PLUGIN_INC) \
                --program=$(builddir)/entangle \
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
                $(PLUGIN_PKG) \
                --pkg=dbus-glib-1 \
                $(libentangle_backend_la_SOURCES:%=$(srcdir)/%) \
                $(libentangle_frontend_la_SOURCES:%=$(srcdir)/%) \
                $(entangle_SOURCES:%=$(srcdir)/%)

girdir = $(datadir)/gir-1.0
gir_DATA = GPhoto-2.0.gir Entangle-0.1.gir

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = GPhoto-2.0.typelib Entangle-0.1.typelib

%.typelib: %.gir
	g-ir-compiler \
		--includedir=$(srcdir) \
		--includedir=$(builddir) \
		-o $@ $<

EXTRA_DIST += GPhoto-2.0.gir
CLEANFILES += Entangle-0.1.gir $(typelibs_DATA)

endif # WITH_GOBJECT_INTROSPECTION

-include $(top_srcdir)/git.mk
