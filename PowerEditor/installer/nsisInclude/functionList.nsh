; this file is part of installer for Notepad++
; Copyright (C)2016 Don HO <don.h@free.fr>
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either
; version 2 of the License, or (at your option) any later version.
;
; Note that the GPL places important restrictions on "derived works", yet
; it does not provide a detailed definition of that term.  To avoid      
; misunderstandings, we consider an application to constitute a          
; "derivative work" for the purpose of this license if it does any of the
; following:                                                             
; 1. Integrates source code from Notepad++.
; 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
;    installer, such as those produced by InstallShield.
; 3. Links to a library or executes a program that does any of the above.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


SectionGroup "Function List Files" functionListComponent
	SetOverwrite off
	
	; UPDATE_PATH: the value is $UPDATE_PATH if doLocalConf.xml exit,
	;              otherwise the value is $APPDATA\${APPNAME}
	${MementoSection} "C" C_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\c.xml"
	${MementoSectionEnd}
	
	${MementoSection} "C++" C++_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\cpp.xml"
	${MementoSectionEnd}

	${MementoSection} "Java" Java_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\java.xml"
	${MementoSectionEnd}

	${MementoSection} "C#" C#_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\cs.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Assembly" Assembly_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\asm.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Bash" Bash_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\bash.xml"
	${MementoSectionEnd}
	
	${MementoSection} "SQL" SQL_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\sql.xml"
	${MementoSectionEnd}
	
	${MementoSection} "PHP" PHP_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\php.xml"
	${MementoSectionEnd}

	${MementoSection} "COBOL section free" COBOL-section-free
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\cobol-free.xml"
	${MementoSectionEnd}

	${MementoSection} "COBOL" COBOL_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\cobol.xml"
	${MementoSectionEnd}

	${MementoSection} "Perl" Perl_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\perl.xml"
	${MementoSectionEnd}
	
	${MementoSection} "JavaScript" JavaScript_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\javascript.js.xml"
	${MementoSectionEnd}

	${MementoSection} "Python" Python_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\python.xml"
	${MementoSectionEnd}
	
	${MementoSection} "ini" ini_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\ini.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Inno Setup" Innosetup_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\inno.xml"
	${MementoSectionEnd}
	
	${MementoSection} "VHDL" VHDL_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\vhdl.xml"
	${MementoSectionEnd}
	
	${MementoSection} "KRL" KRL_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\krl.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Override Map" OverrideMap_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\overrideMap.xml"
	${MementoSectionEnd}
	
	${MementoSection} "NSIS" NSIS_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\nsis.xml"
	${MementoSectionEnd}
	
	${MementoSection} "PowerShell" PowerShell_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\powershell.xml"
	${MementoSectionEnd}

	${MementoSection} "BATCH" BATCH_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\batch.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Ruby" Ruby_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\ruby.xml"
	${MementoSectionEnd}

	${MementoSection} "BaanC" BaanC_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\baanc.xml"
	${MementoSectionEnd}

	${MementoSection} "Sinumerik" Sinumerik_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\sinumerik.xml"
	${MementoSectionEnd}

	${MementoSection} "AutoIt" AutoIt_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\autoit.xml"
	${MementoSectionEnd}

	${MementoSection} "UniVerse BASIC" UniVerseBASIC_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\universe_basic.xml"
	${MementoSectionEnd}

	${MementoSection} "XML" XML_FL
		SetOutPath "$UPDATE_PATH\functionList"
		File ".\functionList\xml.xml"
	${MementoSectionEnd}
SectionGroupEnd



SectionGroup un.functionListComponent
	Section un.PHP_FL
		Delete "$UPDATE_PATH\functionList\php.xml"
	SectionEnd
	
	Section un.Assembly_FL
		Delete "$UPDATE_PATH\functionList\asm.xml"
	SectionEnd
	
	Section un.SQL_FL
		Delete "$UPDATE_PATH\functionList\sql.xml"
	SectionEnd
	
	Section un.Bash_FL
		Delete "$UPDATE_PATH\functionList\bash.xml"
	SectionEnd

	Section un.COBOL-section-free_FL
		Delete "$UPDATE_PATH\functionList\cobol-free.xml"
	SectionEnd

	Section un.Perl_FL
		Delete "$UPDATE_PATH\functionList\perl.xml"
	SectionEnd

	Section un.C_FL
		Delete "$UPDATE_PATH\functionList\c.xml"
	SectionEnd
	
	Section un.C++_FL
		Delete "$UPDATE_PATH\functionList\cpp.xml"
	SectionEnd
	
	Section un.Java_FL
		Delete "$UPDATE_PATH\functionList\java.xml"
	SectionEnd
	
	Section un.C#_FL
		Delete "$UPDATE_PATH\functionList\cs.xml"
	SectionEnd
	
	Section un.JavaScript_FL
		Delete "$UPDATE_PATH\functionList\javascript.js.xml"
	SectionEnd

	Section un.Python_FL
		Delete "$UPDATE_PATH\functionList\python.xml"
	SectionEnd

	Section un.COBOL_FL
		Delete "$UPDATE_PATH\functionList\cobol.xml"
	SectionEnd
	
	Section un.ini_FL
		Delete "$UPDATE_PATH\functionList\ini.xml"
	SectionEnd
	
	Section un.VHDL_FL
		Delete "$UPDATE_PATH\functionList\vhdl.xml"
	SectionEnd	
	
	Section un.Innosetup_FL
		Delete "$UPDATE_PATH\functionList\inno.xml"
	SectionEnd
	
	Section un.XML_FL
		Delete "$UPDATE_PATH\functionList\xml.xml"
	SectionEnd
	
	Section un.NSIS_FL
		Delete "$UPDATE_PATH\functionList\nsis.xml"
	SectionEnd
	
	Section un.KRL_FL
		Delete "$UPDATE_PATH\functionList\krl.xml"
	SectionEnd

	Section un.BATCH_FL
		Delete "$UPDATE_PATH\functionList\batch.xml"
	SectionEnd
	
	Section un.OverrideMap_FL
		Delete "$UPDATE_PATH\functionList\overrideMap.xml"
	SectionEnd

	Section un.BaanC_FL
		Delete "$UPDATE_PATH\functionList\baanc.xml"
	SectionEnd

	Section un.PowerShell_FL
		Delete "$UPDATE_PATH\functionList\powershell.xml"
	SectionEnd

	Section un.AutoIt_FL
		Delete "$UPDATE_PATH\functionList\autoit.xml"
	SectionEnd

	Section un.Ruby_FL
		Delete "$UPDATE_PATH\functionList\ruby.xml"
	SectionEnd

	Section un.UniVerseBASIC_FL
		Delete "$UPDATE_PATH\functionList\universe_basic.xml"
	SectionEnd

	Section un.Sinumerik_FL
		Delete "$UPDATE_PATH\functionList\sinumerik.xml"
	SectionEnd

SectionGroupEnd
