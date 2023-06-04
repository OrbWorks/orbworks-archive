<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" encoding="utf-8"/>

	<xsl:template match="Documentation" mode="bigfile">
		<html><body>
		<xsl:apply-templates select="book/category" mode="bigfile"/>
		</body></html>
	</xsl:template>

	<xsl:template match="category" mode="bigfile">
		<h1>
		<xsl:value-of select="@name" />
		</h1>
		<xsl:apply-templates select="desc" mode="bigfile"/>
		<xsl:apply-templates select="method" mode="bigfile"/>
		<xsl:apply-templates select="object" mode="bigfile"/>
	</xsl:template>

	<xsl:template match="category/desc|object/desc" mode="bigfile">
		<p>
		<xsl:value-of select="."/>
		</p>
	</xsl:template>
	
	<xsl:template match="method" mode="bigfile">
		<p>
		<xsl:value-of select="return/@type"/>
		<xsl:text> </xsl:text>
		<b><xsl:value-of select="@name"/></b>
		<xsl:text>(</xsl:text>
		<xsl:for-each select="param">
			<xsl:value-of select="@type"/>
			<xsl:text> </xsl:text>
			<xsl:value-of select="@name"/>
			<xsl:if test="following-sibling::param">, </xsl:if>
		</xsl:for-each>
		<xsl:text>) - </xsl:text>
		<xsl:apply-templates select="desc" mode="bigfile"/>
		<xsl:apply-templates select="note" mode="bigfile"/>
		<xsl:apply-templates select="example" mode="bigfile"/>
		</p>
	</xsl:template>
	
	<xsl:template match="desc" mode="bigfile">
		<xsl:apply-templates select="text()|param" mode="bigfile"/>
	</xsl:template>

	<xsl:template match="method/note" mode="bigfile">
		<b> Note: </b><xsl:apply-templates select="text()|param|const" mode="bigfile"/>
	</xsl:template>

	<xsl:template match="param" mode="bigfile">
		<i><xsl:value-of select="."/></i>
	</xsl:template>
	
	<xsl:template match="example" mode="bigfile">
		<p><xsl:apply-templates select="desc" mode="bigfile"/></p>
		<p><xsl:apply-templates select="code" mode="bigfile"/></p>
	</xsl:template>

	<xsl:template match="code" mode="bigfile">
		<pre><xsl:value-of select="."/></pre>
	</xsl:template>

	<xsl:template match="object" mode="bigfile">
		<h3>Object <xsl:value-of select="@name"/></h3>
		<xsl:apply-templates select="desc" mode="bigfile"/>
		<xsl:apply-templates select="property" mode="bigfile"/>
		<xsl:apply-templates select="method" mode="bigfile"/>
	</xsl:template>
	
	<xsl:template match="property" mode="bigfile">
		<p>
		<xsl:value-of select="@type"/>
		<xsl:text> </xsl:text>
		<b><xsl:value-of select="@name"/></b>
		<xsl:text> - </xsl:text>
		<xsl:apply-templates select="desc" mode="bigfile"/>
		<xsl:apply-templates select="note" mode="bigfile"/>
		</p>
	</xsl:template>

	<xsl:template match="property/note" mode="bigfile">
		<b> Note: </b><xsl:apply-templates select="text()|param|const" mode="bigfile"/>
	</xsl:template>

</xsl:stylesheet>
