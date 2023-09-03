@ECHO OFF

ECHO Start signing file: %1

".\signCert\SignTool.exe" sign /f ".\signCert\npp_sign_cert.pfx" "%1"
if errorlevel 1 goto SigningFailed
goto SigningOK

:SigningFailed
echo Failed to sign file %1
exit 1

:SigningOK
