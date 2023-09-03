@ECHO OFF

if [%SIGN%] == [] goto NoSignInstaller
if not %SIGN% == 1 goto NoSignInstaller

ECHO Start signing file: %1
%signBinary% "%1"

if errorlevel 1 goto SigningFailed
goto SigningOK

:SigningFailed
echo Failed to sign file %1
exit 1

:NoSignInstaller
ECHO Signing skipped for file: %1

:SigningOK
