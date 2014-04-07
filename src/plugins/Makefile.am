
SUBDIRS = photobox

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(libdir)/entangle/plugins
	$(MKDIR_P) $(DESTDIR)$(datadir)/entangle/plugins

uninstall-local::
	rmdir $(DESTDIR)$(libdir)/entangle/plugins ||:
	rmdir $(DESTDIR)$(libdir)/entangle ||:
	rmdir $(DESTDIR)$(datadir)/entangle/plugins ||:
	rmdir $(DESTDIR)$(datadir)/entangle ||:
