# Find valid warning flags for the C Compiler.           -*-Autoconf-*-
#
# Copyright (C) 2001, 2002, 2006 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301  USA

# Written by Jesse Thilo.

AC_DEFUN([gl_COMPILER_FLAGS],
  [AC_MSG_CHECKING(whether compiler accepts $1)
   AC_SUBST(COMPILER_FLAGS)
   ac_save_CFLAGS="$CFLAGS"
   dnl Some flags are dependant, so we set all previously checked
   dnl flags when testing. Except for -Werror which we have to
   dnl check on its own, because some of our compiler flags cause
   dnl warnings from the autoconf test program!
   if test "$1" = "-Werror" ; then
     CFLAGS="$CFLAGS $1"
   else
     CFLAGS="$CFLAGS $COMPILER_FLAGS $1"
   fi
   AC_TRY_LINK([], [], has_option=yes, has_option=no,)
   echo 'int x;' >conftest.c
   $CC $CFLAGS -c conftest.c 2>conftest.err
   ret=$?
   if test $ret != 0 -o -s conftest.err -o $has_option = "no"; then
       AC_MSG_RESULT(no)
   else
       AC_MSG_RESULT(yes)
       COMPILER_FLAGS="$COMPILER_FLAGS $1"
   fi
   CFLAGS="$ac_save_CFLAGS"
   rm -f conftest*
 ])


dnl
dnl Taken from gnome-common/macros2/gnome-compiler-flags.m4
dnl
dnl We've added:
dnl   -Wextra -Wshadow -Wcast-align -Wwrite-strings -Waggregate-return -Wstrict-prototypes -Winline -Wredundant-decls
dnl We've removed
dnl   CFLAGS="$realsave_CFLAGS"
dnl   to avoid clobbering user-specified CFLAGS
dnl
AC_DEFUN([CAPA_COMPILE_WARNINGS],[
    dnl ******************************
    dnl More compiler warnings
    dnl ******************************

    AC_ARG_ENABLE(compile-warnings,
                  AC_HELP_STRING([--enable-compile-warnings=@<:@no/minimum/yes/maximum/error@:>@],
                                 [Turn on compiler warnings]),,
                  [enable_compile_warnings="m4_default([$1],[maximum])"])

    warnCFLAGS=

    common_flags="-Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fasynchronous-unwind-tables"

    case "$enable_compile_warnings" in
    no)
        try_compiler_flags=""
	;;
    minimum)
	try_compiler_flags="-Wall -Wformat -Wformat-security $common_flags"
	;;
    yes)
	try_compiler_flags="-Wall -Wformat -Wformat-security -Wmissing-prototypes $common_flags"
	;;
    maximum|error)
	try_compiler_flags="-Wall -Wformat -Wformat-security -Wmissing-prototypes -Wnested-externs -Wpointer-arith"
	try_compiler_flags="$try_compiler_flags -Wextra -Wshadow -Wcast-align -Wwrite-strings -Waggregate-return"
	dnl XXX disabled strict-prototypes due  to bug in gtk header files
	dnl /usr/include/gtk-2.0/gtk/gtkitemfactory.h:47: warning: function declaration isn’t a prototype
	dnl try_compiler_flags="$try_compiler_flags -Wstrict-prototypes -Winline -Wredundant-decls -Wno-sign-compare"
	try_compiler_flags="$try_compiler_flags -Winline -Wredundant-decls -Wno-sign-compare"
	try_compiler_flags="$try_compiler_flags $common_flags"
	if test "$enable_compile_warnings" = "error" ; then
	    try_compiler_flags="$try_compiler_flags -Werror"
	fi
	;;
    *)
	AC_MSG_ERROR(Unknown argument '$enable_compile_warnings' to --enable-compile-warnings)
	;;
    esac

    COMPILER_FLAGS=
    for option in $try_compiler_flags; do
        gl_COMPILER_FLAGS($option)
    done
    unset option
    unset try_compiler_flags

    WARNING_CFLAGS="$COMPILER_FLAGS $complCFLAGS"
    AC_SUBST(WARNING_CFLAGS)
])
