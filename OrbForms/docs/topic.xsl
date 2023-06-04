<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" encoding="utf-8"/>

	<xsl:template match="Documentation" mode="topic">
		<xsl:apply-templates select="//topic" mode="topic"/>
	</xsl:template>

	<xsl:template match="topic" mode="topic">
		<xsl:variable name="fname">
			<xsl:call-template name="genPath">
				<xsl:with-param name="node" select="."/>
			</xsl:call-template>
		</xsl:variable>
		<xsl:variable name="book">
			<xsl:call-template name="genBook">
				<xsl:with-param name="node" select="."/>
			</xsl:call-template>
		</xsl:variable>
		<xsl:variable name="title">
			<xsl:choose>
			<xsl:when test="@title">
				<xsl:value-of select="@title"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="@name"/>
			</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
		<xsl:variable name="keywords">
			<xsl:choose>
			<xsl:when test="@keywords">
				<xsl:value-of select="@keywords"/>
			</xsl:when>
			<xsl:otherwise>
			</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>

		<file name="{$fname}" book="{$book}">

		<!-- Object name -->
		<html>
		<title keywords="{$keywords}"><xsl:value-of select="$title" /></title>
		<body>
			<xsl:call-template name="genNavBar"/>
			<p><font size="4"><b><xsl:value-of select="$title" /></b></font></p>
			<font size="3">
				<xsl:apply-templates select="desc"/>
			</font>
		</body>
		</html>
		</file>
	</xsl:template>

</xsl:stylesheet>