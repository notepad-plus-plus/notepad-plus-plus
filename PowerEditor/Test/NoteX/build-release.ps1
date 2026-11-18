# Copyright (C) 2026 Berk Kivircik.
# SPDX-License-Identifier: GPL-3.0-only
# Created for NoteX; publication date: 2026-10-28.

param(
    [string]$BuildTools = '',
    [string]$LogDirectory = "$PSScriptRoot\out"
)
$ErrorActionPreference = 'Stop'

if (-not $BuildTools) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (!(Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio C++ Build Tools or pass -BuildTools.' }
    $BuildTools = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $BuildTools) { throw 'Visual Studio C++ tools were not found. Pass -BuildTools.' }
}

$repo = (Resolve-Path "$PSScriptRoot\..\..\..").Path
New-Item -ItemType Directory -Force -Path $LogDirectory | Out-Null
$LogDirectory = (Resolve-Path $LogDirectory).Path
$msbuild = Join-Path $BuildTools 'MSBuild\Current\Bin\amd64\MSBuild.exe'
$solution = Join-Path $repo 'PowerEditor\visual.net\notepadPlus.sln'
# Clean all projects before compiling any of them: solution Rebuild can remove
# dependency PDBs from the shared output directory after they have been built.
& $msbuild $solution /t:Clean /m:2 /nr:false /p:Configuration=Release /p:Platform=x64 /verbosity:quiet
if ($LASTEXITCODE -ne 0) { throw 'Release clean failed.' }
& $msbuild $solution /t:Build /m:2 /nr:false /p:Configuration=Release /p:Platform=x64 /p:CL_MPCount=2 /verbosity:quiet /fl "/flp:LogFile=$LogDirectory\release-build.log;Verbosity=minimal"
if ($LASTEXITCODE -ne 0) { throw 'Release build failed.' }
& "$PSScriptRoot\run.ps1" -Configuration Release -BuildTools $BuildTools -Output (Join-Path $LogDirectory 'native-tests')
