<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

	<xsl:strip-space elements="*"/>

	<!-- HTML options -->
	<xsl:param name="citerefentry.link" select="1"/>

	<xsl:template name="generate.citerefentry.link">
		<xsl:text>/camoto/manpage/</xsl:text>
		<xsl:value-of select="refentrytitle"/>
	</xsl:template>

	<!-- man page options -->
	<xsl:param name="man.endnotes.list.heading">References</xsl:param>

</xsl:stylesheet>
