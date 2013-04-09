<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="2.0">

    <xsl:output indent="yes"/>

    <xsl:template match="/Results">
        <testsuites>
            <xsl:for-each-group select="*" group-by="Name">
                <!-- xsl:variable name="buildName" select="//TestRun/@name"/ -->
                <testsuite name="{current-grouping-key()}">

                    <xsl:attribute name="time"><xsl:value-of select="Time/@real" /> </xsl:attribute>
                    <xsl:if test="name(.)='Example'"> 
                        <xsl:attribute name="tests" select="count(Name)"/>
                        <xsl:attribute name="failures" select="count(Result[.='FAIL'])" />
                        <xsl:attribute name="errors" select="count(Result[.='CRASH'])" />
                        <xsl:attribute name="skipped" select="count(Name[contains(., 'SKIP')])" />
                        <testcase>
                          <xsl:attribute name="classname">Example.<xsl:value-of select="Name" /> </xsl:attribute>
                          <xsl:attribute name="name"> <xsl:value-of select="Name" /> </xsl:attribute>
                          <xsl:attribute name="time"><xsl:value-of select="Time/@real" /> </xsl:attribute>
                          <xsl:variable name="result"><xsl:value-of select="Result" /> </xsl:variable>
                          <xsl:variable name="crash_message"><xsl:value-of select="CrashDetails" /> </xsl:variable>
                          <xsl:if test="contains($result, 'CRASH')">
                            <failure>
                              <xsl:value-of select="$crash_message" />
                            </failure>
                          </xsl:if>
                        </testcase>
                    </xsl:if>

                    <xsl:if test="name(.)='Test'"> 
                        <xsl:attribute name="tests" select="count(*/Name)"/>
                        <xsl:attribute name="failures" select="count(*/Result[.='FAIL'])" />
                        <xsl:attribute name="errors" select="count(Result[.='CRASH'])" />
                        <xsl:attribute name="skipped" select="count(*/Name[contains(., 'SKIP')])" />
                        <xsl:variable name="result"><xsl:value-of select="Result" /> </xsl:variable>
                        <xsl:variable name="crash_message"><xsl:value-of select="CrashDetails" /> </xsl:variable>
                        <xsl:if test="contains($result, 'CRASH')">
                          <failure>
                            <xsl:value-of select="$crash_message" />
                          </failure>
                        </xsl:if>
                        <xsl:for-each select="Test">
                            <testcase classname="Test.{current-grouping-key()}">
                              <xsl:variable name="tmpname1"><xsl:value-of select="Name" /> </xsl:variable>
                              <xsl:attribute name="name">
                                <xsl:if test="contains($tmpname1, 'process ')">
                                  <xsl:value-of select="substring-before(substring-after($tmpname1, 'process '), ' complete')" />
                                </xsl:if>
                                <xsl:if test="not (contains($tmpname1, 'process '))">
                                  <xsl:value-of select="Name" />
                                </xsl:if>
                              </xsl:attribute>
                              <xsl:attribute name="time"><xsl:value-of select="Time/@real" /> </xsl:attribute>
                              <xsl:variable name="result"><xsl:value-of select="Result" /> </xsl:variable>
                              <xsl:variable name="message"><xsl:value-of select="FailureDetails" /> </xsl:variable>
                              <xsl:if test="contains($result, 'FAIL')"> 
                                 <failure>
                                 <xsl:value-of select="$message" />
                                 </failure>
                              </xsl:if>
                              <xsl:if test="contains($tmpname1, 'SKIP')"> 
                                 <skipped>
                                 </skipped>
                              </xsl:if>
                            </testcase>
                        </xsl:for-each>
                    </xsl:if>
                </testsuite>
            </xsl:for-each-group>
        </testsuites>   
    </xsl:template>

</xsl:stylesheet>
