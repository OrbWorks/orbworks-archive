<?xml version="1.0" ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<xsl:output method="html" encoding="utf-8" />
<xsl:variable name="newline">
<xsl:text>
</xsl:text>
</xsl:variable>

	<xsl:template match="Documentation">
		<!-- HHC Template Header [Starts Here] -->
		<HTML>
			<xsl:value-of select="$newline" />
			<HEAD>
				<xsl:value-of select="$newline" />
			</HEAD>
			<xsl:value-of select="$newline" />
			<BODY>
				<xsl:value-of select="$newline" />
				<OBJECT type="text/site properties">
					<xsl:value-of select="$newline" />
					<param name="Window Styles" value="0x800025" />
					<xsl:value-of select="$newline" />
					<param name="comment" value="title:" />
					<xsl:value-of select="$newline" />
					<param name="comment" value="base:" />
					<xsl:value-of select="$newline" />
				</OBJECT>
				<xsl:value-of select="$newline" />
				<UL>
					<xsl:value-of select="$newline" />
					<!-- HHC Template Header [Ends Here] -->
					<xsl:apply-templates select="book" />
					<!-- HHC Template Footer [Starts Here] -->
				</UL>
				<xsl:value-of select="$newline" />
			</BODY>
			<xsl:value-of select="$newline" />
		</HTML>
		<xsl:value-of select="$newline" />
		<!-- HHC Template Footer [Ends Here] -->
	</xsl:template>
	
	<xsl:template match="book">
		<LI>
			<OBJECT type="text/sitemap">
				<xsl:value-of select="$newline" />
				<param>
					<xsl:attribute name="name">
						<xsl:text>Name</xsl:text>
					</xsl:attribute>
					<xsl:attribute name="value">
						<xsl:value-of select="@title" />
					</xsl:attribute>
				</param>
				<xsl:value-of select="$newline" />
				<param>
					<xsl:attribute name="name">
						<xsl:text>Local</xsl:text>
					</xsl:attribute>
					<xsl:attribute name="value">
						<xsl:text>b</xsl:text>
						<xsl:value-of select="@name" />
						<xsl:text>.html</xsl:text>
					</xsl:attribute>
				</param>
				<xsl:value-of select="$newline" />
			</OBJECT>
			<xsl:value-of select="$newline" />
		</LI>
		<xsl:value-of select="$newline" />
		<UL>
			<xsl:value-of select="$newline" />
			<xsl:for-each select="topic">
				<LI>
					<OBJECT type="text/sitemap">
						<param>
							<xsl:attribute name="name">
								<xsl:text>Name</xsl:text>
							</xsl:attribute>
							<xsl:attribute name="value">
								<xsl:variable name="title">
									<xsl:choose>
										<xsl:when test="@title">
											<xsl:value-of select="@title" />
										</xsl:when>
										<xsl:otherwise>
											<xsl:value-of select="@name" />
										</xsl:otherwise>
									</xsl:choose>
								</xsl:variable>
								<xsl:value-of select="$title" />
							</xsl:attribute>
						</param>
						<xsl:value-of select="$newline" />
						<param>
							<xsl:attribute name="name">
								<xsl:text>Local</xsl:text>
							</xsl:attribute>
							<xsl:attribute name="value">
								<xsl:text>t</xsl:text>
								<xsl:value-of select="@name" />
								<xsl:text>.html</xsl:text>
							</xsl:attribute>
						</param>
						<xsl:value-of select="$newline" />
					</OBJECT>
					<xsl:value-of select="$newline" />
				</LI>
				<xsl:value-of select="$newline" />
			</xsl:for-each>
			<xsl:for-each select="category">
				<LI>
					<OBJECT type="text/sitemap">
						<param>
							<xsl:attribute name="name">
								<xsl:text>Name</xsl:text>
							</xsl:attribute>
							<xsl:attribute name="value">
								<xsl:variable name="title">
									<xsl:choose>
										<xsl:when test="@title">
											<xsl:value-of select="@title" />
										</xsl:when>
										<xsl:otherwise>
											<xsl:value-of select="@name" />
										</xsl:otherwise>
									</xsl:choose>
								</xsl:variable>
								<xsl:value-of select="$title" />
							</xsl:attribute>
						</param>
						<xsl:value-of select="$newline" />
						<param>
							<xsl:attribute name="name">
								<xsl:text>Local</xsl:text>
							</xsl:attribute>
							<xsl:attribute name="value">
								<xsl:text>c</xsl:text>
								<xsl:value-of select="@name" />
								<xsl:text>.html</xsl:text>
							</xsl:attribute>
						</param>
						<xsl:value-of select="$newline" />
					</OBJECT>
					<xsl:value-of select="$newline" />
				</LI>
				<xsl:value-of select="$newline" />
				<UL>
					<xsl:value-of select="$newline" />
					<xsl:for-each select="object">
						<xsl:call-template name="processObject">
							<xsl:with-param name="name" select="@name" />
						</xsl:call-template>
					</xsl:for-each>
				</UL>
				<xsl:value-of select="$newline" />
				<UL>
					<xsl:value-of select="$newline" />
					<xsl:for-each select="method">
						<xsl:call-template name="processMethod">
							<xsl:with-param name="name" select="@name" />
						</xsl:call-template>
					</xsl:for-each>
				</UL>
				<xsl:value-of select="$newline" />
				<UL>
					<xsl:value-of select="$newline" />
					<xsl:for-each select="topic">
						<xsl:call-template name="processTopic">
							<xsl:with-param name="name" select="@name" />
							<xsl:with-param name="title" select="@title" />
						</xsl:call-template>
					</xsl:for-each>
				</UL>
				<xsl:value-of select="$newline" />
			</xsl:for-each>
		</UL>
		<xsl:value-of select="$newline" />
	</xsl:template>
	
	<xsl:template name="processMethod">
		<xsl:param name="name" />
		<LI>
			<OBJECT type="text/sitemap">
				<param>
					<xsl:attribute name="name">
						<xsl:text>Name</xsl:text>
					</xsl:attribute>
					<xsl:attribute name="value">
						<xsl:value-of select="$name" />
					</xsl:attribute>
				</param>
				<xsl:value-of select="$newline" />
				<param>
					<xsl:attribute name="name">
						<xsl:text>Local</xsl:text>
					</xsl:attribute>
					<xsl:attribute name="value">
						<xsl:text>m</xsl:text>
						<xsl:value-of select="$name" />
						<xsl:text>.html</xsl:text>
					</xsl:attribute>
				</param>
				<xsl:value-of select="$newline" />
			</OBJECT>
			<xsl:value-of select="$newline" />
		</LI>
		<xsl:value-of select="$newline" />
	</xsl:template>
	
	<xsl:template name="processObject">
		<xsl:param name="name" />
		<LI>
			<OBJECT type="text/sitemap">
				<param>
					<xsl:attribute name="name">
						<xsl:text>Name</xsl:text>
					</xsl:attribute>
					<xsl:attribute name="value">
						<xsl:value-of select="$name" />
					</xsl:attribute>
				</param>
				<xsl:value-of select="$newline" />
				<param>
					<xsl:attribute name="name">
						<xsl:text>Local</xsl:text>
					</xsl:attribute>
					<xsl:attribute name="value">
						<xsl:text>o</xsl:text>
						<xsl:value-of select="$name" />
						<xsl:text>.html</xsl:text>
					</xsl:attribute>
				</param>
				<xsl:value-of select="$newline" />
			</OBJECT>
			<xsl:value-of select="$newline" />
		</LI>
		<xsl:value-of select="$newline" />
		<UL>
			<xsl:value-of select="$newline" />
			<xsl:for-each select="method">
				<xsl:call-template name="processObjectMethod">
					<xsl:with-param name="name" select="@name" />
					<xsl:with-param name="objectname" select="../@name" />
				</xsl:call-template>
			</xsl:for-each>
		</UL>
		<xsl:value-of select="$newline" />
	</xsl:template>
	
	<xsl:template name="processObjectMethod">
		<xsl:param name="name" />
		<xsl:param name="objectname" />
		<LI>
			<OBJECT type="text/sitemap">
				<param>
					<xsl:attribute name="name">
						<xsl:text>Name</xsl:text>
					</xsl:attribute>
					<xsl:attribute name="value">
						<xsl:value-of select="$name" />
					</xsl:attribute>
				</param>
				<xsl:value-of select="$newline" />
				<param>
					<xsl:attribute name="name">
						<xsl:text>Local</xsl:text>
					</xsl:attribute>
					<xsl:attribute name="value">
						<xsl:text>m</xsl:text>
						<xsl:value-of select="$objectname" />
						<xsl:text>-</xsl:text>
						<xsl:value-of select="$name" />
						<xsl:text>.html</xsl:text>
					</xsl:attribute>
				</param>
				<xsl:value-of select="$newline" />
			</OBJECT>
			<xsl:value-of select="$newline" />
		</LI>
		<xsl:value-of select="$newline" />
	</xsl:template>
	
	<xsl:template name="processTopic">
		<xsl:param name="name" />
		<xsl:param name="title" />
		<LI>
			<OBJECT type="text/sitemap">
				<param>
					<xsl:attribute name="name">
						<xsl:text>Name</xsl:text>
					</xsl:attribute>
					<xsl:attribute name="value">
						<xsl:value-of select="$title" />
					</xsl:attribute>
				</param>
				<xsl:value-of select="$newline" />
				<param>
					<xsl:attribute name="name">
						<xsl:text>Local</xsl:text>
					</xsl:attribute>
					<xsl:attribute name="value">
						<xsl:text>t</xsl:text>
						<xsl:value-of select="$name" />
						<xsl:text>.html</xsl:text>
					</xsl:attribute>
				</param>
				<xsl:value-of select="$newline" />
			</OBJECT>
			<xsl:value-of select="$newline" />
		</LI>
		<xsl:value-of select="$newline" />
	</xsl:template>
</xsl:stylesheet>
