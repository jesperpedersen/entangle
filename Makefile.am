
SUBDIRS = src docs

ACLOCAL_AMFLAGS = -I m4

# XXX figure out why these aren't detected by git.mk
GITIGNOREFILES = aclocal.m4 entangle-*.tar.gz compile config.guess config.h.in config.sub depcomp install-sh missing ltmain.sh m4 gtk-doc.make INSTALL

EXTRA_DIST = entangle.spec entangle.spec.in

MAINTAINERCLEANFILES = ChangeLog

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

dist-hook: gen-ChangeLog

# Generate the ChangeLog file (with all entries since the switch to git)
# and insert it into the directory we're about to use to create a tarball.
gen_start_date = 2009-01-01
.PHONY: gen-ChangeLog
gen-ChangeLog:
	if test -d .git; then                                   \
          $(top_srcdir)/gitlog-to-changelog                     \
            --since=$(gen_start_date) > $(distdir)/cl-t;        \
          rm -f $(distdir)/ChangeLog;                           \
          mv $(distdir)/cl-t $(distdir)/ChangeLog;              \
        fi

-include $(top_srcdir)/git.mk
