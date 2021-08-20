; This file is part of Notepad++ project
; Copyright (C)2021 Don HO <don.h@free.fr>
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; at your option any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.


SectionGroup "Auto-completion Files" autoCompletionComponent
	SetOverwrite off
	
	${MementoSection} "C" C
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\c.xml"
	${MementoSectionEnd}
	
	${MementoSection} "C++" C++
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\cpp.xml"
	${MementoSectionEnd}

	${MementoSection} "Java" Java
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\java.xml"
	${MementoSectionEnd}
	
	${MementoSection} "C#" C#
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\cs.xml"
	${MementoSectionEnd}
	
	${MementoSection} "HTML" HTML
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\html.xml"
	${MementoSectionEnd}
	
	${MementoSection} "RC" RC
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\rc.xml"
	${MementoSectionEnd}
	
	${MementoSection} "SQL" SQL
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\sql.xml"
	${MementoSectionEnd}
	
	${MementoSection} "PHP" PHP
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\php.xml"
	${MementoSectionEnd}

	${MementoSection} "CSS" CSS
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\css.xml"
	${MementoSectionEnd}

	${MementoSection} "VB" VB
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\vb.xml"
	${MementoSectionEnd}

	${MementoSection} "Perl" Perl
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\perl.xml"
	${MementoSectionEnd}
	
	${MementoSection} "JavaScript" JavaScript
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\javascript.xml"
	${MementoSectionEnd}

	${MementoSection} "Python" Python
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\python.xml"
	${MementoSectionEnd}
	
	${MementoSection} "ActionScript" ActionScript
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\actionscript.xml"
	${MementoSectionEnd}
	
	${MementoSection} "LISP" LISP
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\lisp.xml"
	${MementoSectionEnd}
	
	${MementoSection} "VHDL" VHDL
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\vhdl.xml"
	${MementoSectionEnd}
	
	${MementoSection} "TeX" TeX
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\tex.xml"
	${MementoSectionEnd}
	
	${MementoSection} "DocBook" DocBook
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\xml.xml"
	${MementoSectionEnd}
	
	${MementoSection} "NSIS" NSIS
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\nsis.xml"
	${MementoSectionEnd}
	
	${MementoSection} "CMAKE" CMAKE
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\cmake.xml"
	${MementoSectionEnd}

	${MementoSection} "BATCH" BATCH
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\batch.xml"
	${MementoSectionEnd}
	
	${MementoSection} "CoffeeScript" CoffeeScript
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\coffee.xml"
	${MementoSectionEnd}

	${MementoSection} "BaanC" BaanC
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\baanc.xml"
	${MementoSectionEnd}

	${MementoSection} "Lua" Lua
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\lua.xml"
	${MementoSectionEnd}

	${MementoSection} "AutoIt" AutoIt
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\autoit.xml"
	${MementoSectionEnd}

	${MementoSection} "COBOL" COBOL
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\cobol.xml"
	${MementoSectionEnd}

	${MementoSection} "TypeScript" TypeScript
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\typescript.xml"
	${MementoSectionEnd}
SectionGroupEnd



SectionGroup un.autoCompletionComponent
	Section un.PHP
		Delete "$INSTDIR\autoCompletion\php.xml"
	SectionEnd

	Section un.CSS
		Delete "$INSTDIR\autoCompletion\css.xml"
	SectionEnd	
	
	Section un.HTML
		Delete "$INSTDIR\autoCompletion\html.xml"
	SectionEnd
	
	Section un.SQL
		Delete "$INSTDIR\autoCompletion\sql.xml"
	SectionEnd
	
	Section un.RC
		Delete "$INSTDIR\autoCompletion\rc.xml"
	SectionEnd

	Section un.VB
		Delete "$INSTDIR\autoCompletion\vb.xml"
	SectionEnd

	Section un.Perl
		Delete "$INSTDIR\autoCompletion\perl.xml"
	SectionEnd

	Section un.C
		Delete "$INSTDIR\autoCompletion\c.xml"
	SectionEnd
	
	Section un.C++
		Delete "$INSTDIR\autoCompletion\cpp.xml"
	SectionEnd
	
	Section un.Java
		Delete "$INSTDIR\autoCompletion\java.xml"
	SectionEnd
	
	Section un.C#
		Delete "$INSTDIR\autoCompletion\cs.xml"
	SectionEnd
	
	Section un.JavaScript
		Delete "$INSTDIR\autoCompletion\javascript.xml"
	SectionEnd

	Section un.Python
		Delete "$INSTDIR\autoCompletion\python.xml"
	SectionEnd

	Section un.ActionScript
		Delete "$INSTDIR\autoCompletion\actionscript.xml"
	SectionEnd
	
	Section un.LISP
		Delete "$INSTDIR\autoCompletion\lisp.xml"
	SectionEnd
	
	Section un.VHDL
		Delete "$INSTDIR\autoCompletion\vhdl.xml"
	SectionEnd	
	
	Section un.TeX
		Delete "$INSTDIR\autoCompletion\tex.xml"
	SectionEnd
	
	Section un.DocBook
		Delete "$INSTDIR\autoCompletion\xml.xml"
	SectionEnd
	
	Section un.NSIS
		Delete "$INSTDIR\autoCompletion\nsis.xml"
	SectionEnd
	
	Section un.CMAKE
		Delete "$INSTDIR\autoCompletion\cmake.xml"
	SectionEnd

	Section un.BATCH
		Delete "$INSTDIR\autoCompletion\batch.xml"
	SectionEnd
	
	Section un.CoffeeScript
		Delete "$INSTDIR\autoCompletion\coffee.xml"
	SectionEnd

	Section un.BaanC
		Delete "$INSTDIR\autoCompletion\baanc.xml"
	SectionEnd

	Section un.Lua
		Delete "$INSTDIR\autoCompletion\lua.xml"
	SectionEnd

	Section un.AutoIt
		Delete "$INSTDIR\autoCompletion\autoit.xml"
	SectionEnd

	Section un.COBOL
		Delete "$INSTDIR\autoCompletion\cobol.xml"
	SectionEnd

	Section un.TypeScript
		Delete "$INSTDIR\autoCompletion\typescript.xml"
	SectionEnd

SectionGroupEnd
