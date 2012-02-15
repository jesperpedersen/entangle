#!/bin/sh
# Run this to generate all the initial makefiles, etc.

set -e
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

THEDIR=`pwd`
cd $srcdir

DIE=0

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile gtk-vnc."
	echo "Download the appropriate package for your distribution,"
	echo "or see http://www.gnu.org/software/autoconf"
	DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo
	DIE=1
	echo "You must have automake installed to compile gtk-vnc."
	echo "Download the appropriate package for your distribution,"
	echo "or see http://www.gnu.org/software/automake"
}

if test "$DIE" -eq 1; then
	exit 1
fi

if test -z "$*"; then
	echo "I am going to run ./configure with --enable-werror"
        echo "If you wish to pass any extra arguments to it, please specify them"
        echo "on the $0 command line."
fi

mkdir -p m4

intltoolize --force
gtkdocize || exit 1
autoreconf -if

cd $THEDIR

$srcdir/configure --enable-werror "$@" && {
    echo
    echo "Now type 'make' to compile entangle."
}
