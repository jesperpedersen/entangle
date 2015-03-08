
SUBDIRS = src po docs help

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	$(PACKAGE).spec \
	$(PACKAGE).spec.in \
	GNUmakefile \
	cfg.mk \
	AUTHORS.in \
	maint.mk \
	$(NULL)

MAINTAINERCLEANFILES = ChangeLog AUTHORS

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

dist-hook: gen-ChangeLog gen-AUTHORS

.PHONY: gen-ChangeLog gen-AUTHORS
gen-ChangeLog:
	if test -d .git; then                                   \
	  $(top_srcdir)/build-aux/gitlog-to-changelog           \
	    > $(distdir)/cl-t;                                  \
	  rm -f $(distdir)/ChangeLog;                           \
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;              \
	fi

gen-AUTHORS:
	$(AM_V_GEN)if test -d $(srcdir)/.git; then                      \
	  out="`cd $(srcdir) && git log --pretty=format:'%aN <%aE>' | sort -u`" && \
	  perl -p -e "s/#authorslist#// and print '$$out'"            \
	    < $(srcdir)/AUTHORS.in > $(distdir)/AUTHORS-tmp &&        \
	  mv -f $(distdir)/AUTHORS-tmp $(distdir)/AUTHORS ;           \
	fi
