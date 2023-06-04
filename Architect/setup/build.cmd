@echo off
rem Options:
rem noclean - incremental build
rem nobuild - do not build runtime/compiler/IDE, just gen docs and build package
rem bits    - build runtime only
rem demo    - build the demo version
rem device  - build device package only

rem Complete/Demo build copies all .dlls into the bin/bindemo dir

setlocal

if "%1"=="demo" (
set BIN_DIR=..\bindemo
set INSTALL_NAME=pocketc-demo
set TARGET_NAME="Demo"
set LICENSE_NAME="demoLicense.txt"
) else (
set BIN_DIR=..\bin
set INSTALL_NAME=pocketc
set TARGET_NAME="Release"
set LICENSE_NAME="license.txt"
)

set BUILD_TYPE=/rebuild
if "%1" == "noclean" set BUILD_TYPE=/build
if "%2" == "noclean" set BUILD_TYPE=/build

set BUILD=1
if "%1" == "nobuild" (
set BUILD_TYPE=
set BUILD=0
)
if "%2" == "nobuild" (
set BUILD_TYPE=
set BUILD=0
)

if "%BUILD_TYPE%"=="/rebuild" (
rd /q /s %BIN_DIR%
md %BIN_DIR%
)

pushd %BIN_DIR%

rem *************
rem Build runtime
rem *************

if "%BUILD%"=="1" (
echo *** [Building Runtime/Device compiler]
wscript ..\setup\buildCW.js

xcopy /qy ..\..\OrbForms\OrbFormsRT\obj\OrbFormsRT.prc
wzzip OrbFormsRT.zip OrbFormsRT.prc
rem xcopy will ask if this is a dir/file if we don't create the file first
type NUL > OrbFormsRT-debug.prc
xcopy /qy ..\..\OrbForms\OrbFormsRT\objDebug\OrbFormsRT.prc OrbFormsRT-debug.prc

rem ************************
rem Get standalone resources
rem ************************
echo *** [Breaking down code/data resources]
if not exist standaloneBits\. (
md standaloneBits
) else (
erase /q /s standaloneBits > NUL
)
pushd standaloneBits
xcopy /qy ..\..\..\OrbForms\OrbFormsRT\objFat\OrbFormsRT.prc
..\..\setup\deprc OrbFormsRT.prc
xcopy /qy code0000.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy code0001.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy code0002.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy data0000.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy Talt9000.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy Talt9001.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy Talt9002.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy Talt9003.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy Talt9004.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy Talt9005.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy MBAR9000.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy tFRM9000.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy tFRM9100.bin ..\..\..\OrbForms\compiler\res\standalone
xcopy /qy pref0000.bin ..\..\..\OrbForms\compiler\res\standalone
popd

if not exist launcherBits\. (
md launcherBits
) else (
erase /q /s launcherBits > NUL
)
pushd launcherBits
xcopy /qy ..\..\..\OrbForms\OrbLaunch\obj\Starter.prc
..\..\setup\deprc Starter.prc
xcopy /qy code0000.bin ..\..\..\OrbForms\compiler\res\launcher
xcopy /qy code0001.bin ..\..\..\OrbForms\compiler\res\launcher
xcopy /qy data0000.bin ..\..\..\OrbForms\compiler\res\launcher
xcopy /qy Talt1001.bin ..\..\..\OrbForms\compiler\res\launcher
popd

if "%1"=="bits" goto :eof
if "%1"=="device" goto skipdesktop

rem ***************
rem Build Architect
rem ***************

echo *** [Building Architect %TARGET_NAME%]
C:\Tools\VS8\Common7\IDE\devenv /nologo %BUILD_TYPE% %TARGET_NAME% ..\Desktop\Architect.sln

rem end if BUILD
)

:skipdesktop
rem **********
rem Build Docs
rem **********
echo *** [Building docs]
pushd ..\..\OrbForms\docs
call gendocspc
popd

echo *** [Copying docs]
xcopy /qy ..\..\OrbForms\docs\outpc\PocketC.chm
xcopy /qy ..\..\OrbForms\docs\outpc\QuickDocs.txt
if not exist HTMLdocs\. (
md HTMLdocs
) else (
erase /q /s HTMLdocs > NUL
)
xcopy /qy ..\..\OrbForms\docs\outhtmlpc\* HTMLdocs


rem ************
rem Copy samples
rem ************
echo *** [Copying samples]
if not exist samples\. (
md samples
) else (
erase /q /s samples > NUL
)
xcopy /qys /exclude:..\setup\excludecvs.txt+..\setup\exclude-ts.txt ..\samples\* samples
md samples\Add-ins
xcopy /qys /exclude:..\setup\excludecvs.txt+..\setup\exclude-ts.txt ..\samples\* samples
xcopy /qys /exclude:..\setup\excludecvs.txt+..\setup\exclude-ts.txt ..\..\OrbForms\samples\Add-ins\* samples\Add-ins


rem ************
rem Copy add-ins
rem ************
echo *** [Copying add-ins]
if not exist add-ins\. (
md add-ins
) else (
erase /q /s add-ins > NUL
)
xcopy /qys /exclude:..\setup\excludecvs.txt+..\setup\exclude-ts.txt ..\..\OrbForms\add-ins\* add-ins


rem ********************
rem Copy PocketC headers
rem ********************
echo *** [Copying PocketC headers]
if not exist PocketC\. (
md PocketC
) else (
erase /q /s PocketC > NUL
)
xcopy /qys /exclude:..\setup\excludecvs.txt+..\setup\exclude-ts.txt ..\PocketC\* PocketC


rem *************
rem Copy utilcode
rem *************
echo *** [Copying utilcode]
if not exist utilcode\. (
md utilcode
) else (
erase /q /s utilcode > NUL
)
xcopy /qys /exclude:..\setup\excludecvs.txt+..\setup\exclude-ts.txt ..\..\OrbForms\utilcode\* utilcode
xcopy /qys /exclude:..\setup\excludecvs.txt+..\setup\exclude-ts.txt ..\utilcode\* utilcode

if "%1"=="device" goto skipinstaller
rem ***************
rem Build installer
rem ***************
echo *** [Building installer "%INSTALL_NAME%.nsi"]
xcopy /qy ..\setup\*.nsi
xcopy /qy ..\setup\%LICENSE_NAME%
xcopy /qy ..\setup\setup_icon.ico
xcopy /qy ..\setup\MathLib.prc
xcopy /qy ..\setup\check.bmp
xcopy /qy ..\setup\uncheck.bmp
xcopy /qy ..\..\OrbForms\orbpdb\orbpdb.txt
xcopy /qy ..\setup\Palm256Palette.pal
xcopy /qy ..\setup\mfc80.dll
xcopy /qy ..\setup\msvcr80.dll
xcopy /qy ..\setup\msvcp80.dll
xcopy /qy ..\setup\*.manifest
xcopy /qy ..\setup\T3_DIA_Compatibility_prcs.zip

rem only increment the build number when building retail
if "%1"=="demo" (
..\..\tools\build\stampver -v"..\setup\version.inf" Architect.exe
..\..\tools\nsis\Makensis /Opocketc-demo.installer.log pocketc-demo
..\..\tools\nsis\Makensis /Opocketc-demoPG.installer.log pocketc-demoPG
) else (
..\..\tools\build\stampver -v"..\setup\version.inf" -i4 -j4 Architect.exe
..\..\tools\nsis\Makensis /Opocketc.installer.log pocketc.nsi
)
:skipinstaller
popd

rem ********************
rem Build device package
rem ********************
if not "%1"=="demo" (
echo *** [Building device package]
rd /q /s ..\bindevice
md ..\bindevice
pushd ..\bindevice
md HTMLdocs
xcopy /qys %BIN_DIR%\HTMLdocs HTMLdocs
md samples
xcopy /qys %BIN_DIR%\samples samples
md utilcode
xcopy /qys %BIN_DIR%\utilcode utilcode
md add-ins
xcopy /qys %BIN_DIR%\add-ins add-ins
md PocketC
xcopy /qys %BIN_DIR%\PocketC PocketC

xcopy /qy ..\PalmCompiler\PCA.prc
xcopy /qy ..\..\OrbForms\OrbFormsRT\obj\OrbFormsRT.prc
rem xcopy will ask if this is a dir/file if we don't create the file first
type NUL > OrbFormsRT-debug.prc
xcopy /qy ..\..\OrbForms\OrbFormsRT\objDebug\OrbFormsRT.prc OrbFormsRT-debug.prc
xcopy /qy ..\bin\PocketC.chm
xcopy /qys ..\device .
wzzip -rp PocketCArchitect.zip * > NUL

popd
)
rem end if not demo

endlocal
