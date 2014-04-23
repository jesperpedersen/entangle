
SUBDIRS = plugins

noinst_LTLIBRARIES = libentangle_backend.la libentangle_frontend.la

bin_PROGRAMS = entangle

builderdir = $(pkgdatadir)
builder_DATA = \
	frontend/entangle-camera-support.ui \
	frontend/entangle-camera-manager.ui \
	frontend/entangle-camera-picker.ui \
	frontend/entangle-help-about.ui \
	frontend/entangle-image-popup.ui \
	frontend/entangle-preferences-display.ui

desktopdir = $(datadir)/applications
desktop_DATA_IN = entangle.desktop.in
desktop_DATA = entangle.desktop

appdatadir = $(datadir)/appdata
appdata_DATA_IN = entangle.appdata.xml.in
appdata_DATA = $(appdata_DATA_IN:.xml.in=.xml)

icon128x128dir = $(datadir)/icons/hicolor/128x128/apps
icon128x128_DATA = $(wildcard icons/128x128/*.png)

icon16x16dir = $(datadir)/icons/hicolor/16x16/apps
icon16x16_DATA = $(wildcard icons/16x16/*.png)

icon22x22dir = $(datadir)/icons/hicolor/22x22/apps
icon22x22_DATA = $(wildcard icons/22x22/*.png)

icon256x256dir = $(datadir)/icons/hicolor/256x256/apps
icon256x256_DATA = $(wildcard icons/256x256/*.png)

icon32x32dir = $(datadir)/icons/hicolor/32x32/apps
icon32x32_DATA = $(wildcard icons/32x32/*.png)

icon48x48dir = $(datadir)/icons/hicolor/48x48/apps
icon48x48_DATA = $(wildcard icons/48x48/*.png)

icon64x64dir = $(datadir)/icons/hicolor/64x64/apps
icon64x64_DATA = $(wildcard icons/64x64/*.png)

iconscalabledir = $(datadir)/icons/hicolor/scalable/apps
iconscalable_DATA = $(wildcard icons/scalable/*.png)



gsettings_SCHEMAS = org.entangle-photo.manager.gschema.xml

@GSETTINGS_RULES@

iccdir = $(pkgdatadir)
icc_DATA = sRGB.icc

BUILT_ENUMS = \
	backend/entangle-colour-profile-enums.h \
	backend/entangle-colour-profile-enums.c \
	backend/entangle-camera-enums.h \
	backend/entangle-camera-enums.c \
	frontend/entangle-image-display-enums.h \
	frontend/entangle-image-display-enums.c \
	$(NULL)

BUILT_SOURCES = \
	$(desktop_DATA) \
	$(appdata_DATA) \
	$(BUILT_ENUMS) \
	$(NULL)

EXTRA_DIST = \
	$(builder_DATA) \
	$(desktop_DATA) \
	$(desktop_DATA_IN) \
	$(appdata_DATA) \
	$(appdata_DATA_IN) \
	$(icon128x128_DATA) \
	$(icon16x16_DATA) \
	$(icon22x22_DATA) \
	$(icon256x256_DATA) \
	$(icon32x32_DATA) \
	$(icon48x48_DATA) \
	$(icon64x64_DATA) \
	$(iconscalable_DATA) \
	$(icc_DATA) \
	$(gsettings_SCHEMAS) \
	$(NULL)

libentangle_backend_la_SOURCES = \
	backend/entangle-camera.h backend/entangle-camera.c \
	backend/entangle-camera-enums.h backend/entangle-camera-enums.c \
	backend/entangle-camera-file.h backend/entangle-camera-file.c \
	backend/entangle-camera-list.h backend/entangle-camera-list.c \
	backend/entangle-colour-profile.h backend/entangle-colour-profile.c \
	backend/entangle-colour-profile-enums.h backend/entangle-colour-profile-enums.c \
	backend/entangle-control.h backend/entangle-control.c \
	backend/entangle-control-button.h backend/entangle-control-button.c \
	backend/entangle-control-choice.h backend/entangle-control-choice.c \
	backend/entangle-control-date.h backend/entangle-control-date.c \
	backend/entangle-control-group.h backend/entangle-control-group.c \
	backend/entangle-control-range.h backend/entangle-control-range.c \
	backend/entangle-control-text.h backend/entangle-control-text.c \
	backend/entangle-control-toggle.h backend/entangle-control-toggle.c \
	backend/entangle-debug.h backend/entangle-debug.c \
	backend/entangle-device-manager.h backend/entangle-device-manager.c \
	backend/entangle-image.h backend/entangle-image.c \
	backend/entangle-pixbuf.h backend/entangle-pixbuf.c \
	backend/entangle-image-loader.h backend/entangle-image-loader.c \
	backend/entangle-pixbuf-loader.h backend/entangle-pixbuf-loader.c \
	backend/entangle-preferences.h backend/entangle-preferences.c \
	backend/entangle-progress.h backend/entangle-progress.c \
	backend/entangle-session.h backend/entangle-session.c \
	backend/entangle-thumbnail-loader.h backend/entangle-thumbnail-loader.c

libentangle_backend_la_LDFLAGS = \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(GTHREAD_LIBS) \
	$(GDK_PIXBUF_LIBS) \
	$(GPHOTO2_LIBS) \
	$(LCMS2_LIBS) \
	$(GUDEV_LIBS) \
	$(DBUS_GLIB_LIBS) \
	$(LIBPEAS_LIBS) \
	$(GEXIV2_LIBS) \
	$(LIBRAW_LIBS) \
	$(WARN_LDFLAGS)

libentangle_backend_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS) \
	$(GPHOTO2_CFLAGS) \
	$(LCMS2_CFLAGS) \
	$(GUDEV_CFLAGS) \
	$(DBUS_GLIB_CFLAGS) \
	$(LIBPEAS_CFLAGS) \
	$(GEXIV2_CFLAGS) \
	$(LIBRAW_CFLAGS) \
	$(WARN_CFLAGS) \
	-DPKGDATADIR="\"$(pkgdatadir)\"" \
	-DDATADIR="\"$(datadir)\"" \
	-DLIBDIR="\"$(libdir)\"" \
	-DG_LOG_DOMAIN="\"$(PACKAGE)\"" \
	$(NULL)

libentangle_frontend_la_SOURCES = \
	frontend/entangle-window.h frontend/entangle-window.c \
	frontend/entangle-application.h frontend/entangle-application.c \
	frontend/entangle-camera-support.h frontend/entangle-camera-support.c \
	frontend/entangle-camera-manager.h frontend/entangle-camera-manager.c \
	frontend/entangle-camera-picker.h frontend/entangle-camera-picker.c \
	frontend/entangle-control-panel.h frontend/entangle-control-panel.c \
        frontend/entangle-dpms.h frontend/entangle-dpms.c \
	frontend/entangle-help-about.h frontend/entangle-help-about.c \
	frontend/entangle-image-display.h frontend/entangle-image-display.c \
	frontend/entangle-image-display-enums.h \
	frontend/entangle-image-display-enums.c \
	frontend/entangle-image-histogram.h frontend/entangle-image-histogram.c \
	frontend/entangle-image-popup.h frontend/entangle-image-popup.c \
	frontend/entangle-image-statusbar.h frontend/entangle-image-statusbar.c \
	frontend/entangle-preferences-display.h frontend/entangle-preferences-display.c \
	frontend/entangle-session-browser.h frontend/entangle-session-browser.c \
	frontend/entangle-overlay-box.h frontend/entangle-overlay-box.c \
	frontend/entangle-auto-drawer.h frontend/entangle-auto-drawer.c \
	frontend/entangle-drawer.h frontend/entangle-drawer.c

libentangle_frontend_la_LIBADD = libentangle_backend.la

libentangle_frontend_la_LDFLAGS = \
	$(GTHREAD_LIBS) \
	$(GMODULE_LIBS) \
	$(GTK_LIBS) \
	$(LIBPEAS_LIBS) \
	$(LIBPEAS_UI_LIBS) \
	$(GEXIV2_LIBS) \
	$(XEXT_LIBS) \
	$(WARN_LDFLAGS)

libentangle_frontend_la_CFLAGS = \
	-I$(srcdir)/backend \
	$(GTHREAD_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GTK_CFLAGS) \
	$(LIBPEAS_CFLAGS) \
	$(LIBPEAS_UI_CFLAGS) \
	$(GEXIV2_CFLAGS) \
	$(XEXT_CFLAGS) \
	$(WARN_CFLAGS) \
	-DPKGDATADIR="\"$(pkgdatadir)\"" \
	-DDATADIR="\"$(datadir)\"" \
	-DLIBDIR="\"$(libdir)\"" \
	-DG_LOG_DOMAIN="\"$(PACKAGE)\"" \
	$(NULL)



entangle_SOURCES = entangle-main.c

entangle_LDADD = libentangle_frontend.la

entangle_LDFLAGS = \
	$(GTHREAD_LIBS) \
	$(GMODULE_LIBS) \
	$(GTK_LIBS) \
	$(GOBJECT_INTROSPECTION_LIBS) \
	$(LIBPEAS_LIBS) \
	$(WARN_LDFLAGS)

entangle_CFLAGS = \
        -DLOCALE_DIR=\""$(datadir)/locale"\"            \
	-I$(srcdir)/backend \
	-I$(srcdir)/frontend \
	$(GTHREAD_FLAGS) \
	$(GMODULE_FLAGS) \
	$(GTK_CFLAGS) \
	$(GOBJECT_INTROSPECTION_CFLAGS) \
	$(LIBPEAS_CFLAGS) \
	$(WARN_CFLAGS) \
	-DPKGDATADIR="\"$(pkgdatadir)\"" \
	-DDATADIR="\"$(datadir)\"" \
	-DG_LOG_DOMAIN="\"$(PACKAGE)\"" \
	$(NULL)


%.desktop.tmp: $(srcdir)/%.desktop.in
	$(AM_V_GEN)sed -e "s,::DATADIR::,$(datadir),g" < $< > $@

%.desktop: %.desktop.tmp
	$(AM_V_GEN)intltool-merge -d $(top_srcdir)/po $< $@

%.appdata.xml: %.appdata.xml.in
	$(AM_V_GEN)intltool-merge -x $(top_srcdir)/po $< $@

backend/entangle-camera-enums.c: backend/entangle-camera.h backend/entangle-camera-enums.h Makefile.am
	$(AM_V_GEN)glib-mkenums \
			--fhead "#include \"entangle-camera-enums.h\"\n\n" \
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

backend/entangle-camera-enums.h: backend/entangle-camera.h Makefile.am
	$(AM_V_GEN)glib-mkenums \
			--fhead "#ifndef __ENTANGLE_CAMERA_ENUMS_H__\n" \
                        --fhead "#define __ENTANGLE_CAMERA_ENUMS_H__\n\n" \
                        --fhead "#include <glib-object.h>\n" \
                        --fhead "G_BEGIN_DECLS\n\n" \
                        --ftail "G_END_DECLS\n\n" \
                        --ftail "#endif /* __ENTANGLE_CAMERA_ENUMS_H__ */\n" \
                        --fprod "\n/* --- @filename@ --- */" \
                        --eprod "#define ENTANGLE_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
                        --eprod "GType @enum_name@_get_type (void);\n" \
                $< >  $@

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

frontend/entangle-image-display-enums.c: frontend/entangle-image-display.h frontend/entangle-image-display-enums.h Makefile.am
	$(AM_V_GEN)glib-mkenums \
			--fhead "#include \"entangle-image-display-enums.h\"\n\n" \
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

frontend/entangle-image-display-enums.h: frontend/entangle-image-display.h Makefile.am
	$(AM_V_GEN)glib-mkenums \
			--fhead "#ifndef __ENTANGLE_IMAGE_DISPLAY_ENUMS_H__\n" \
                        --fhead "#define __ENTANGLE_IMAGE_DISPLAY_ENUMS_H__\n\n" \
                        --fhead "#include <glib-object.h>\n" \
                        --fhead "G_BEGIN_DECLS\n\n" \
                        --ftail "G_END_DECLS\n\n" \
                        --ftail "#endif /* __ENTANGLE_IMAGE_DISPLAY_ENUMS_H__ */\n" \
                        --fprod "\n/* --- @filename@ --- */" \
                        --eprod "#define ENTANGLE_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
                        --eprod "GType @enum_name@_get_type (void);\n" \
                $< >  $@

CLEANFILES = *~ $(BUILT_ENUMS)

MAINTAINERCLEANFILES = $(desktop_DATA) $(appdata_DATA)

Entangle-0.1.gir: entangle $(G_IR_SCANNER) Makefile.am
	$(AM_V_GEN)$(G_IR_SCANNER) -v --warn-all \
                --namespace Entangle \
                --nsversion 0.1 \
		--include GObject-2.0 \
		--include Gtk-3.0 \
		--include Peas-1.0 \
		--include GExiv2-$(GEXIV2_GIR_VERSION) \
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
                --pkg=gtk+-3.0 \
                --pkg=libgphoto2 \
                --pkg=libpeas-1.0 \
                --pkg=dbus-glib-1 \
                --pkg=gexiv2 \
                $(libentangle_backend_la_SOURCES:%=$(srcdir)/%) \
                $(libentangle_frontend_la_SOURCES:%=$(srcdir)/%) \
                $(entangle_SOURCES:%=$(srcdir)/%)

girdir = $(datadir)/gir-1.0
gir_DATA = Entangle-0.1.gir

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = Entangle-0.1.typelib

%.typelib: %.gir
	$(AM_V_GEN)$(G_IR_COMPILER) \
		--includedir=$(srcdir) \
		--includedir=$(builddir) \
		-o $@ $<

CLEANFILES += Entangle-0.1.gir $(typelibs_DATA)
