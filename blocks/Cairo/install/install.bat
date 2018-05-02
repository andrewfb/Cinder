REM Run this from Visual Studio Command Prompt. Requires msys2 to be in the PATH
@echo off
setlocal enableextensions
set me=%~n0
set parent=%~dp0

rmdir .\tmp /q /s
mkdir .\tmp

if not exist C:\msys64 ( 
	echo %me%: Didn't find msys64. You must download msys64 to continue. Exiting!
	exit
)

if not defined Platform (
	set cairo_final_lib_path="%cd%\..\lib\msw\x86"
) else (
	set cairo_final_lib_path="%cd%\..\lib\msw\x64\"
)

set PATH=%PATH%;C:\msys64\usr\bin
echo %me%: set msys64 into path.

if not exist C:\msys64\usr\bin\curl.exe pacman -S curl

if not exist C:\msys64\usr\bin\tar.exe pacman -S tar

if not exist C:\msys64\usr\bin\make.exe pacman -S make

rmdir %cairo_final_lib_path% /q /s
mkdir %cairo_final_lib_path%
echo Cairo final path: %cairo_final_lib_path%
set cairo_final_include_path="%cd%\..\include\msw\cairo"
rmdir %cairo_final_include_path% /q /s
mkdir %cairo_final_include_path%
set pixman_final_include_path="%cd%\..\include\msw\pixman"
rmdir %pixman_final_include_path% /q /s
mkdir %pixman_final_include_path%
set libpng_final_include_path="%cd%\..\include\msw\libpng"
rmdir %libpng_final_include_path% /q /s
mkdir %libpng_final_include_path%
cd tmp

echo %me%: Downloading libs...

curl http://zlib.net/zlib-1.2.11.tar.gz -o zlib.tar.gz
tar -xf zlib.tar.gz
mv zlib-* zlib
rm zlib.tar.gz

curl ftp://ftp.simplesystems.org/pub/libpng/png/src/libpng16/libpng-1.6.34.tar.gz -o libpng.tar.gz
tar -xf libpng.tar.gz
mv libpng-* libpng 
rm libpng.tar.gz

curl https://www.cairographics.org/releases/pixman-0.34.0.tar.gz -o pixman.tar.gz
tar -xf pixman.tar.gz
mv pixman-* pixman 
rm pixman.tar.gz

curl https://cairographics.org/snapshots/cairo-1.15.12.tar.xz -o cairo.tar.xz
tar -xf cairo.tar.xz
mv cairo-* cairo 
rm cairo.tar.xz

echo %me%: Finished downloading. Preparing to build...

set ROOTDIR=%cd%
echo "%me%: setting working directory. %ROOTDIR%"

cd "%ROOTDIR%\zlib\contrib\vstudio\vc14"

echo Building zlib...
sed "/RuntimeLibrary=/s/2/0/;s=x64\\\ZlibStat$(Configuration)=ZlibStat=;s=x86\\\ZlibStat$(Configuration)=ZlibStat=;s=ZLIB_WINAPI;==" zlibstat.vcxproj > fixed.vcxproj 
move /Y fixed.vcxproj zlibstat.vcxproj
msbuild.exe zlibstat.vcxproj /p:PlatformToolset=v140 /p:Configuration="ReleaseWithoutAsm" /t:Build
move /Y ZlibStat\zlibstat.lib ..\..\..\zlibstat.lib

echo Building Pixman...
cd %ROOTDIR%\pixman
sed s/-MD/-MT/ Makefile.win32.common > Makefile.fixed
move /Y Makefile.fixed Makefile.win32.common
cd pixman
make -f Makefile.win32 "CFG=release" "MMX=off"

set INCLUDE=%INCLUDE%;"%ROOTDIR%\zlib"
set INCLUDE=%INCLUDE%;"%ROOTDIR%\libpng"
set INCLUDE=%INCLUDE%;"%ROOTDIR%\pixman\pixman"
set INCLUDE=%INCLUDE%;"%ROOTDIR%\cairo\boilerplate"
set INCLUDE=%INCLUDE%;"%ROOTDIR%\cairo\src"

set LIB=%LIB%;"%ROOTDIR%\zlib\contrib\vstudio\vc14\ZlibStat"
if not defined Platform (
	set LIBPNGOUTPUT="x64"
) else (
	set LIBPNGOUTPUT="x86"
)
set LIB=%LIB%;"%ROOTDIR%\libpng\projects\vstudio\%LIBPNGOUTPUT%\Release Library\"

echo Building libpng...
cd "%ROOTDIR%\libpng\projects\vstudio"
if defined Platform (
	sed "s=$(SolutionDir)=..=;s/Win32/x64/" libpng\libpng.vcxproj > libpng\fixed.vcxproj
	move /Y libpng\fixed.vcxproj libpng\libpng.vcxproj
	sed 's/Win32/x64/g' zlib\zlib.vcxproj > zlib\fixed.vcxproj
	move /Y zlib\fixed.vcxproj zlib\zlib.vcxproj
	sed 's/Win32/x64/g' pnglibconf\pnglibconf.vcxproj > pnglibconf\fixed.vcxproj
	move /Y pnglibconf\fixed.vcxproj pnglibconf\pnglibconf.vcxproj
	sed 's/Win32/x64/g' vstudio.sln > fixed.sln
	move /Y fixed.sln vstudio.sln
) else (
	sed "s=$(SolutionDir)=..=" libpng\libpng.vcxproj > libpng\fixed.vcxproj
	move /Y libpng\fixed.vcxproj libpng\libpng.vcxproj
)
sed 's/zlib-1.2.8/zlib/' zlib.props > libpng\zlib.props
move /Y libpng\zlib.props zlib.props
if not defined Platform (
	msbuild vstudio.sln /p:PlatformToolset=v140 /p:Configuration="Release Library" /t:libpng:Rebuild
	move /Y "Release Library\libpng16.lib" "%ROOTDIR%\libpng\libpng.lib"
)
else (
	msbuild vstudio.sln /p:PlatformToolset=v140 /p:Configuration="Release Library" /p:Platform="x64" /t:libpng:Rebuild
	move /Y "x64\Release Library\libpng16.lib" "%ROOTDIR%\libpng\libpng.lib"
)

echo Building Cairo...
cd %ROOTDIR%\cairo
sed s/-MD/-MT/ build\Makefile.win32.common > build\Makefile.fixed
move /Y build\Makefile.fixed build\Makefile.win32.common
sed "s/CAIRO_HAS_FT_FONT=./CAIRO_HAS_FT_FONT=1/" build\Makefile.win32.features > build\Makefile.win32.features.fixed
move /Y build\Makefile.win32.features.fixed build\Makefile.win32.common
sed s/zdll.lib/zlibstat.lib/ build\Makefile.win32.common > build\Makefile.fixed
move /Y build\Makefile.fixed build\Makefile.win32.common

make -f Makefile.win32 "CFG=release"

echo move /Y "%ROOTDIR%\pixman\pixman\release\pixman-1.lib" %cairo_final_lib_path%
cd "%ROOTDIR%\pixman\pixman\"
for %%I in (pixman.h pixman-version.h) do copy %%I %pixman_final_include_path%
   
move /Y "%ROOTDIR%\cairo\src\release\cairo-static.lib" %cairo_final_lib_path%
cd "%ROOTDIR%\cairo\"
copy cairo-version.h %cairo_final_include_path%
cd src\
for %%I in (cairo-features.h cairo.h cairo-deprecated.h cairo-win32.h cairo-script.h cairo-ps.h cairo-pdf.h cairo-svg.h cairo-pdf.h) do copy %%I %cairo_final_include_path%
cd "%ROOTDIR%\.."
echo "rmdir tmp\ /q /s"
