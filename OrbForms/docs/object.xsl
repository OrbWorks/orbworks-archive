<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" encoding="utf-8"/>

	<xsl:template match="Documentation" mode="object">
		<xsl:apply-templates select="book/category/object" mode="object"/>
	</xsl:template>

	<xsl:template match="object" mode="object">
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
		<title keywords="{$keywords}"><xsl:value-of select="@name" /></title>
		<body>
			<xsl:call-template name="genNavBar"/>
			<p><font size="4"><b><xsl:value-of select="@name" /></b></font></p>
			<p><xsl:apply-templates select="desc"/></p>

			<!-- Properties -->
			<xsl:if test="property">
				<p><b>Object Properties</b></p>
				<table border="0" width="100%">

				<xsl:for-each select="property">
					<tr>
					<td width="15"> </td>
					<td width="20%" bgcolor="#E0E0E0" valign="top">
						<xsl:value-of select="@type"/>
						<xsl:text> </xsl:text>
						<b><xsl:value-of select="@name"/></b>
					</td>
					<td width="80%">
						<xsl:apply-templates select="desc"/>
					</td>
					</tr>
				</xsl:for-each>

				</table>
			</xsl:if>

			<!-- Methods -->
			<xsl:if test="method">
				<p><b>Object Methods</b></p>
				<table border="0" width="100%">

				<xsl:for-each select="method">
					<tr>
					<td width="15"> </td>
					<td width="20%" bgcolor="#E0E0E0" valign="top">
						<xsl:call-template name="funcDecl">
							<xsl:with-param name="addLink" select="true()"/>
							<xsl:with-param name="node" select="."/>
						</xsl:call-template>
					</td>
					<td width="80%">
						<xsl:apply-templates select="desc"/>
					</td>
					</tr>
				</xsl:for-each>

				</table>
			</xsl:if>

			<!-- Handlers -->
			<xsl:if test="handler">
				<p><b>Object Handlers</b></p>
				<table border="0" width="100%">

				<xsl:for-each select="handler">
					<tr>
					<td width="15"> </td>
					<td width="20%" bgcolor="#E0E0E0" valign="top">
						<b><xsl:value-of select="@name"/></b>
					</td>
					<td width="80%">
						<xsl:apply-templates select="text()|const|xref"/>
					</td>
					</tr>
				</xsl:for-each>

				</table>
			</xsl:if>

		</body>
		</html>
		</file>
	</xsl:template>

</xsl:stylesheet>