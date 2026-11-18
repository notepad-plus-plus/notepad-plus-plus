# Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
# Sign reviewed application binaries before creating package inventories or update signatures.
param(
    [Parameter(Mandatory=$true)][string]$CertificateThumbprint,
    [Parameter(Mandatory=$true)][string]$SignTool,
    [Parameter(Mandatory=$true)][string[]]$Binary,
    [string]$TimestampUrl = 'http://timestamp.digicert.com'
)
$ErrorActionPreference = 'Stop'
$certificate = Get-Item -LiteralPath "Cert:\CurrentUser\My\$CertificateThumbprint"
if (!$certificate.HasPrivateKey -or $certificate.NotAfter -le (Get-Date)) { throw 'A valid code signing identity with a private key is required.' }
if ($certificate.EnhancedKeyUsageList.ObjectId.Value -notcontains '1.3.6.1.5.5.7.3.3') { throw 'Certificate does not permit code signing.' }
foreach ($file in $Binary) {
    $resolved = (Resolve-Path -LiteralPath $file).Path
    & $SignTool sign /sha1 $CertificateThumbprint /fd SHA256 /tr $TimestampUrl /td SHA256 $resolved
    if ($LASTEXITCODE -ne 0) { throw 'Authenticode signing failed.' }
    & $SignTool verify /pa /all $resolved
    if ($LASTEXITCODE -ne 0) { throw 'Authenticode trust verification failed.' }
    $signature = Get-AuthenticodeSignature -LiteralPath $resolved
    if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Thumbprint -ne $CertificateThumbprint -or !$signature.TimeStamperCertificate) {
        throw 'Signature identity, trust, or timestamp verification failed.'
    }
}
