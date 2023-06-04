<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" encoding="utf-8"/>

	<xsl:template match="Documentation" mode="category">
		<xsl:apply-templates select="book/category" mode="category"/>
	</xsl:template>

	<xsl:template match="category" mode="category">
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

		<!-- Object name -->
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
		<html>
		<title keywords="{$keywords}"><xsl:value-of select="$title" /></title>
		<body>
			<xsl:call-template name="genNavBar"/>
			<p><font size="4"><b><xsl:value-of select="$title" /></b></font></p>
			<p><xsl:value-of select="desc"/></p>

			<!-- Objects -->
			<xsl:if test="object">
				<p><b>Objects and Structures</b></p>
				<table border="0" width="100%">

				<xsl:for-each select="object">
					<tr>
					<td width="15"> </td>
					<td width="20%" bgcolor="#E0E0E0" valign="top">
						<b>
						<xsl:call-template name="genLink">
							<xsl:with-param name="node" select="."/>
							<xsl:with-param name="text" select="@name"/>
						</xsl:call-template>
						</b>
					</td>
					<td width="80%">
						<xsl:apply-templates select="sdesc"/>
					</td>
					</tr>
				</xsl:for-each>

				</table>
			</xsl:if>

			<!-- Functions-->
			<xsl:if test="method">
				<p><b>Functions</b></p>
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

			<xsl:if test="topic">
				<p><b>Additional Topics</b></p>
				<table border="0" width="100%">

				<xsl:for-each select="topic">
					<tr>
					<td width="15"> </td>
					<td width="20%" bgcolor="#E0E0E0" valign="top">
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
					<td width="80%">
						<xsl:apply-templates select="sdesc"/>
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