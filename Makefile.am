
SUBDIRS = src docs

ACLOCAL_AMFLAGS = -I m4

# XXX figure out why these aren't detected by git.mk
GITIGNOREFILES = aclocal.m4 capa-*.tar.gz compile config.guess config.h.in config.sub depcomp install-sh missing ltmain.sh m4 gtk-doc.make

EXTRA_DIST = capa.spec capa.spec.in

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

-include $(top_srcdir)/git.mk
