dnl Process this file with autoconf to produce a configure script.

AC_INIT([capa], [0.0.1])
AC_CONFIG_SRCDIR([src/main.c])
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
GTK_REQUIRED=2.12.0
AC_SUBST(GTK_REQUIRED)
GPHOTO2_REQUIRED=2.4.7
AC_SUBST(GPHOTO2_REQUIRED)
GLADE2_REQUIRED=2.6.0
AC_SUBST(GLADE2_REQUIRED)

dnl *******************************************************************************
dnl Setup core compilers / build helpers
dnl *******************************************************************************

AC_PROG_CC_STDC
AM_PROG_CC_C_O

AM_PROG_LIBTOOL


PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_REQUIRED)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(GPHOTO2, libgphoto2 >= $GPHOTO2_REQUIRED)
AC_SUBST(GPHOTO2_CFLAGS)
AC_SUBST(GPHOTO2_LIBS)

PKG_CHECK_MODULES(GLADE2, libglade-2.0 >= $GLADE2_REQUIRED)
AC_SUBST(GLADE2_CFLAGS)
AC_SUBST(GLADE2_LIBS)

CAPA_COMPILE_WARNINGS

AC_CONFIG_FILES(
  Makefile
  src/Makefile
  )

AC_OUTPUT
