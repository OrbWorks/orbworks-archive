<?xml version="1.0" encoding="UTF-8" ?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" encoding="utf-8"/>
<xsl:template match="faq">
<html><head>
<title><xsl:value-of select="title"></xsl:value-of></title>
<!-- style defintiion-->
<link REL="StyleSheet" HREF="faq.css" TYPE="text/css"></link>
</head><body>

<!-- main page layout -->
<!--img src="faq.gif" width="280" height="20" border="0" alt="FAQs"/-->
<br></br><hr></hr>
<h1>Contents</h1>
<xsl:apply-templates select="section" mode="build-toc"/>
<hr></hr>
<h1>Answers</h1>
<xsl:apply-templates select="section" mode="build-content"/>
</body></html>
</xsl:template>

<!-- build question section -->
<xsl:template match="section" mode="build-toc">
<h2><xsl:value-of select="title"/></h2>
<ol><xsl:apply-templates select="content" mode="build-toc"/></ol>
</xsl:template>

<!-- build question reference -->
<xsl:template match="content" mode="build-toc">
<li>
<!-- Reference to the actual question -->
<!-- The bookmark value is sectionid + question index -->
<A href="#{../@id}{position()}"><xsl:value-of select="question"/></A>
</li>
</xsl:template>

<!-- build question section contents-->
<xsl:template match="section" mode="build-content">
<h2><xsl:value-of select="title"/></h2>
<ol><xsl:apply-templates select="content" mode="build-content"/></ol>
</xsl:template>

<!-- build question contents -->
<xsl:template match="content" mode="build-content">
<li>
<A name="{../@id}{position()}"><b><xsl:value-of select="question"/></b></A>
<br></br><xsl:value-of select="answer"/><br></br><br></br>
</li>
</xsl:template>

</xsl:stylesheet>


