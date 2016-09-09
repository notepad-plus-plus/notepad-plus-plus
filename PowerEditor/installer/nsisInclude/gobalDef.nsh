
; Define the application name
!define APPNAME "Notepad++"

!define APPVERSION "7"
!define APPNAMEANDVERSION "${APPNAME} v${APPVERSION}"
!define VERSION_MAJOR 7
!define VERSION_MINOR 0

!define APPWEBSITE "http://notepad-plus-plus.org/"

!define UNINSTALL_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
!define MEMENTO_REGISTRY_ROOT HKLM
!define MEMENTO_REGISTRY_KEY ${UNINSTALL_REG_KEY}

; Main Install settings
Name "${APPNAMEANDVERSION}"
InstallDir "$PROGRAMFILES\${APPNAME}"
InstallDirRegKey HKLM "Software\${APPNAME}" ""
