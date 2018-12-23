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


SectionGroup "Auto-completion Files" autoCompletionComponent
	SetOverwrite off
	
	${MementoSection} "C" C
		Delete "$INSTDIR\plugins\APIs\c.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\c.xml"
	${MementoSectionEnd}
	
	${MementoSection} "C++" C++
		Delete "$INSTDIR\plugins\APIs\cpp.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\cpp.xml"
	${MementoSectionEnd}

	${MementoSection} "Java" Java
		Delete "$INSTDIR\plugins\APIs\java.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\java.xml"
	${MementoSectionEnd}
	
	${MementoSection} "C#" C#
		Delete "$INSTDIR\plugins\APIs\cs.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\cs.xml"
	${MementoSectionEnd}
	
	${MementoSection} "HTML" HTML
		Delete "$INSTDIR\plugins\APIs\html.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\html.xml"
	${MementoSectionEnd}
	
	${MementoSection} "RC" RC
		Delete "$INSTDIR\plugins\APIs\rc.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\rc.xml"
	${MementoSectionEnd}
	
	${MementoSection} "SQL" SQL
		Delete "$INSTDIR\plugins\APIs\sql.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\sql.xml"
	${MementoSectionEnd}
	
	${MementoSection} "PHP" PHP
		Delete "$INSTDIR\plugins\APIs\php.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\php.xml"
	${MementoSectionEnd}

	${MementoSection} "CSS" CSS
		Delete "$INSTDIR\plugins\APIs\css.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\css.xml"
	${MementoSectionEnd}

	${MementoSection} "VB" VB
		Delete "$INSTDIR\plugins\APIs\vb.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\vb.xml"
	${MementoSectionEnd}

	${MementoSection} "Perl" Perl
		Delete "$INSTDIR\plugins\APIs\perl.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\perl.xml"
	${MementoSectionEnd}
	
	${MementoSection} "JavaScript" JavaScript
		Delete "$INSTDIR\plugins\APIs\javascript.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\javascript.xml"
	${MementoSectionEnd}

	${MementoSection} "Python" Python
		Delete "$INSTDIR\plugins\APIs\python.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\python.xml"
	${MementoSectionEnd}
	
	${MementoSection} "ActionScript" ActionScript
		Delete "$INSTDIR\plugins\APIs\actionscript.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\actionscript.xml"
	${MementoSectionEnd}
	
	${MementoSection} "LISP" LISP
		Delete "$INSTDIR\plugins\APIs\lisp.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\lisp.xml"
	${MementoSectionEnd}
	
	${MementoSection} "VHDL" VHDL
		Delete "$INSTDIR\plugins\APIs\vhdl.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\vhdl.xml"
	${MementoSectionEnd}
	
	${MementoSection} "TeX" TeX
		Delete "$INSTDIR\plugins\APIs\tex.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\tex.xml"
	${MementoSectionEnd}
	
	${MementoSection} "DocBook" DocBook
		Delete "$INSTDIR\plugins\APIs\xml.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\xml.xml"
	${MementoSectionEnd}
	
	${MementoSection} "NSIS" NSIS
		Delete "$INSTDIR\plugins\APIs\nsis.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\nsis.xml"
	${MementoSectionEnd}
	
	${MementoSection} "CMAKE" CMAKE
		Delete "$INSTDIR\plugins\APIs\cmake.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\cmake.xml"
	${MementoSectionEnd}

	${MementoSection} "BATCH" BATCH
		Delete "$INSTDIR\plugins\APIs\batch.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\batch.xml"
	${MementoSectionEnd}
	
	${MementoSection} "CoffeeScript" CoffeeScript
		Delete "$INSTDIR\plugins\APIs\coffee.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\coffee.xml"
	${MementoSectionEnd}

	${MementoSection} "BaanC" BaanC
		Delete "$INSTDIR\plugins\APIs\baanc.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\baanc.xml"
	${MementoSectionEnd}

	${MementoSection} "Lua" Lua
		Delete "$INSTDIR\plugins\APIs\lua.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\lua.xml"
	${MementoSectionEnd}

	${MementoSection} "AutoIt" AutoIt
		Delete "$INSTDIR\plugins\APIs\autoit.xml"
		SetOutPath "$INSTDIR\autoCompletion"
		File ".\APIs\autoit.xml"
	${MementoSectionEnd}
SectionGroupEnd



SectionGroup un.autoCompletionComponent
	Section un.PHP
		Delete "$INSTDIR\plugins\APIs\php.xml"
		Delete "$INSTDIR\autoCompletion\php.xml"
	SectionEnd

	Section un.CSS
		Delete "$INSTDIR\plugins\APIs\css.xml"
		Delete "$INSTDIR\autoCompletion\css.xml"
	SectionEnd	
	
	Section un.HTML
		Delete "$INSTDIR\plugins\APIs\html.xml"
		Delete "$INSTDIR\autoCompletion\html.xml"
	SectionEnd
	
	Section un.SQL
		Delete "$INSTDIR\plugins\APIs\sql.xml"
		Delete "$INSTDIR\autoCompletion\sql.xml"
	SectionEnd
	
	Section un.RC
		Delete "$INSTDIR\plugins\APIs\rc.xml"
		Delete "$INSTDIR\autoCompletion\rc.xml"
	SectionEnd

	Section un.VB
		Delete "$INSTDIR\plugins\APIs\vb.xml"
		Delete "$INSTDIR\autoCompletion\vb.xml"
	SectionEnd

	Section un.Perl
		Delete "$INSTDIR\plugins\APIs\perl.xml"
		Delete "$INSTDIR\autoCompletion\perl.xml"
	SectionEnd

	Section un.C
		Delete "$INSTDIR\plugins\APIs\c.xml"
		Delete "$INSTDIR\autoCompletion\c.xml"
	SectionEnd
	
	Section un.C++
		Delete "$INSTDIR\plugins\APIs\cpp.xml"
		Delete "$INSTDIR\autoCompletion\cpp.xml"
	SectionEnd
	
	Section un.Java
		Delete "$INSTDIR\plugins\APIs\java.xml"
		Delete "$INSTDIR\autoCompletion\java.xml"
	SectionEnd
	
	Section un.C#
		Delete "$INSTDIR\plugins\APIs\cs.xml"
		Delete "$INSTDIR\autoCompletion\cs.xml"
	SectionEnd
	
	Section un.JavaScript
		Delete "$INSTDIR\plugins\APIs\javascript.xml"
		Delete "$INSTDIR\autoCompletion\javascript.xml"
	SectionEnd

	Section un.Python
		Delete "$INSTDIR\plugins\APIs\python.xml"
		Delete "$INSTDIR\autoCompletion\python.xml"
	SectionEnd

	Section un.ActionScript
		Delete "$INSTDIR\plugins\APIs\actionscript.xml"
		Delete "$INSTDIR\autoCompletion\actionscript.xml"
	SectionEnd
	
	Section un.LISP
		Delete "$INSTDIR\plugins\APIs\lisp.xml"
		Delete "$INSTDIR\autoCompletion\lisp.xml"
	SectionEnd
	
	Section un.VHDL
		Delete "$INSTDIR\plugins\APIs\vhdl.xml"
		Delete "$INSTDIR\autoCompletion\vhdl.xml"
	SectionEnd	
	
	Section un.TeX
		Delete "$INSTDIR\plugins\APIs\tex.xml"
		Delete "$INSTDIR\autoCompletion\tex.xml"
	SectionEnd
	
	Section un.DocBook
		Delete "$INSTDIR\plugins\APIs\xml.xml"
		Delete "$INSTDIR\autoCompletion\xml.xml"
	SectionEnd
	
	Section un.NSIS
		Delete "$INSTDIR\plugins\APIs\nsis.xml"
		Delete "$INSTDIR\autoCompletion\nsis.xml"
	SectionEnd
	
	Section un.CMAKE
		Delete "$INSTDIR\plugins\APIs\cmake.xml"
		Delete "$INSTDIR\autoCompletion\cmake.xml"
	SectionEnd

	Section un.BATCH
		Delete "$INSTDIR\plugins\APIs\batch.xml"
		Delete "$INSTDIR\autoCompletion\batch.xml"
	SectionEnd
	
	Section un.CoffeeScript
		Delete "$INSTDIR\plugins\APIs\coffee.xml"
		Delete "$INSTDIR\autoCompletion\coffee.xml"
	SectionEnd

	Section un.BaanC
		Delete "$INSTDIR\plugins\APIs\baanc.xml"
		Delete "$INSTDIR\autoCompletion\baanc.xml"
	SectionEnd

	Section un.Lua
		Delete "$INSTDIR\plugins\APIs\lua.xml"
		Delete "$INSTDIR\autoCompletion\lua.xml"
	SectionEnd


	Section un.autoit
		Delete "$INSTDIR\plugins\APIs\autoit.xml"
		Delete "$INSTDIR\autoCompletion\autoit.xml"
	SectionEnd

SectionGroupEnd
