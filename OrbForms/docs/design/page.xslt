<?xml version="1.0" encoding="UTF-8" ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" encoding="utf-8"/>

<xsl:template match="page">
<html><head>
</head><body>
<!-- main page layout -->
<div id="Topic">
<ul>
<xsl:apply-templates select="section" mode="build-toc"/>
</ul>
</div>
<xsl:apply-templates select="section" mode="build-content"/>
</body></html>
</xsl:template>

<xsl:template match="title">
<xsl:choose>
<xsl:when test="@style"><div id="{@style}"><div id="Subject"><xsl:value-of select="."/></div></div></xsl:when>
<xsl:otherwise><div id="Subject"><xsl:value-of select="."/></div>
</xsl:otherwise>
</xsl:choose>
</xsl:template>

<!-- build topic section -->
<xsl:template match="section" mode="build-toc">
<div id="TopicIndex"><li><A href="#{../@id}{position()}"><xsl:value-of select="title"/></A></li></div>
</xsl:template>

<!-- build section -->
<xsl:template match="section" mode="build-content">
<div id="Content">
<A name="{../@id}{position()}"><H3><xsl:value-of select="title"/></H3></A>
<xsl:apply-templates select="paragraph" mode="build-content"/>
<xsl:apply-templates select="picture" mode="build-content"/>
</div>
</xsl:template>

<!-- display each paragraph -->
<xsl:template match="paragraph" mode="build-content">
<xsl:choose>
<xsl:when test="@code"><pre><xsl:value-of select="."/></pre></xsl:when>
<xsl:otherwise><p><xsl:value-of select="."/></p>
<xsl:apply-templates select="picture" mode="build-content"/>
<xsl:apply-templates select="registereduser_downloadform"/>
</xsl:otherwise>
</xsl:choose>
<xsl:apply-templates select="list"></xsl:apply-templates>
<xsl:apply-templates select="link"></xsl:apply-templates>
</xsl:template>



<!-- display each picture -->
<xsl:template match="picture" mode="build-content">
<a href="{@large}"><img src="{@small}" width="180" height="120" border="1" /></a>
<br></br>
</xsl:template>

<xsl:template match="link">
<a href="{@url}"><xsl:value-of select="@text"/></a>
</xsl:template>

<xsl:template match="list">
<ul><xsl:apply-templates select="line"></xsl:apply-templates></ul>
</xsl:template>

<xsl:template match="line">
<li><xsl:value-of select="@text"/></li>
</xsl:template>



</xsl:stylesheet>


