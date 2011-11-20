dnl Process this file with autoconf to produce a configure script.

AC_INIT([entangle], [0.2.0])
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
GPHOTO2_REQUIRED=2.4.8
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

dnl *******************************************************************************
dnl Setup core compilers / build helpers
dnl *******************************************************************************

AC_PROG_CC_STDC
AM_PROG_CC_C_O

AM_PROG_LIBTOOL


PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GIO, glib-2.0 >= $GIO_REQUIRED)
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

PKG_CHECK_MODULES(GMODULE, gmodule-2.0 >= $GLIB_REQUIRED)
AC_SUBST(GMODULE_CFLAGS)
AC_SUBST(GMODULE_LIBS)

PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= $GTHREAD_REQUIRED)
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0 >= $GDK_PIXBUF_REQUIRED)
AC_SUBST(GDK_PIXBUF_CFLAGS)
AC_SUBST(GDK_PIXBUF_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-3.0 >= $GTK_REQUIRED)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(GPHOTO2, libgphoto2 >= $GPHOTO2_REQUIRED)
AC_SUBST(GPHOTO2_CFLAGS)
AC_SUBST(GPHOTO2_LIBS)

PKG_CHECK_MODULES(GUDEV, gudev-1.0 >= $GUDEV_REQUIRED)
AC_SUBST(GUDEV_CFLAGS)
AC_SUBST(GUDEV_LIBS)

PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1 >= $DBUS_GLIB_REQUIRED)
AC_SUBST(DBUS_GLIB_CFLAGS)
AC_SUBST(DBUS_GLIB_LIBS)

PKG_CHECK_MODULES(LCMS, lcms >= $LCMS_REQUIRED)
AC_SUBST(LCMS_CFLAGS)
AC_SUBST(LCMS_LIBS)

GTK_DOC_CHECK(1.9)

PKG_CHECK_MODULES([GOBJECT_INTROSPECTION],
                  [gobject-introspection-1.0 >= $GOBJECT_INTROSPECTION_REQUIRED],
                  [enable_introspection=yes],
                  [AC_MSG_ERROR([gobject-introspection is not available])])
AC_DEFINE([WITH_GOBJECT_INTROSPECTION], [1], [enable GObject introspection support])
AC_SUBST(GOBJECT_INTROSPECTION_CFLAGS)
AC_SUBST(GOBJECT_INTROSPECTION_LIBS)
AC_SUBST([G_IR_SCANNER], [$($PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0)])
AC_SUBST([G_IR_COMPILER], [$($PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0)])

PKG_CHECK_MODULES(PEAS, libpeas-1.0 >= $LIBPEAS_REQUIRED,
                  [],[AC_MSG_ERROR([libpeas >= $LIBPEAS_REQUIRED is required for plugins])])
PKG_CHECK_MODULES(PEAS_UI, libpeas-gtk-1.0 >= $LIBPEAS_REQUIRED,
                  [],[AC_MSG_ERROR([libpeas-gtk >= $LIBPEAS_REQUIRED is required for plugins])])
AC_SUBST(LIBPEAS_CFLAGS)
AC_SUBST(LIBPEAS_LIBS)
AC_SUBST(LIBPEAS_UI_CFLAGS)
AC_SUBST(LIBPEAS_UI_LIBS)

ENTANGLE_COMPILE_WARNINGS


AC_CONFIG_FILES(
  Makefile
  entangle.spec
  src/Makefile
  docs/Makefile
  docs/reference/Makefile
  )

AC_OUTPUT
