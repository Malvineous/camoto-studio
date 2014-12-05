man_MANS = camoto-studio.1

EXTRA_DIST = camoto-studio.xml
EXTRA_DIST += camoto.xsl

# Also distribute the converted man pages so users don't need DocBook installed
EXTRA_DIST += $(man_MANS)

HTML_MAN = camoto-studio.html

.PHONY: html

html: $(HTML_MAN)

MAINTAINERCLEANFILES = $(man_MANS) $(HTML_MAN)

%.1: %.xml camoto.xsl
	xmlto --skip-validation man -m $(<D)/camoto.xsl $<

%.html: %.xml camoto.xsl
	xmlto --skip-validation xhtml-nochunks -m $(<D)/camoto.xsl $<
