@echo off
rem Builds installer\enlangg-setup.exe: stages the toolchain, packs it, links it into the setup exe.
rem Needs gcc + windres on PATH (MinGW). Run from anywhere.
setlocal
pushd "%~dp0.."
set STAGE=installer\stage
set OUT=installer\enlangg-setup.exe

if exist %STAGE% rmdir /s /q %STAGE%
mkdir %STAGE%\bin %STAGE%\lib %STAGE%\include || goto :fail

echo [1/4] Building the 7 toolchain binaries...
gcc -O2 -s -I. -Isrc -Ienlngdb/c -Ienlng/c enlangg.c enlngdb/c/enlngdb.c enlngdb/c/enlngdb_parser.c enlng/c/enlng_emitter.c -lm -o %STAGE%\bin\enlangg.exe || goto :fail
gcc -O2 -s -I. -Isrc enlng.c -lm -o %STAGE%\bin\enlng.exe || goto :fail
gcc -O2 -s -I. -Isrc -Ienlngdb/c enlngdb/c/main.c enlngdb/c/enlngdb.c enlngdb/c/enlngdb_parser.c -lm -o %STAGE%\bin\enlngdb.exe || goto :fail
gcc -O2 -s -I. -Isrc -Ienlngf/c enlngf.c -lm -lws2_32 -lgdi32 -luser32 -lshell32 -lcomdlg32 -ladvapi32 -o %STAGE%\bin\enlngf.exe || goto :fail
gcc -O2 -s -I. -Isrc -Ienlngd/c enlngd.c -lm -lws2_32 -lgdi32 -luser32 -lshell32 -lcomdlg32 -ladvapi32 -o %STAGE%\bin\enlngd.exe || goto :fail
gcc -O2 -s -I. -Isrc -Ienlngs/c enlngs.c -lm -lws2_32 -lgdi32 -luser32 -lshell32 -lcomdlg32 -ladvapi32 -o %STAGE%\bin\enlngs.exe || goto :fail
gcc -O2 -s -I. -Isrc -Ienlngm/c enlngm.c -lm -lws2_32 -lgdi32 -luser32 -lshell32 -lcomdlg32 -ladvapi32 -o %STAGE%\bin\enlngm.exe || goto :fail

echo [2/4] Staging standard library and headers...
for %%f in (lib_math lib_strings lib_ds lib_file lib_fs lib_ml lib_net lib_db lib_concurrency lib_std lib_enlngdb) do copy /y %%f.enlng %STAGE%\lib\ >nul || goto :fail
copy /y src\enlng_runtime.h %STAGE%\include\ >nul || goto :fail
copy /y enlng\c\enlng_mem.h %STAGE%\include\ >nul || goto :fail
copy /y LICENSE %STAGE%\ >nul || goto :fail
copy /y README.md %STAGE%\ >nul || goto :fail

echo [3/4] Packing payload...
gcc -O2 installer\pack.c -o installer\pack.exe || goto :fail
installer\pack.exe installer\payload.bin %STAGE% bin/enlangg.exe bin/enlng.exe bin/enlngdb.exe bin/enlngf.exe bin/enlngd.exe bin/enlngs.exe bin/enlngm.exe ^
  lib/lib_math.enlng lib/lib_strings.enlng lib/lib_ds.enlng lib/lib_file.enlng lib/lib_fs.enlng lib/lib_ml.enlng lib/lib_net.enlng lib/lib_db.enlng lib/lib_concurrency.enlng lib/lib_std.enlng lib/lib_enlngdb.enlng ^
  include/enlng_runtime.h include/enlng_mem.h LICENSE README.md || goto :fail

echo [4/4] Linking setup exe...
pushd installer
windres setup.rc -O coff -o setup_res.o || (popd & goto :fail)
gcc -O2 -s -mwindows -I.. setup.c setup_res.o -o enlangg-setup.exe -lwininet -ladvapi32 -lshell32 -lshlwapi -lole32 -lcomctl32 -luser32 -lgdi32 || (popd & goto :fail)
popd

echo [OK] %OUT%
popd
exit /b 0
:fail
echo [FAILED]
popd
exit /b 1
