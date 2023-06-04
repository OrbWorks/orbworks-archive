<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" encoding="utf-8"/>

	<xsl:include href="common.xsl"/>
	<xsl:include href="toc.xsl"/>
	<xsl:include href="book.xsl"/>
	<xsl:include href="category.xsl"/>
	<xsl:include href="method.xsl"/>
	<xsl:include href="object.xsl"/>
	<xsl:include href="topic.xsl"/>

	<xsl:template match="/">
		<xsl:apply-templates select="Documentation" mode="toc"/>
		<xsl:apply-templates select="Documentation" mode="book"/>
		<xsl:apply-templates select="Documentation" mode="category"/>
		<xsl:apply-templates select="Documentation" mode="object"/>
		<xsl:apply-templates select="Documentation" mode="method"/>
		<xsl:apply-templates select="Documentation" mode="topic"/>
	</xsl:template>

</xsl:stylesheet>
