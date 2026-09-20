@echo off
setlocal
echo =========================================================
echo   ENLANG SOVEREIGN ALL-IN-ONE COMPILER ^& WASM BUILDER
echo =========================================================

echo [1/4] Compiling native C binaries (enlng.exe ^& enlangg.exe)...
gcc -O2 -s enlng.c -o enlng.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] enlng.exe compilation failed!
    exit /b %ERRORLEVEL%
)

gcc -O2 -s enlangg.c enlngdb/c/enlngdb.c enlngdb/c/enlngdb_parser.c enlng/c/enlng_emitter.c -o enlangg.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] enlangg.exe compilation failed!
    exit /b %ERRORLEVEL%
)

echo [2/4] Synchronizing with WebAssembly Engine...
python sync_wasm.py
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] WebAssembly synchronization failed!
    exit /b %ERRORLEVEL%
)

echo [3/4] Rebuilding Windows Suite Installers...
python build_setup.py

echo =========================================================
echo  [SUCCESS] All Native binaries ^& WebAssembly are 100%% IN SYNC!
echo =========================================================
exit /b 0
