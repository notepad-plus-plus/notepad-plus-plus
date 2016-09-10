SectionGroup "Auto-completion Files" autoCompletionComponent
	SetOverwrite off
	
	${MementoSection} "C" C
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\c.xml"
	${MementoSectionEnd}
	
	${MementoSection} "C++" C++
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\cpp.xml"
	${MementoSectionEnd}

	${MementoSection} "Java" Java
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\java.xml"
	${MementoSectionEnd}
	
	${MementoSection} "C#" C#
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\cs.xml"
	${MementoSectionEnd}
	
	${MementoSection} "HTML" HTML
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\html.xml"
	${MementoSectionEnd}
	
	${MementoSection} "RC" RC
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\rc.xml"
	${MementoSectionEnd}
	
	${MementoSection} "SQL" SQL
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\sql.xml"
	${MementoSectionEnd}
	
	${MementoSection} "PHP" PHP
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\php.xml"
	${MementoSectionEnd}

	${MementoSection} "CSS" CSS
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\css.xml"
	${MementoSectionEnd}

	${MementoSection} "VB" VB
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\vb.xml"
	${MementoSectionEnd}

	${MementoSection} "Perl" Perl
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\perl.xml"
	${MementoSectionEnd}
	
	${MementoSection} "JavaScript" JavaScript
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\javascript.xml"
	${MementoSectionEnd}

	${MementoSection} "Python" Python
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\python.xml"
	${MementoSectionEnd}
	
	${MementoSection} "ActionScript" ActionScript
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\actionscript.xml"
	${MementoSectionEnd}
	
	${MementoSection} "LISP" LISP
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\lisp.xml"
	${MementoSectionEnd}
	
	${MementoSection} "VHDL" VHDL
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\vhdl.xml"
	${MementoSectionEnd}
	
	${MementoSection} "TeX" TeX
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\tex.xml"
	${MementoSectionEnd}
	
	${MementoSection} "DocBook" DocBook
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\xml.xml"
	${MementoSectionEnd}
	
	${MementoSection} "NSIS" NSIS
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\nsis.xml"
	${MementoSectionEnd}
	
	${MementoSection} "CMAKE" CMAKE
		SetOutPath "$INSTDIR\plugins\APIs"
		File ".\APIs\cmake.xml"
	${MementoSectionEnd}
SectionGroupEnd



SectionGroup un.autoCompletionComponent
	Section un.PHP
		Delete "$INSTDIR\plugins\APIs\php.xml"
	SectionEnd

	Section un.CSS
		Delete "$INSTDIR\plugins\APIs\css.xml"
	SectionEnd	
	
	Section un.HTML
		Delete "$INSTDIR\plugins\APIs\html.xml"
	SectionEnd
	
	Section un.SQL
		Delete "$INSTDIR\plugins\APIs\sql.xml"
	SectionEnd
	
	Section un.RC
		Delete "$INSTDIR\plugins\APIs\rc.xml"
	SectionEnd

	Section un.VB
		Delete "$INSTDIR\plugins\APIs\vb.xml"
	SectionEnd

	Section un.Perl
		Delete "$INSTDIR\plugins\APIs\perl.xml"
	SectionEnd

	Section un.C
		Delete "$INSTDIR\plugins\APIs\c.xml"
	SectionEnd
	
	Section un.C++
		Delete "$INSTDIR\plugins\APIs\cpp.xml"
	SectionEnd
	
	Section un.Java
		Delete "$INSTDIR\plugins\APIs\java.xml"
	SectionEnd
	
	Section un.C#
		Delete "$INSTDIR\plugins\APIs\cs.xml"
	SectionEnd
	
	Section un.JavaScript
		Delete "$INSTDIR\plugins\APIs\javascript.xml"
	SectionEnd

	Section un.Python
		Delete "$INSTDIR\plugins\APIs\python.xml"
	SectionEnd

	Section un.ActionScript
		Delete "$INSTDIR\plugins\APIs\actionscript.xml"
	SectionEnd
	
	Section un.LISP
		Delete "$INSTDIR\plugins\APIs\lisp.xml"
	SectionEnd
	
	Section un.VHDL
		Delete "$INSTDIR\plugins\APIs\vhdl.xml"
	SectionEnd	
	
	Section un.TeX
		Delete "$INSTDIR\plugins\APIs\tex.xml"
	SectionEnd
	
	Section un.DocBook
		Delete "$INSTDIR\plugins\APIs\xml.xml"
	SectionEnd
	
	Section un.NSIS
		Delete "$INSTDIR\plugins\APIs\nsis.xml"
	SectionEnd
	
	Section un.AWK
		Delete "$INSTDIR\plugins\APIs\awk.xml"
	SectionEnd
	
	Section un.CMAKE
		Delete "$INSTDIR\plugins\APIs\cmake.xml"
	SectionEnd	
SectionGroupEnd
