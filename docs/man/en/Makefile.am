
man1_SRC = entangle.pod
man1_MANS = $(man1_SRC:%.pod=%.1)

POD2MAN = pod2man -c "User Commands" -r "$(PACKAGE)-$(VERSION)"

EXTRA_DIST = $(man1_SRC) $(man1_MANS)

entangle.1: entangle.pod
	$(AM_V_GEN)$(POD2MAN) --name ENTANGLE $< $(srcdir)/$@

MAINTAINERCLEANFILES = $(man1_MANS)
