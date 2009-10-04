
PAGES = $(wildcard *.html.in)
TEMPLATES = page.html navigation.html
IMAGES = $(wildcard *.png)
STYLES = $(wildcard *.css)

HTMLPAGES = $(PAGES:%.html.in=%.html)

FANCYBOX = $(wildcard jquery.fancybox/*.png) $(wildcard jquery.fancybox/*.js) $(wildcard jquery.fancybox/*.css)

websitedir = $(datadir)/$(PACKAGE)/website
website_DATA = $(HTMLPAGES) $(IMAGES) $(STYLES)

fancyboxdir = $(websitedir)/jquery.fancybox
fancybox_DATA = $(FANCYBOX)

EXTRA_DIST = $(TEMPLATES) $(PAGES) $(IMAGES) $(STYLES) $(FANCYBOX)

%.html: %.html.in $(TEMPLATES)
	$(TPAGE) $< > $@

CLEANFILES = $(HTMLPAGES)

GITIGNOREFILES = $(CLEANFILES)

-include $(top_srcdir)/git.mk
