@echo off

echo Build Documentation

echo - clean output directory
md out
md outhtml
erase /q out\*
erase /q outhtml\*

echo - build faqs
cd faq
..\msxsl faq.xml faq.xslt -o ..\faq.html
xcopy /qy *.gif ..\out
xcopy /qy *.css ..\out
cd..

echo - process documentation xml

DocTool -product docs.xml orbdocs.xml OrbForms "OrbForms Designer"
msxsl orbdocs.xml docs.xsl -o out.html
DocTool -chmsplit out.html out
msxsl orbdocs.xml htmhlp_toc.xsl -o OrbForms.hhc
xcopy /qy OrbForms.hhc out
xcopy /qy *.gif out
xcopy /qy *.jpg out

copy prjdlghlp.txt out
dir /b out\*.html out\*.css prjdlghlp.txt > OrbFormsHelp_hhp_files.txt
copy OrbFormsHelp_hhp.txt+OrbFormsHelp_hhp_files.txt out\OrbForms.hhp
cd out
..\DocTool -chmindex OrbForms.hhp
hhc OrbForms.hhp
cd ..
echo - generating HTML books
DocTool -booksplit out.html outhtml
