dnl Process this file with autoconf to produce a configure script.

AC_INIT([entangle], [0.3.3])
AC_CONFIG_SRCDIR([src/entangle-main.c])
AM_CONFIG_HEADER([config.h])
dnl Make automake keep quiet about wildcards & other GNUmake-isms
AM_INIT_AUTOMAKE([-Wno-portability])

AC_CONFIG_MACRO_DIR([m4])

# Use the silent-rules feature when possible.
m4_ifndef([AM_SILENT_RULES], [m4_define([AM_SILENT_RULES],[])])
AM_SILENT_RULES([yes])

AC_CANONICAL_HOST

dnl *******************************************************************************
dnl Declare required library versions
dnl *******************************************************************************

GLIB_REQUIRED=2.26.0
AC_SUBST(GLIB_REQUIRED)
GIO_REQUIRED=2.26.0
AC_SUBST(GIO_REQUIRED)
GTHREAD_REQUIRED=2.26.0
AC_SUBST(GTHREAD_REQUIRED)
GDK_PIXBUF_REQUIRED=2.12.0
AC_SUBST(GDK_PIXBUF_REQUIRED)
GTK_REQUIRED=3.0.0
AC_SUBST(GTK_REQUIRED)
GPHOTO2_REQUIRED=2.4.11
AC_SUBST(GPHOTO2_REQUIRED)
GUDEV_REQUIRED=145
AC_SUBST(GUDEV_REQUIRED)
DBUS_GLIB_REQUIRED=0.60
AC_SUBST(DBUS_GLIB_REQUIRED)
GOBJECT_INTROSPECTION_REQUIRED=0.9.3
AC_SUBST(GOBJECT_INTROSPECTION_REQUIRED)
LCMS_REQUIRED=1.18
AC_SUBST(LCMS_REQUIRED)
LIBPEAS_REQUIRED=0.5.5
AC_SUBST(LIBPEAS_REQUIRED)
GEXIV2_REQUIRED=0.2.2
AC_SUBST(GEXIV2_REQUIRED)
XEXT_REQUIRED=1.3.0
AC_SUBST(XEXT_REQUIRED)

dnl *******************************************************************************
dnl Setup core compilers / build helpers
dnl *******************************************************************************

AC_PROG_CC_STDC
AM_PROG_CC_C_O

AM_PROG_LIBTOOL


PKG_CHECK_MODULES([GLIB], [glib-2.0 >= $GLIB_REQUIRED])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES([GIO], [glib-2.0 >= $GIO_REQUIRED])
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

AC_MSG_CHECKING([for glib-compile-schemas])
AC_SUBST([GLIB_COMPILE_SCHEMAS], [$($PKG_CONFIG --variable=glib_compile_schemas gio-2.0)])
if [ test -n "$GLIB_COMPILE_SCHEMAS" ]; then
  AC_MSG_RESULT([$GLIB_COMPILE_SCHEMAS])
else
  AC_MSG_ERROR([The $GLIB_COMPILE_SCHEMAS binary must be installed before building entangle])
fi

PKG_CHECK_MODULES([GMODULE], [gmodule-2.0 >= $GLIB_REQUIRED])
AC_SUBST(GMODULE_CFLAGS)
AC_SUBST(GMODULE_LIBS)

PKG_CHECK_MODULES([GTHREAD], [gthread-2.0 >= $GTHREAD_REQUIRED])
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

PKG_CHECK_MODULES([GDK_PIXBUF], [gdk-pixbuf-2.0 >= $GDK_PIXBUF_REQUIRED])
AC_SUBST(GDK_PIXBUF_CFLAGS)
AC_SUBST(GDK_PIXBUF_LIBS)

PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= $GTK_REQUIRED])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES([GPHOTO2], [libgphoto2 >= $GPHOTO2_REQUIRED])
AC_SUBST(GPHOTO2_CFLAGS)
AC_SUBST(GPHOTO2_LIBS)

PKG_CHECK_MODULES([GUDEV], [gudev-1.0 >= $GUDEV_REQUIRED])
AC_SUBST(GUDEV_CFLAGS)
AC_SUBST(GUDEV_LIBS)

PKG_CHECK_MODULES([DBUS_GLIB], [dbus-glib-1 >= $DBUS_GLIB_REQUIRED])
AC_SUBST(DBUS_GLIB_CFLAGS)
AC_SUBST(DBUS_GLIB_LIBS)

PKG_CHECK_MODULES([LCMS], [lcms >= $LCMS_REQUIRED])
AC_SUBST(LCMS_CFLAGS)
AC_SUBST(LCMS_LIBS)

PKG_CHECK_MODULES([GEXIV2], [gexiv2 >= $GEXIV2_REQUIRED])
AC_SUBST(GEXIV2_CFLAGS)
AC_SUBST(GEXIV2_LIBS)

PKG_CHECK_MODULES([XEXT], [xext >= $XEXT_REQUIRED],
                  [have_xext=yes], [have_xext=no])
AC_SUBST(XEXT_CFLAGS)
AC_SUBST(XEXT_LIBS)
if test "$have_xext" = "yes"; then
  AC_DEFINE_UNQUOTED([HAVE_XEXT], [], [Use Xext library])
fi


GTK_DOC_CHECK([1.10],[--flavour no-tmpl])

PKG_CHECK_MODULES([GOBJECT_INTROSPECTION],
                  [gobject-introspection-1.0 >= $GOBJECT_INTROSPECTION_REQUIRED])
AC_SUBST(GOBJECT_INTROSPECTION_CFLAGS)
AC_SUBST(GOBJECT_INTROSPECTION_LIBS)
AC_SUBST([G_IR_SCANNER], [$($PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0)])
AC_SUBST([G_IR_COMPILER], [$($PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0)])

PKG_CHECK_MODULES([LIBPEAS], [libpeas-1.0 >= $LIBPEAS_REQUIRED])
PKG_CHECK_MODULES([LIBPEAS_UI], [libpeas-gtk-1.0 >= $LIBPEAS_REQUIRED])
AC_SUBST(LIBPEAS_CFLAGS)
AC_SUBST(LIBPEAS_LIBS)
AC_SUBST(LIBPEAS_UI_CFLAGS)
AC_SUBST(LIBPEAS_UI_LIBS)

ENTANGLE_COMPILE_WARNINGS

GETTEXT_PACKAGE=entangle
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

# Autoconf 2.61a.99 and earlier don't support linking a file only
# in VPATH builds.  But since GNUmakefile is for maintainer use
# only, it does not matter if we skip the link with older autoconf.
# Automake 1.10.1 and earlier try to remove GNUmakefile in non-VPATH
# builds, so use a shell variable to bypass this.
GNUmakefile=GNUmakefile
m4_if(m4_version_compare([2.61a.100],
        m4_defn([m4_PACKAGE_VERSION])), [1], [],
      [AC_CONFIG_LINKS([$GNUmakefile:$GNUmakefile], [],
        [GNUmakefile=$GNUmakefile])])

AC_CONFIG_FILES(
  Makefile
  entangle.spec
  src/Makefile
  src/icons/Makefile
  src/icons/128x128/Makefile
  src/icons/128x128/actions/Makefile
  src/icons/16x16/Makefile
  src/icons/16x16/actions/Makefile
  src/icons/22x22/Makefile
  src/icons/22x22/actions/Makefile
  src/icons/32x32/Makefile
  src/icons/32x32/actions/Makefile
  src/icons/48x48/Makefile
  src/icons/48x48/actions/Makefile
  src/icons/64x64/Makefile
  src/icons/64x64/actions/Makefile
  po/Makefile.in
  docs/Makefile
  docs/reference/Makefile
  )

AC_OUTPUT

# Need to eval twice, to ensure we fully subsitute out all path variables
rawdatadir=`eval "echo $datadir"`
rawdatadir=`eval "echo $rawdatadir"`

searchdirs="/usr/share:/usr/local/share"
test -n "$XDG_DATA_DIRS" && searchdirs="$searchdirs:$XDG_DATA_DIRS"

found=0
oldIFS=$IFS
IFS=:
for i in $searchdirs
do
    test "$i" = "$rawdatadir" && found=1
done
IFS=$oldIFS

if test "$found" = "0"
then
  AC_MSG_NOTICE([WARNING:])
  AC_MSG_NOTICE([WARNING: GSettings schemas will be installed under:])
  AC_MSG_NOTICE([WARNING:])
  AC_MSG_NOTICE([WARNING:      $rawdatadir])
  AC_MSG_NOTICE([WARNING:])
  AC_MSG_NOTICE([WARNING: GSettings will only look for configuration file schemas under:])
  AC_MSG_NOTICE([WARNING:])
  AC_MSG_NOTICE([WARNING:      $searchdirs])
  AC_MSG_NOTICE([WARNING:])
  AC_MSG_NOTICE([WARNING: If you continue with this install location for schemas, you])
  AC_MSG_NOTICE([WARNING: should set the following environment variable:])
  AC_MSG_NOTICE([WARNING:])
  AC_MSG_NOTICE([WARNING:      XDG_DATA_DIRS=$rawdatadir])
  AC_MSG_NOTICE([WARNING:])
  AC_MSG_NOTICE([WARNING: If XDG_DATA_DIRS is not set, Entangle will attempt to enable])
  AC_MSG_NOTICE([WARNING: a workaround on startup to avoid crashes])
fi
