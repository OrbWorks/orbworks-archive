@echo off

echo Build Documentation

echo - clean output directories
erase /q outpc\*
erase /q outhtmlpc\*

echo - process documentation xml
DocTool -product docs.xml pcdocs.xml PocketC "PocketC Architect"
msxsl pcdocs.xml docs.xsl -o outpc.html
DocTool -quickdocs pcdocs.xml outpc\QuickDocs.txt
DocTool -chmsplit outpc.html outpc
msxsl pcdocs.xml htmhlp_toc.xsl -o PocketC.hhc
xcopy /qy PocketC.hhc outpc
xcopy /qy *.gif outpc
xcopy /qy *.jpg outpc

dir /b outpc\*.html > PocketCHelp_hhp_files.txt
copy PocketCHelp_hhp.txt+PocketCHelp_hhp_files.txt outpc\PocketC.hhp
cd outpc
..\DocTool -chmindex PocketC.hhp
"C:\Program Files\HTML Help Workshop\hhc" PocketC.hhp
cd ..
echo - generating HTML books
DocTool -booksplit outpc.html outhtmlpc
