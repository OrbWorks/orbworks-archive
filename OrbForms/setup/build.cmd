@echo off
rem Options:
rem noclean - incremental build
rem nobuild - do not build runtime/compiler/IDE, just gen docs and build package
rem bits    - build runtime only
rem demo    - build the demo version

rem Complete/Demo build copies all .dlls into the bin/bindemo dir

setlocal

if "%1"=="demo" (
set BIN_DIR=..\bindemo
set INSTALL_NAME=orbforms-demo
set TARGET_NAME="Demo"
set LICENSE_NAME="demoLicense.txt"
) else (
set BIN_DIR=..\bin
set INSTALL_NAME=orbforms
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
echo *** [Building Runtime]
wscript ..\setup\buildCW.js

xcopy /qy ..\OrbFormsRT\obj\OrbFormsRT.prc
wzzip OrbFormsRT.zip OrbFormsRT.prc
rem xcopy will ask if this is a dir/file if we don't create the file first
type NUL > OrbFormsRT-debug.prc
xcopy /qy ..\OrbFormsRT\objDebug\OrbFormsRT.prc OrbFormsRT-debug.prc

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
xcopy /qy ..\..\OrbFormsRT\objFat\OrbFormsRT.prc
..\..\setup\deprc OrbFormsRT.prc
xcopy /qy code0000.bin ..\..\compiler\res\standalone
xcopy /qy code0001.bin ..\..\compiler\res\standalone
xcopy /qy code0002.bin ..\..\compiler\res\standalone
xcopy /qy data0000.bin ..\..\compiler\res\standalone
xcopy /qy Talt9000.bin ..\..\compiler\res\standalone
xcopy /qy Talt9001.bin ..\..\compiler\res\standalone
xcopy /qy Talt9002.bin ..\..\compiler\res\standalone
xcopy /qy Talt9003.bin ..\..\compiler\res\standalone
xcopy /qy Talt9004.bin ..\..\compiler\res\standalone
xcopy /qy Talt9005.bin ..\..\compiler\res\standalone
xcopy /qy MBAR9000.bin ..\..\compiler\res\standalone
xcopy /qy tFRM9000.bin ..\..\compiler\res\standalone
xcopy /qy tFRM9100.bin ..\..\compiler\res\standalone
xcopy /qy pref0000.bin ..\..\compiler\res\standalone
popd

if not exist launcherBits\. (
md launcherBits
) else (
erase /q /s launcherBits > NUL
)
pushd launcherBits
xcopy /qy ..\..\OrbLaunch\obj\Starter.prc
..\..\setup\deprc Starter.prc
xcopy /qy code0000.bin ..\..\compiler\res\launcher
xcopy /qy code0001.bin ..\..\compiler\res\launcher
xcopy /qy data0000.bin ..\..\compiler\res\launcher
xcopy /qy Talt1001.bin ..\..\compiler\res\launcher
popd

if "%1"=="bits" goto :eof

rem ***********
rem Build orbit
rem ***********

echo *** [Building orbit %TARGET_NAME%]
devenv /nologo %BUILD_TYPE% %TARGET_NAME% ..\orbit\orbit.sln

rem end if BUILD
)

rem **********
rem Build Docs
rem **********
echo *** [Building docs]
pushd ..\docs
call gendocs
popd

echo *** [Copying docs]
xcopy /qy ..\docs\out\OrbForms.chm
if not exist HTMLdocs\. (
md HTMLdocs
) else (
erase /q /s HTMLdocs > NUL
)
xcopy /qy ..\docs\outhtml\* HTMLdocs


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


rem ************
rem Copy add-ins
rem ************
echo *** [Copying add-ins]
if not exist add-ins\. (
md add-ins
) else (
erase /q /s add-ins > NUL
)
xcopy /qys /exclude:..\setup\excludecvs.txt+..\setup\exclude-ts.txt ..\add-ins\* add-ins


rem *************
rem Copy utilcode
rem *************
echo *** [Copying utilcode]
if not exist utilcode\. (
md utilcode
) else (
erase /q /s utilcode > NUL
)
xcopy /qys /exclude:..\setup\excludecvs.txt+..\setup\exclude-ts.txt ..\utilcode\* utilcode


rem ***************
rem Build installer
rem ***************
echo *** [Building installer "%INSTALL_NAME%.nsi"]
xcopy /qy ..\setup\*.nsi
xcopy /qy ..\setup\%LICENSE_NAME%
xcopy /qy ..\setup\setup_icon.ico
xcopy /qy ..\setup\*.fon
xcopy /qy ..\setup\MathLib.prc
xcopy /qy ..\setup\check.bmp
xcopy /qy ..\setup\uncheck.bmp
xcopy /qy ..\orbpdb\orbpdb.txt
xcopy /qy ..\setup\Palm256Palette.pal
xcopy /qy ..\setup\mfc80.dll
xcopy /qy ..\setup\msvcr80.dll
xcopy /qy ..\setup\msvcp80.dll
xcopy /qy ..\setup\*.manifest
xcopy /qy ..\setup\dbghelp.dll
xcopy /qy ..\setup\zlib.dll
xcopy /qy ..\setup\crashrpt.dll
xcopy /qy ..\setup\T3_DIA_Compatibility_prcs.zip

rem only increment the build number when building retail
if "%1"=="demo" (
..\..\tools\build\stampver -v"..\setup\version.inf" orbit.exe
..\..\tools\nsis\Makensis /Oorbforms-demo.installer.log orbforms-demo
..\..\tools\nsis\Makensis /Oorbforms-demoPG.installer.log orbforms-demoPG
) else (
..\..\tools\build\stampver -v"..\setup\version.inf" -i4 -j4 orbit.exe
..\..\tools\nsis\Makensis /Oorbforms.installer.log orbforms.nsi
)

rem ***************
rem Bring BVT Scripts
rem ***************
echo Copy *** [IDE BVT Components]
xcopy /qy ..\orbit\uitest\bvt.bat
xcopy /qy ..\orbit\uitest\bvt.js
xcopy /qy ..\orbit\uitest\tinputhlp.dll
xcopy /qy ..\orbit\uitest\tnotifyhlp.dll

popd
endlocal
