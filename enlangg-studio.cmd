@echo off
setlocal enabledelayedexpansion

title Enlangg Studio - Pure Sovereign VS Code Edition

:: 1. Add Sovereign Compilers to PATH
if exist "%USERPROFILE%\.enlangg\bin" (
    set "PATH=%USERPROFILE%\.enlangg\bin;%PATH%"
)
if exist "%~dp0" (
    set "PATH=%~dp0;%PATH%"
)

:: 2. Configure Dedicated Enlangg Studio Profile & Extensions
set "STUDIO_DATA=%USERPROFILE%\.enlangg\studio\data"
set "STUDIO_EXTS=%USERPROFILE%\.enlangg\studio\extensions"

if not exist "%STUDIO_DATA%\User" mkdir "%STUDIO_DATA%\User"
if not exist "%STUDIO_EXTS%" mkdir "%STUDIO_EXTS%"

:: Ensure default settings.json exists with Dark Modern theme and compiler associations
if not exist "%STUDIO_DATA%\User\settings.json" (
    (
        echo {
        echo   "workbench.colorTheme": "Default Dark Modern",
        echo   "editor.fontFamily": "Consolas, 'Cascadia Code', 'Courier New', monospace",
        echo   "editor.fontSize": 14,
        echo   "editor.lineHeight": 22,
        echo   "editor.minimap.enabled": true,
        echo   "editor.renderWhitespace": "selection",
        echo   "editor.cursorBlinking": "smooth",
        echo   "editor.cursorSmoothCaretAnimation": "on",
        echo   "editor.smoothScrolling": true,
        echo   "terminal.integrated.defaultProfile.windows": "PowerShell",
        echo   "terminal.integrated.env.windows": {
        echo     "PATH": "${env:USERPROFILE}\\.enlangg\\bin;${env:PATH}"
        echo   },
        echo   "files.associations": {
        echo     "*.enlng": "enlang",
        echo     "*.enlg": "enlang",
        echo     "*.enlngdb": "enlgdb",
        echo     "*.enlgdb": "enlgdb",
        echo     "*.enlngf": "enlangf",
        echo     "*.enlgf": "enlangf",
        echo     "*.enlngd": "enlangd",
        echo     "*.enlgd": "enlangd",
        echo     "*.enlngs": "enlgs",
        echo     "*.enlgs": "enlgs",
        echo     "*.enlngm": "enlgm",
        echo     "*.enlgm": "enlgm"
        echo   }
        echo }
    ) > "%STUDIO_DATA%\User\settings.json"
)

:: Copy official Enlang extension if not present or updated
if exist "%~dp0vscode-enlang" (
    if not exist "%STUDIO_EXTS%\enlang1234.enlang-official-1.2.0" (
        mkdir "%STUDIO_EXTS%\enlang1234.enlang-official-1.2.0" 2>nul
    )
    copy /y "%~dp0vscode-enlang\package.json" "%STUDIO_EXTS%\enlang1234.enlang-official-1.2.0\" >nul 2>&1
    copy /y "%~dp0vscode-enlang\extension.js" "%STUDIO_EXTS%\enlang1234.enlang-official-1.2.0\" >nul 2>&1
    copy /y "%~dp0vscode-enlang\language-configuration.json" "%STUDIO_EXTS%\enlang1234.enlang-official-1.2.0\" >nul 2>&1
    if exist "%~dp0vscode-enlang\syntaxes" (
        if not exist "%STUDIO_EXTS%\enlang1234.enlang-official-1.2.0\syntaxes" mkdir "%STUDIO_EXTS%\enlang1234.enlang-official-1.2.0\syntaxes" 2>nul
        copy /y "%~dp0vscode-enlang\syntaxes\*" "%STUDIO_EXTS%\enlang1234.enlang-official-1.2.0\syntaxes\" >nul 2>&1
    )
    if exist "%~dp0vscode-enlang\snippets" (
        if not exist "%STUDIO_EXTS%\enlang1234.enlang-official-1.2.0\snippets" mkdir "%STUDIO_EXTS%\enlang1234.enlang-official-1.2.0\snippets" 2>nul
        copy /y "%~dp0vscode-enlang\snippets\*" "%STUDIO_EXTS%\enlang1234.enlang-official-1.2.0\snippets\" >nul 2>&1
    )
)

:: 3. Locate VS Code Binary (Code-OSS / VS Code)
set "CODE_BIN="

if exist "D:\Microsoft VS Code\Code.exe" (
    set "CODE_BIN=D:\Microsoft VS Code\Code.exe"
) else if exist "%LOCALAPPDATA%\Programs\Microsoft VS Code\Code.exe" (
    set "CODE_BIN=%LOCALAPPDATA%\Programs\Microsoft VS Code\Code.exe"
) else if exist "C:\Program Files\Microsoft VS Code\Code.exe" (
    set "CODE_BIN=C:\Program Files\Microsoft VS Code\Code.exe"
) else (
    for %%X in (code.cmd code.exe) do (
        if not defined CODE_BIN (
            set "CODE_BIN=%%~$PATH:X"
        )
    )
)

:: 4. Launch Pure VS Code Engine or fallback to Desktop App
set "WORKSPACE_DIR=%~dp0"
if "%~1" neq "" set "WORKSPACE_DIR=%~1"

if defined CODE_BIN (
    echo [Enlangg Studio] Launching Pure VS Code Engine...
    start "" "!CODE_BIN!" --user-data-dir "%STUDIO_DATA%" --extensions-dir "%STUDIO_EXTS%" "%WORKSPACE_DIR%"
) else (
    echo [Enlangg Studio] VS Code not detected, launching bundled Desktop App...
    cd /d "%~dp0desktop"
    call npx electron .
)
