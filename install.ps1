# =====================================================================
#   Enlangg Sovereign Toolchain - Universal Windows PowerShell Installer
#   Usage:
#     powershell -ExecutionPolicy ByPass -c "irm https://raw.githubusercontent.com/Aero99op/enlang-main/main/install.ps1 | iex"
# =====================================================================

$ErrorActionPreference = "Stop"

$Cyan = "`e[36m"
$Green = "`e[32m"
$Yellow = "`e[33m"
$Bold = "`e[1m"
$Reset = "`e[0m"

Write-Host @"
=====================================================================
    ENLANGG & ENLNG - Sovereign Programming Language Toolchain
=====================================================================
"@

$InstallDir = Join-Path $HOME ".enlangg\bin"
if (!(Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
}

$EnlanggExe = Join-Path $InstallDir "enlangg.exe"
$EnlngExe = Join-Path $InstallDir "enlng.exe"

$RepoUrl = "https://raw.githubusercontent.com/Aero99op/enlang-main/main"
$ReleaseFallbackUrl = "https://github.com/Aero99op/enlang-main/releases/latest/download"

# 1. Download / Install Binaries
Write-Host "$Yellow>> Installing enlangg & enlng binaries to: $InstallDir ...$Reset"

# Check if installing from local repo workspace first
$LocalEnlangg = Join-Path $PSScriptRoot "enlangg.exe"
$LocalEnlng = Join-Path $PSScriptRoot "enlng.exe"

if ((Test-Path $LocalEnlangg) -and (Test-Path $LocalEnlng)) {
    Write-Host "   Using local build binaries..."
    Copy-Item -Force $LocalEnlangg $EnlanggExe
    Copy-Item -Force $LocalEnlng $EnlngExe
} else {
    Write-Host "   Fetching latest production binaries from GitHub..."
    try {
        # Try raw repo / release
        Invoke-WebRequest -Uri "$RepoUrl/enlangg.exe" -OutFile $EnlanggExe -UseBasicParsing
        Invoke-WebRequest -Uri "$RepoUrl/enlng.exe" -OutFile $EnlngExe -UseBasicParsing
    } catch {
        Write-Host "   Downloading from GitHub Releases..."
        Invoke-WebRequest -Uri "$ReleaseFallbackUrl/enlangg.exe" -OutFile $EnlanggExe -UseBasicParsing
        Invoke-WebRequest -Uri "$ReleaseFallbackUrl/enlng.exe" -OutFile $EnlngExe -UseBasicParsing
    }
}

# 2. Update Environment PATH
Write-Host "$Yellow>> Configuring system PATH environment variable...$Reset"
$UserPath = [Environment]::GetEnvironmentVariable("Path", [EnvironmentVariableTarget]::User)
if ($UserPath -split ";" -notcontains $InstallDir) {
    $NewPath = "$UserPath;$InstallDir"
    [Environment]::SetEnvironmentVariable("Path", $NewPath, [EnvironmentVariableTarget]::User)
    Write-Host "$Green   [OK] Added $InstallDir to User PATH.$Reset"
} else {
    Write-Host "$Green   [OK] $InstallDir is already in User PATH.$Reset"
}

# Update current session PATH immediately
if ($env:Path -split ";" -notcontains $InstallDir) {
    $env:Path = "$env:Path;$InstallDir"
}

# 3. Verify Installation
Write-Host "$Green>> Verification:$Reset"
try {
    & "$EnlanggExe" --version
    & "$EnlngExe" --version
} catch {
    # If direct invocation in PS has strict policy
}

Write-Host @"
$Green$Bold
=====================================================================
  [SUCCESS] Enlangg & Enlng are now globally installed! 🚀
=====================================================================
$Reset
$BoldYou can now open ANY terminal (PowerShell / CMD) and run:$Reset
  $Cyan enlangg run <file.enlng>   $Reset (Execute natural backend code)
  $Cyan enlng run <file.enlng>     $Reset (Pure sovereign general-purpose engine)
  $Cyan enlangg --help             $Reset (Show full toolchain manual)

Documentation & Online Playground: https://enlangg.vercel.app
"@
