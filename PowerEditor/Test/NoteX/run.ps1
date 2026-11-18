# Copyright (C) 2026 Berk Kivircik.
# SPDX-License-Identifier: GPL-3.0-only
# Created for NoteX; publication date: 2026-10-28.

param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Release',
    [string]$BuildTools = '',
    [string]$Output = "$PSScriptRoot\out"
)
$ErrorActionPreference = 'Stop'

if (-not $BuildTools) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio C++ Build Tools or pass -BuildTools.' }
    $BuildTools = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $BuildTools) { throw 'Visual Studio C++ tools were not found. Pass -BuildTools.' }
}

$repo = (Resolve-Path "$PSScriptRoot\..\..\..").Path
$library = "$repo\PowerEditor\visual.net\x64\$Configuration\libScintilla.lib"
if (!(Test-Path -LiteralPath $library)) { throw "Build the x64 $Configuration solution first." }
New-Item -ItemType Directory -Force -Path $Output | Out-Null
$Output = (Resolve-Path $Output).Path
$runtime = if ($Configuration -eq 'Release') { '/MT' } else { '/MTd' }
$command = @"
@echo off
call "$BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc $runtime /DUNICODE /D_UNICODE /I"$repo\scintilla\include" /I"$repo\PowerEditor\src" "$PSScriptRoot\formatting_tests.cpp" "$repo\PowerEditor\src\MISC\sha2\sha-256.cpp" /Fe:notex-tests.exe /link /LTCG "$library" user32.lib gdi32.lib imm32.lib ole32.lib oleaut32.lib advapi32.lib uuid.lib msimg32.lib
if errorlevel 1 exit /b 1
notex-tests.exe
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /W4 $runtime /DUNICODE /D_UNICODE /I"$repo\scintilla\include" /I"$repo\PowerEditor\src" "$PSScriptRoot\menu_tests.cpp" /Fe:menu-tests.exe /link user32.lib gdi32.lib comctl32.lib
if errorlevel 1 exit /b 1
menu-tests.exe
exit /b %errorlevel%
"@
$script = Join-Path $Output 'build-and-test.cmd'
Set-Content -LiteralPath $script -Value $command -Encoding ascii
Push-Location $Output
try {
    & cmd.exe /c $script | Tee-Object -FilePath (Join-Path $Output 'results.log')
    if ($LASTEXITCODE -ne 0) { throw "NoteX native integration tests failed ($LASTEXITCODE)." }
} finally { Pop-Location }
