!include "FileFunc.nsh"
!include "logiclib.nsh"

OutFile "GetVersion.exe"
SetCompressor /SOLID lzma 		; Compression technique
RequestExecutionLevel user 		; This is to make output exe run as invoker (Admin privileges not required)
!define EXEPATH "..\bin\Notepad++.exe"	; Absolute path


Section "Main"
	SetAutoClose true	; it simply writes version info in the text. So close immediatly after writing version info

    DetailPrint "Getting version from ${EXEPATH}"
    GetDllVersion "${EXEPATH}" $R0 $R1 	; Two values were read during compilation
	
	;${GetFileVersion} "${EXEPATH}" $R0
	;DetailPrint "Another way to get version: $R0"

    IntOp $R2 $R0 / 0x00010000		; $R2 now contains major version
    IntOp $R3 $R0 & 0x0000FFFF		; $R3 now contains minor version
    IntOp $R4 $R1 / 0x00010000		; $R4 now contains release
    IntOp $R5 $R1 & 0x0000FFFF		; $R5 now contains build
	
	; if notepad++.exe version = 7.0.0.0
	; 	$1 = 7.0.0.0 (File Version)
	;	$2 = 7		 (Product Version)
	;	$3 = 0		 (Minor Version)
	
	; if notepad++.exe version = 7.1.2.0
	; 	$1 = 7.0.0.0 (File Version)
	;	$2 = 7.1.2 	 (Product Version)
	;	$3 = 12		 (Minor Version)
	
    StrCpy $1 "$R2.$R3.$R4.$R5"
	DetailPrint "Version read: $0"
	
	${If} $R5 == "0"
		${If} $R4 == "0"
			StrCpy $3 "$R3"			; $2 = $R3 will be either 0 or some value 
			${If} $R3 == "0"
				StrCpy $2 "$R2"
			${Else}
				StrCpy $2 "$R2.$R3"
			${Endif}
		${Else}
			StrCpy $2 "$R2.$R3.$R4"
			StrCpy $3 "$R3$R4"
		${Endif}
	${Else}
		StrCpy $2 "$R2.$R3.$R4.$R5"
		StrCpy $3 "$R3$R4$R5"
	${Endif}
	
	FileOpen $0 VersionInfo.txt w 	;Opens a Empty File and fills it with requied macro
		
		FileWrite $0 "!define VERSION1 $R2$\r$\n"
		FileWrite $0 "!define VERSION2 $R3$\r$\n"
		FileWrite $0 "!define VERSION3 $R4$\r$\n"
		FileWrite $0 "!define VERSION4 $R5$\r$\n"
		
		FileWrite $0 "!define VERSION $1$\r$\n"
		FileWrite $0 "!define APPVERSION $2$\r$\n"
		FileWrite $0 "!define PRODVERSION $R2.$3$\r$\n"
		
		FileWrite $0 "!define VERSION_MAJOR $R2$\r$\n"
		FileWrite $0 "!define VERSION_MINOR $3$\r$\n"
		
	FileClose $0 					;Closes the filled file

SectionEnd