# Builds the in-browser engines used by the website playground and Enlangg Studio:
#   website/enlng_engine.{js,wasm}     the Enlng interpreter (same sources as enlng.exe)
#   website/enlngdb_engine.{js,wasm}   the EnlngDB engine (same sources as enlngdb.exe)
# Needs the Emscripten SDK. One-time setup:
#   git clone https://github.com/emscripten-core/emsdk.git D:\emsdk
#   D:\emsdk\emsdk install 3.1.74; D:\emsdk\emsdk activate 3.1.74
# Usage: powershell -File tools\build_wasm.ps1 [-Emsdk D:\emsdk]
param([string]$Emsdk = "D:\emsdk")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$node = Get-ChildItem "$Emsdk\node" -Directory | Select-Object -First 1
$py = Get-ChildItem "$Emsdk\python" -Directory | Select-Object -First 1
$env:EMSDK = $Emsdk
$env:PATH = "$Emsdk;$Emsdk\upstream\emscripten;$($node.FullName)\bin;$($py.FullName);" + $env:PATH
$common = @("-O2", "-sMODULARIZE=1", "-sEXPORTED_RUNTIME_METHODS=callMain,FS", "-sINVOKE_RUN=0",
            "-sALLOW_MEMORY_GROWTH=1", "-sENVIRONMENT=web,worker,node", "-sFORCE_FILESYSTEM=1")
Push-Location $root
try {
    & emcc.bat -I. -Isrc enlng.c -o website\enlng_engine.js "-sEXPORT_NAME=EnlngEngine" "-sSTACK_SIZE=33554432" @common
    & emcc.bat -I. -Isrc -Ienlngdb/c enlngdb/c/main.c enlngdb/c/enlngdb.c enlngdb/c/enlngdb_parser.c `
        -o website\enlngdb_engine.js "-sEXPORT_NAME=EnlngdbEngine" "-sSTACK_SIZE=5242880" @common
    # the standard library files the playground offers to `use`
    $lib = [ordered]@{}
    foreach ($n in "lib_math","lib_strings","lib_ds","lib_file","lib_fs","lib_ml","lib_net","lib_db","lib_concurrency","lib_std","lib_enlngdb") {
        $lib["$n.enlng"] = [IO.File]::ReadAllText("$root\$n.enlng")
    }
    [IO.File]::WriteAllText("$root\website\enlng_stdlib.json", ($lib | ConvertTo-Json -Compress), (New-Object Text.UTF8Encoding $false))
} finally { Pop-Location }
Write-Host "Built website\enlng_engine.wasm, website\enlngdb_engine.wasm and website\enlng_stdlib.json"
