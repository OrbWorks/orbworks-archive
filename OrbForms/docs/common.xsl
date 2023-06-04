<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

	<!-- Create the function declaration -->

	<xsl:template name="funcDecl">
		<xsl:param name="useBold" select="true()"/>
		<xsl:param name="showNames" select="true()"/>
		<xsl:param name="addLink" select="false()"/>
		<xsl:param name="node" select="false()"/>
		<xsl:value-of select="return/@type"/>
		<xsl:text> </xsl:text>
		<xsl:choose>
			<xsl:when test="$useBold">
				<xsl:choose>
				<xsl:when test="$addLink">
					<b>
					<xsl:call-template name="genLink">
						<xsl:with-param name="node" select="$node"/>
						<xsl:with-param name="text" select="@name"/>
					</xsl:call-template>
					</b>
				</xsl:when>
				<xsl:otherwise>
					<b><xsl:value-of select="@name"/></b>
				</xsl:otherwise>
				</xsl:choose>
			</xsl:when>
			<xsl:otherwise>
				<xsl:choose>
				<xsl:when test="$addLink">
					<xsl:call-template name="genLink">
						<xsl:with-param name="node" select="$node"/>
						<xsl:with-param name="text" select="@name"/>
					</xsl:call-template>
				</xsl:when>
				<xsl:otherwise>
					<xsl:value-of select="@name"/>
				</xsl:otherwise>
				</xsl:choose>
			</xsl:otherwise>
		</xsl:choose>
		<xsl:text>(</xsl:text>
		<xsl:for-each select="param">
			<xsl:value-of select="@type"/>
			<xsl:if test="$showNames">
				<xsl:text> </xsl:text>
				<xsl:value-of select="@name"/>
			</xsl:if>
			<xsl:if test="following-sibling::param">, </xsl:if>
		</xsl:for-each>
		<xsl:text>)</xsl:text>
	</xsl:template>

	<!-- Generate the path for a reference -->

	<xsl:template name="genPath">
		<xsl:param name="node"/>
		<xsl:choose>
			<xsl:when test="name($node)='method' and name($node/..)='object'">
				<xsl:text>m</xsl:text><xsl:value-of select="$node/../@name"/>-<xsl:value-of select="$node/@name"/>
			</xsl:when>
			<xsl:when test="name($node)='method'">
				<xsl:text>m</xsl:text><xsl:value-of select="@name"/>
			</xsl:when>
			<xsl:when test="name($node)='toc'">
				<xsl:text>index</xsl:text>
			</xsl:when>
			<xsl:when test="name($node)='book'">
				<xsl:text>b</xsl:text><xsl:value-of select="@name"/>
			</xsl:when>
			<xsl:when test="name($node)='category'">
				<xsl:text>c</xsl:text><xsl:value-of select="@name"/>
			</xsl:when>
			<xsl:when test="name($node)='topic'">
				<xsl:text>t</xsl:text><xsl:value-of select="@name"/>
			</xsl:when>
			<xsl:when test="name($node)='object'">
				<xsl:text>o</xsl:text><xsl:value-of select="@name"/>
			</xsl:when>
		</xsl:choose>
		<xsl:text>.html</xsl:text>
	</xsl:template>

	<xsl:template name="genBook">
		<xsl:param name="node"/>
		<xsl:choose>
			<xsl:when test="name($node)='method' and name($node/..)='object'">
				<xsl:value-of select="$node/../../../@name"/>
				<xsl:text>-</xsl:text>
				<xsl:value-of select="$node/../../@name"/>
			</xsl:when>
			<xsl:when test="name($node)='method'">
				<xsl:value-of select="$node/../../@name"/>
				<xsl:text>-</xsl:text>
				<xsl:value-of select="$node/../@name"/>
			</xsl:when>
			<xsl:when test="name($node)='toc'">
				<xsl:text>index</xsl:text>
			</xsl:when>
			<xsl:when test="name($node)='book'">
				<xsl:value-of select="@name"/>
			</xsl:when>
			<xsl:when test="name($node)='category'">
				<xsl:value-of select="$node/../@name"/>
				<xsl:text>-</xsl:text>
				<xsl:value-of select="@name"/>
			</xsl:when>
			<xsl:when test="name($node)='topic' and name($node/..)='category'">
				<xsl:value-of select="$node/../../@name"/>
				<xsl:text>-</xsl:text>
				<xsl:value-of select="$node/../@name"/>
			</xsl:when>
			<xsl:when test="name($node)='topic'">
				<xsl:value-of select="$node/../@name"/>
			</xsl:when>
			<xsl:when test="name($node)='object'">
				<xsl:value-of select="$node/../../@name"/>
				<xsl:text>-</xsl:text>
				<xsl:value-of select="$node/../@name"/>
			</xsl:when>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="desc">
		<xsl:apply-templates select="text()|*"/>
	</xsl:template>

	 <!-- All desc items are explicitly defined, so that we can use desc/* to copy HTML -->

	 <xsl:template match="desc/*">
		<xsl:copy-of select="."/>
	</xsl:template>

	<xsl:template match="note">
		<b> Note: </b><xsl:apply-templates select="text()|*"/>
	</xsl:template>

	<xsl:template match="desc/note">
		<b> Note: </b><xsl:apply-templates select="text()|*"/>
	</xsl:template>

	<xsl:template match="param">
		<i><xsl:value-of select="."/></i>
	</xsl:template>

	<xsl:template match="desc/param">
		<i><xsl:value-of select="."/></i>
	</xsl:template>

	<xsl:template match="const">
		<i><xsl:value-of select="."/></i>
	</xsl:template>

	<xsl:template match="desc/const">
		<i><xsl:value-of select="."/></i>
	</xsl:template>

	<xsl:template match="xref">
		<xsl:variable name="fname">
			<xsl:call-template name="genPath">
				<xsl:with-param name="node" select="@node"/>
			</xsl:call-template>
		</xsl:variable>

		<a href="{@node}.html"><xsl:value-of select="text()"/></a>
	</xsl:template>

	<xsl:template match="desc/xref">
		<xsl:variable name="fname">
			<xsl:call-template name="genPath">
				<xsl:with-param name="node" select="@node"/>
			</xsl:call-template>
		</xsl:variable>

		<a href="{@node}.html"><xsl:value-of select="text()"/></a>
	</xsl:template>

	<xsl:template match="example">
		<p><b>Example:</b></p>
		<p><xsl:apply-templates select="desc"/></p>
		<p><xsl:apply-templates select="code"/></p>
	</xsl:template>

	<xsl:template match="code">
		<table border="1" bordercolor="#000000" cellspacing="0" cellpadding="3" bgcolor="#E0E0E0" width="100%">
			<tr><td><pre><xsl:value-of select="."/></pre></td></tr>
		</table>
	</xsl:template>

	<xsl:template name="genLink">
		<xsl:param name="node"/>
		<xsl:param name="text"/>

		<xsl:variable name="fname">
			<xsl:call-template name="genPath">
				<xsl:with-param name="node" select="$node"/>
			</xsl:call-template>
		</xsl:variable>

		<a href="{$fname}"><xsl:value-of select="$text"/></a>
	</xsl:template>

	<!-- Navigation bar -->

	<xsl:template name="genNavBar">
		<table width="100%" bgcolor="#E0E0FF" border="0">
		<tr><td>
			<xsl:apply-templates select="." mode="navbar">
				<xsl:with-param name="current" select="true()"/>
			</xsl:apply-templates>
		</td></tr>
		</table>
	</xsl:template>

	<xsl:template match="Documentation" mode="navbar">
		<xsl:param name="current" select="false()"/>
		<xsl:choose>
		<xsl:when test="$current">
			<xsl:text>Contents</xsl:text>
		</xsl:when>
		<xsl:otherwise>
			<xsl:call-template name="genLink">
				<xsl:with-param name="node" select="toc"/>
				<xsl:with-param name="text" select="'Contents'"/>
			</xsl:call-template>
		</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="book" mode="navbar">
		<xsl:param name="current" select="false()"/>
		<xsl:apply-templates select=".." mode="navbar"/>

		<xsl:choose>
		<xsl:when test="$current">
			<xsl:text> &gt; </xsl:text>
			<xsl:value-of select="@title"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:text> &gt; </xsl:text>
			<xsl:call-template name="genLink">
				<xsl:with-param name="node" select="."/>
				<xsl:with-param name="text" select="@title"/>
			</xsl:call-template>
		</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="category" mode="navbar">
		<xsl:param name="current" select="false()"/>

		<xsl:apply-templates select=".." mode="navbar"/>

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

		<xsl:choose>
		<xsl:when test="$current">
			<xsl:text> &gt; </xsl:text>
			<xsl:value-of select="$title"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:text> &gt; </xsl:text>
			<xsl:call-template name="genLink">
				<xsl:with-param name="node" select="."/>
				<xsl:with-param name="text" select="$title"/>
			</xsl:call-template>
		</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="object" mode="navbar">
		<xsl:param name="current" select="false()"/>

		<xsl:apply-templates select=".." mode="navbar"/>

		<xsl:choose>
		<xsl:when test="$current">
			<xsl:text> &gt; </xsl:text>
			<xsl:value-of select="@name"/>
			<xsl:text> object</xsl:text>
		</xsl:when>
		<xsl:otherwise>
			<xsl:text> &gt; </xsl:text>
			<xsl:call-template name="genLink">
				<xsl:with-param name="node" select="."/>
				<xsl:with-param name="text">
					<xsl:value-of select="@name"/>
					<xsl:text> object</xsl:text>
				</xsl:with-param>
			</xsl:call-template>
		</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="method" mode="navbar">
		<xsl:param name="current" select="false()"/>

		<xsl:apply-templates select=".." mode="navbar"/>

		<xsl:choose>
		<xsl:when test="$current">
			<xsl:text> &gt; </xsl:text>
			<xsl:value-of select="@name"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:text> &gt; </xsl:text>
			<xsl:call-template name="genLink">
				<xsl:with-param name="node" select="."/>
				<xsl:with-param name="text" select="@name"/>
			</xsl:call-template>
		</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="topic" mode="navbar">
		<xsl:param name="current" select="false()"/>

		<xsl:apply-templates select=".." mode="navbar"/>

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

		<xsl:choose>
		<xsl:when test="$current">
			<xsl:text> &gt; </xsl:text>
			<xsl:value-of select="$title"/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:text> &gt; </xsl:text>
			<xsl:call-template name="genLink">
				<xsl:with-param name="node" select="."/>
				<xsl:with-param name="text" select="$title"/>
			</xsl:call-template>
		</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

</xsl:stylesheet>
