
PAGES = $(wildcard *.html.in)
TEMPLATES = page.html navigation.html

websitedir = $(datadir)/docs/$(PACKAGE)-$(VERSION)/web
website_DATA = $(PAGES:%.html.in=%.html)

%.html: %.html.in $(TEMPLATES)
	$(TPAGE) $< > $@

CLEANFILES = $(website_DATA)

-include $(top_srcdir)/git.mk
