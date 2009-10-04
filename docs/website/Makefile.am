
PAGES = $(wildcard *.html.in)
TEMPLATES = page.html navigation.html
IMAGES = $(wildcard *.png)
STYLES = $(wildcard *.css)

HTMLPAGES = $(PAGES:%.html.in=%.html)

websitedir = $(datadir)/$(PACKAGE)/website
website_DATA = $(HTMLPAGES) $(IMAGES) $(STYLES)

EXTRA_DIST = $(TEMPLATES) $(PAGES) $(IMAGES) $(STYLES)

%.html: %.html.in $(TEMPLATES)
	$(TPAGE) $< > $@

CLEANFILES = $(HTMLPAGES)

GITIGNOREFILES = $(CLEANFILES)

-include $(top_srcdir)/git.mk
