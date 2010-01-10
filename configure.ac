dnl Process this file with autoconf to produce a configure script.

AC_INIT([capa], [0.0.1])
AC_CONFIG_SRCDIR([src/capa-main.c])
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

GLIB_REQUIRED=2.10.0
AC_SUBST(GLIB_REQUIRED)
GTHREAD_REQUIRED=2.10.0
AC_SUBST(GTHREAD_REQUIRED)
GDK_PIXBUF_REQUIRED=2.12.0
AC_SUBST(GDK_PIXBUF_REQUIRED)
GTK_REQUIRED=2.12.0
AC_SUBST(GTK_REQUIRED)
GPHOTO2_REQUIRED=2.4.7
AC_SUBST(GPHOTO2_REQUIRED)
GLADE2_REQUIRED=2.6.0
AC_SUBST(GLADE2_REQUIRED)
GCONF2_REQUIRED=2.12.0
AC_SUBST(GCONF2_REQUIRED)
HAL_REQUIRED=0.5.0
AC_SUBST(HAL_REQUIRED)
DBUS_GLIB_REQUIRED=0.60
AC_SUBST(DBUS_GLIB_REQUIRED)
GOBJECT_INTROSPECTION_REQUIRED=0.6.2
AC_SUBST(GOBJECT_INTROSPECTION_REQUIRED)
LCMS_REQUIRED=1.18
AC_SUBST(LCMS_REQUIRED)
STARTUP_REQUIRED=0.5
AC_SUBST(STARTUP_REQUIRED)
UNIQUE_REQUIRED=1.0.0
AC_SUBST(UNIQUE_REQUIRED)
GJS_REQUIRED=0.4
AC_SUBST(GJS_REQUIRED)

dnl *******************************************************************************
dnl Setup core compilers / build helpers
dnl *******************************************************************************

AC_PROG_CC_STDC
AM_PROG_CC_C_O

AM_PROG_LIBTOOL


PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GMODULE, gmodule-2.0 >= $GLIB_REQUIRED)
AC_SUBST(GMODULE_CFLAGS)
AC_SUBST(GMODULE_LIBS)

PKG_CHECK_MODULES(GTHREAD, gthread-2.0 >= $GTHREAD_REQUIRED)
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0 >= $GDK_PIXBUF_REQUIRED)
AC_SUBST(GDK_PIXBUF_CFLAGS)
AC_SUBST(GDK_PIXBUF_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_REQUIRED)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(GPHOTO2, libgphoto2 >= $GPHOTO2_REQUIRED)
AC_SUBST(GPHOTO2_CFLAGS)
AC_SUBST(GPHOTO2_LIBS)

PKG_CHECK_MODULES(GLADE2, libglade-2.0 >= $GLADE2_REQUIRED)
AC_SUBST(GLADE2_CFLAGS)
AC_SUBST(GLADE2_LIBS)

PKG_CHECK_MODULES(GCONF2, gconf-2.0 >= $GCONF2_REQUIRED)
AC_SUBST(GCONF2_CFLAGS)
AC_SUBST(GCONF2_LIBS)

PKG_CHECK_MODULES(HAL, hal >= $HAL_REQUIRED)
AC_SUBST(HAL_CFLAGS)
AC_SUBST(HAL_LIBS)

PKG_CHECK_MODULES(DBUS_GLIB, dbus-glib-1 >= $DBUS_GLIB_REQUIRED)
AC_SUBST(DBUS_GLIB_CFLAGS)
AC_SUBST(DBUS_GLIB_LIBS)

PKG_CHECK_MODULES(LCMS, lcms >= $LCMS_REQUIRED)
AC_SUBST(LCMS_CFLAGS)
AC_SUBST(LCMS_LIBS)

PKG_CHECK_MODULES(STARTUP, libstartup-notification-1.0 >= $STARTUP_REQUIRED)
AC_SUBST(STARTUP_CFLAGS)
AC_SUBST(STARTUP_LIBS)

PKG_CHECK_MODULES(UNIQUE, unique-1.0 >= $UNIQUE_REQUIRED)
AC_SUBST(UNIQUE_CFLAGS)
AC_SUBST(UNIQUE_LIBS)


GTK_DOC_CHECK(1.9)


AC_ARG_ENABLE([introspection],
        AS_HELP_STRING([--enable-introspection], [enable GObject introspection]),
        [], [enable_introspection=check])

if test "x$enable_introspection" != "xno" ; then
        PKG_CHECK_MODULES([GOBJECT_INTROSPECTION],
                          [gobject-introspection-1.0 >= $GOBJECT_INTROSPECTION_REQUIRED],
                          [enable_introspection=yes],
                          [
                             if test "x$enable_introspection" = "xcheck"; then
			       enable_introspection=no
                             else
                               AC_MSG_ERROR([gobject-introspection is not available])
                             fi
                          ])
        if test "x$enable_introspection" = "xyes" ; then
          AC_DEFINE([WITH_GOBJECT_INTROSPECTION], [1], [enable GObject introspection support])
          AC_SUBST(GOBJECT_INTROSPECTION_CFLAGS)
          AC_SUBST(GOBJECT_INTROSPECTION_LIBS)
          AC_SUBST([G_IR_SCANNER], [$($PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0)])
          AC_SUBST([G_IR_COMPILER], [$($PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0)])
        fi
fi
AM_CONDITIONAL([WITH_GOBJECT_INTROSPECTION], [test "x$enable_introspection" = "xyes"])

AC_ARG_WITH([javascript],
      AS_HELP_STRING([--with-javascript],[enable JavaScript plugins]),
      [], [with_javascript=check])

if test "x$with_javascript" != "xno" ; then
  if test "x$enable_introspection" = "xno" ; then
    if test "x$with_javascript" = "xyes"; then
      AC_MSG_ERROR([gobject-introspection is requird for javascript plugins])
    fi
  fi

  PKG_CHECK_MODULES(GJS, gjs-1.0 >= $GJS_REQUIRED)
  AC_SUBST(GJS_CFLAGS)
  AC_SUBST(GJS_LIBS)

  PKG_CHECK_MODULES(GJS_GI, gjs-gi-1.0 >= $GJS_REQUIRED)
  AC_SUBST(GJS_GI_CFLAGS)
  AC_SUBST(GJS_GI_LIBS)

  with_javascript=yes
  AC_DEFINE([WITH_JAVASCRIPT], [1], [enable JavaScript plugins])
fi
AM_CONDITIONAL([WITH_JAVASCRIPT], [test "x$with_javascript" = "xyes"])


CAPA_COMPILE_WARNINGS

AC_PATH_PROG([TPAGE], [tpage])

if test -z "$TPAGE" ; then
   AC_MSG_ERROR([The 'tpage' command from Template-Toolkit is required to build the website])
fi


AC_CONFIG_FILES(
  Makefile
  capa.spec
  src/Makefile
  docs/Makefile
  docs/reference/Makefile
  docs/website/Makefile
  )

AC_OUTPUT
