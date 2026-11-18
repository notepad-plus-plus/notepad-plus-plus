# Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
param([string]$Go='go')
$ErrorActionPreference='Stop'
Push-Location $PSScriptRoot
try {
    $version = [regex]::Match((Get-Content -LiteralPath Version.h -Raw), '#define NOTEX_UPDATE_VERSION "([^" ]+)"').Groups[1].Value
    if (!$version) { throw 'Missing updater version.' }
    & $Go run github.com/tc-hib/go-winres@v0.3.3 make --in windows.json --arch amd64 --out version --file-version "$version.0" --product-version "$version.0"
    if ($LASTEXITCODE -ne 0) { throw 'Updater resource generation failed.' }
    & $Go build -trimpath '-ldflags=-s -w' -o ../dist/NoteXUpdater.exe .
    if ($LASTEXITCODE -ne 0) { throw 'Updater build failed.' }
    $info=(Get-Item -LiteralPath ../dist/NoteXUpdater.exe).VersionInfo
    if ($info.ProductName -ne 'NoteX' -or $info.FileDescription -ne 'NoteX Updater' -or $info.OriginalFilename -ne 'NoteXUpdater.exe' -or $info.FileVersion -ne "$version.0") {
        throw 'Updater identity verification failed.'
    }
} finally { Pop-Location }
