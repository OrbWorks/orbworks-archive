<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" encoding="utf-8"/>

	<xsl:template match="Documentation" mode="method">
		<xsl:apply-templates select="book/category/object/method" mode="method"/>
		<xsl:apply-templates select="book/category/method" mode="method"/>
	</xsl:template>

	<xsl:template match="method" mode="method">
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

		<xsl:variable name="mname">
			<xsl:choose>
			<xsl:when test="name(..)='object'">
				<xsl:value-of select="../@name"/>.<xsl:value-of select="@name"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="@name"/>
			</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>
		<xsl:variable name="keywords">
			<xsl:choose>
			<xsl:when test="name(..)='object'">
				<xsl:value-of select="@name"/>
			</xsl:when>
			<xsl:otherwise>
			</xsl:otherwise>
			</xsl:choose>
		</xsl:variable>

		<!-- Object name -->
		<html>
		<title keywords="{$keywords}"><xsl:value-of select="$mname" /></title>
		<body>
			<xsl:call-template name="genNavBar"/>
			<p><font size="4"><b><xsl:value-of select="$mname" /></b></font></p>
			<!-- Declaration -->
			<table border="1" bordercolor="#000000" cellspacing="0" cellpadding="3" bgcolor="#E0E0E0" width="100%">
				<tr><td><code><xsl:call-template name="funcDecl"/></code></td></tr>
			</table>

			<!-- Parameters -->
			<xsl:if test="param">
				<p><b>Parameters:</b></p>
				<table border="0">
				<xsl:for-each select="param">
					<tr>
					<td width="15"> </td>
					<td width="20%"><xsl:value-of select="@name"/></td>
					<td width="80%"><xsl:value-of select="."/></td>
					</tr>
				</xsl:for-each>
				</table>
			</xsl:if>

			<!-- Return value -->
			<xsl:if test="not(return/@type='void')">
				<p><b>Return value: </b><xsl:value-of select="return"/></p>
			</xsl:if>

			<!-- Description -->
			<p><xsl:apply-templates select="desc"/></p>

			<!-- Notes -->
			<xsl:apply-templates select="note"/>

			<!-- Example -->
			<xsl:apply-templates select="example"/>
		</body>
		</html>
		</file>
	</xsl:template>

</xsl:stylesheet>