
pluginlibdir = $(libdir)/entangle/plugins/photobox
pluginlib_DATA = $(wildcard *.py) $(wildcard *.plugin)

settingsdir = $(datadir)/entangle/plugins/photobox/schemas
settings_DATA = org.entangle-photo.plugins.photobox.gschema.xml


EXTRA_DIST = $(pluginlib_DATA) $(settings_DATA)

install-data-hook:
	$(MKDIR_P) $(DESTDIR)$(settingsdir)
	$(GLIB_COMPILE_SCHEMAS) $(DESTDIR)$(settingsdir)/

uninstall-local:
	rm -f $(DESTDIR)/$(settingsdir)/$(settings_DATA) || :
	rm -f $(DESTDIR)/$(settingsdir)/gschemas.compiled || :
	rmdir $(DESTDIR)/$(settingsdir) || :
