
SUBDIRS = src po docs

ACLOCAL_AMFLAGS = -I m4

# XXX figure out why these aren't detected by git.mk
GITIGNOREFILES = \
        aclocal.m4 \
        entangle-*.tar.gz \
        compile \
        config.guess \
        config.h.in \
        config.sub \
        depcomp \
        install-sh \
        missing \
        ltmain.sh \
        gtk-doc.make \
        m4/gtk-doc.m4 \
        m4/libtool.m4 \
        m4/ltversion.m4 \
        m4/ltoptions.m4 \
        m4/ltsugar.m4 \
        m4/lt~obsolete.m4 \
        INSTALL

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

-include $(top_srcdir)/git.mk
