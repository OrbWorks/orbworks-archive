@echo off
setlocal
set BASE_DIR=%CD%
set TODAY=%DATE:/=-%
set TODAY=%TODAY:~4%
set W32_SRC=C:\Development\PocketC
set EMU_SRC=C:\Development\OrbForms\bin
set PRC_SRC=C:\Development\PocketC\PocketC-Palm
set COPY_OPTIONS=/q /i /exclude:setup\excludesvn.txt

REM Create backup
REM -------------

if exist ilovepde.zip rename ilovepde.zip PDE.zip
set DEST="%BASE_DIR%\Backup\%TODAY%"
if exist %DEST% goto NO_BACKUP

cd Backup
md %TODAY%
cd ..
echo Copying old releases to %DEST%
xcopy /qy PocketC.zip %DEST%
xcopy /qy PocketC_rt.zip %DEST%
xcopy /qy PocketC_retail.zip %DEST%
xcopy /qy PDE.zip %DEST%
xcopy /qy PDEdemo.zip %DEST%
xcopy /qy PocketCScan.zip %DEST%
goto RELEASE_DIRS

:NO_BACKUP
echo %DEST% exists, no backup done.

:RELEASE_DIRS

REM Build PocketC device code
echo Building device bits
wscript buildCW.js

REM Update bins for standalone
echo Updating PDE standalone binaries
pushd ..\PocketC
call grabBins.cmd

REM Build PDE
echo Build tools
C:\Tools\VS8\Common7\IDE\devenv /nologo /build Release /project DatGen PDE\PocketC.sln
C:\Tools\VS8\Common7\IDE\devenv /nologo /build Release /project bytecode PDE\PocketC.sln
C:\Tools\VS8\Common7\IDE\devenv /nologo /build Release /project pc2html PDE\PocketC.sln
C:\Tools\VS8\Common7\IDE\devenv /nologo /build Release /project PDE PDE\PocketC.sln
C:\Tools\VS8\Common7\IDE\devenv /nologo /build "Demo Release" /project PDE PDE\PocketC.sln
popd

REM Create new release dirs
REM -----------------------
echo Building new release dirs

REM PocketC_Dist
set DEST="%BASE_DIR%"\PocketC_Dist
if exist %DEST% erase /q /s %DEST%
md %DEST%
xcopy /qy %PRC_SRC%\PocketC.prc %DEST%
xcopy DistSrc\_PocketC\* %DEST% %COPY_OPTIONS%
md %DEST%\docs
xcopy DistSrc\docs %DEST%\docs %COPY_OPTIONS%
md %DEST%\samples
xcopy DistSrc\samples %DEST%\samples %COPY_OPTIONS%
md %DEST%\PocketCLib
xcopy DistSrc\PocketCLib %DEST%\PocketCLib /s %COPY_OPTIONS%
md %DEST%\UtilCode
xcopy DistSrc\UtilCode %DEST%\UtilCode /s %COPY_OPTIONS%

REM Retail_Dist
set DEST="%BASE_DIR%"\Retail_Dist
if exist %DEST% erase /q /s %DEST%
md %DEST%
xcopy /qy %PRC_SRC%\Retail\PocketC.prc %DEST%
xcopy DistSrc\_PocketC\* %DEST% %COPY_OPTIONS%
md %DEST%\docs
xcopy DistSrc\docs %DEST%\docs %COPY_OPTIONS%
md %DEST%\samples
xcopy DistSrc\samples %DEST%\samples %COPY_OPTIONS%
md %DEST%\PocketCLib
xcopy DistSrc\PocketCLib %DEST%\PocketCLib /s %COPY_OPTIONS%
md %DEST%\UtilCode
xcopy DistSrc\UtilCode %DEST%\UtilCode /s %COPY_OPTIONS%


REM Runtime_Dist
set DEST="%BASE_DIR%"\Runtime_Dist
if exist %DEST% erase /q /s %DEST%
md %DEST%
xcopy /qy %PRC_SRC%\Runtime\PocketC.prc %DEST%
xcopy DistSrc\_Runtime\* %DEST% %COPY_OPTIONS%

REM build the standard funcs.dat
pushd %W32_SRC%
DatGen
popd
REM PDE_Dist
set DEST="%BASE_DIR%"\PDE_Dist
if exist %DEST% erase /q /s %DEST%
md %DEST%
xcopy DistSrc\_PDE\* %DEST% %COPY_OPTIONS%
xcopy DistSrc\_PDEretail\* %DEST% %COPY_OPTIONS%
xcopy /qy %W32_SRC%\pde.exe %DEST%
xcopy /qy %EMU_SRC%\emuctrl.dll %DEST%
xcopy /qy %W32_SRC%\pc2html.exe %DEST%
xcopy /qy %W32_SRC%\funcs.dat %DEST%
md %DEST%\docs
xcopy DistSrc\docs %DEST%\docs %COPY_OPTIONS%
xcopy DistSrc\PDE_docs %DEST%\docs %COPY_OPTIONS%
md %DEST%\samples
xcopy DistSrc\samples %DEST%\samples %COPY_OPTIONS%
md %DEST%\PocketCLib
xcopy DistSrc\PocketCLib %DEST%\PocketCLib /s %COPY_OPTIONS%
md %DEST%\UtilCode
xcopy DistSrc\UtilCode %DEST%\UtilCode /s %COPY_OPTIONS%
md %DEST%\device_rt
xcopy DistSrc\_Runtime\* %DEST%\device_rt %COPY_OPTIONS%
xcopy /qy %PRC_SRC%\Runtime\PocketC.prc %DEST%\device_rt

REM PDEdemo_Dist
set DEST="%BASE_DIR%"\PDEdemo_Dist
if exist %DEST% erase /q /s %DEST%
md %DEST%
xcopy DistSrc\_PDE\* %DEST% %COPY_OPTIONS%
xcopy DistSrc\_PDEdemo\* %DEST% %COPY_OPTIONS%
xcopy /qy %W32_SRC%\PDE\Demo\pde.exe %DEST%
xcopy /qy %EMU_SRC%\emuctrl.dll %DEST%
xcopy /qy %W32_SRC%\pc2html.exe %DEST%
xcopy /qy %W32_SRC%\funcs.dat %DEST%
md %DEST%\docs
xcopy DistSrc\docs %DEST%\docs %COPY_OPTIONS%
xcopy DistSrc\PDE_docs %DEST%\docs %COPY_OPTIONS%
md %DEST%\samples
xcopy DistSrc\samples %DEST%\samples %COPY_OPTIONS%
md %DEST%\PocketCLib
xcopy DistSrc\PocketCLib %DEST%\PocketCLib /s %COPY_OPTIONS%
md %DEST%\UtilCode
xcopy DistSrc\UtilCode %DEST%\UtilCode /s %COPY_OPTIONS%
md %DEST%\device_rt
xcopy DistSrc\_Runtime\* %DEST%\device_rt %COPY_OPTIONS%
xcopy /qy %PRC_SRC%\Runtime\PocketC.prc %DEST%\device_rt

REM build the scanner funcs.dat
pushd %W32_SRC%
DatGen -scan
popd
REM Scanner_Dist
set DEST="%BASE_DIR%"\Scanner_Dist
if exist %DEST% erase /q /s %DEST%
md %DEST%
xcopy /qy /s "%BASE_DIR%\PDE_Dist" %DEST%
xcopy DistSrc\_Scanner %DEST% %COPY_OPTIONS%
xcopy DistSrc\scanner_docs %DEST%\docs %COPY_OPTIONS%
erase /q %DEST%\device_rt\*
xcopy /qy %PRC_SRC%\ScanRuntime\PocketC.prc %DEST%\device_rt
xcopy /qy %W32_SRC%\funcs.dat %DEST%


REM Erase/Create new ZIP files
REM --------------------------
echo Creating new .zip files
erase /q PocketC.zip
erase /q PocketC_rt.zip
erase /q PocketC_retail.zip
erase /q pde.zip
erase /q pdedemo.zip
erase /q PocketCScan.zip

cd PocketC_Dist
wzzip -rp ..\PocketC.zip * > NUL
cd ..
erase /s /q PocketC_Dist
cd Retail_Dist
wzzip -rp ..\PocketC_retail.zip * > NUL
cd ..
erase /s /q Retail_Dist
cd Runtime_Dist
wzzip -rp ..\PocketC_rt.zip * > NUL
cd ..
erase /s /q Runtime_Dist
cd PDE_Dist
copy ..\setup\*
..\..\tools\nsis\Makensis /O..\PDE.installer.log pde.nsi
wzzip ..\pde.zip ..\PDE-setup.exe > NUL
cd ..
erase /s /q PDE_Dist
erase PDE-setup.exe
cd PDEdemo_Dist
copy ..\setup\*
..\..\tools\nsis\Makensis /O..\PDEdemo.installer.log pde.nsi
rename ..\PDE-setup.exe PDE-demo-setup.exe
wzzip ..\pdedemo.zip ..\PDE-demo-setup.exe > NUL
cd ..
erase /s /q PDEdemo_Dist
erase PDE-demo-setup.exe
cd Scanner_Dist
wzzip -rp ..\PocketCScan.zip * > NUL
cd ..
erase /s /q Scanner_Dist


:DONE
endlocal
exit

:BDIR_ERROR
echo ERROR: %DEST% already exists
goto DONE