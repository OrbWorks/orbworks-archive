<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" encoding="utf-8"/>

	<xsl:template match="Documentation" mode="book">
		<xsl:apply-templates select="book" mode="book"/>
	</xsl:template>

	<xsl:template match="book" mode="book">
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
		<file name="{$fname}" book="{$book}">

		<html>
		<title><xsl:value-of select="@title"/></title>
		<body>
			<xsl:call-template name="genNavBar"/>
			<p><font size="4">
				<b><xsl:value-of select="@title" /></b>
			</font></p>
			<table border="0" width="100%">
			<xsl:for-each select="topic|category">
				<tr>
				<td width="15"> </td>
				<td width="30%" bgcolor="#E0E0E0" valign="top">
					<xsl:choose>
					<xsl:when test="@title">
						<b>
						<xsl:call-template name="genLink">
							<xsl:with-param name="node" select="."/>
							<xsl:with-param name="text" select="@title"/>
						</xsl:call-template>
						</b>
					</xsl:when>
					<xsl:otherwise>
						<b>
						<xsl:call-template name="genLink">
							<xsl:with-param name="node" select="."/>
							<xsl:with-param name="text" select="@name"/>
						</xsl:call-template>
						</b>
					</xsl:otherwise>
					</xsl:choose>
				</td>
				<xsl:choose>
				<xsl:when test="sdesc">
					<td width="70%">
						<xsl:apply-templates select="sdesc"/>
					</td>
				</xsl:when>
				<xsl:otherwise>
					<td width="70%">
						<xsl:apply-templates select="desc"/>
					</td>
				</xsl:otherwise>
				</xsl:choose>
				</tr>
			</xsl:for-each>
			</table>
		</body>
		</html>
		</file>
	</xsl:template>
</xsl:stylesheet>