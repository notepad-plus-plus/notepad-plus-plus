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


SectionGroup "Function List Files" functionListComponent
	SetOverwrite on

	${MementoSection} "C" C_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\c.xml"
	${MementoSectionEnd}

	${MementoSection} "C++" C++_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\cpp.xml"
	${MementoSectionEnd}

	${MementoSection} "Java" Java_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\java.xml"
	${MementoSectionEnd}

	${MementoSection} "C#" C#_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\cs.xml"
	${MementoSectionEnd}

	${MementoSection} "CSS" CSS_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\css.xml"
	${MementoSectionEnd}

	${MementoSection} "Assembly" Assembly_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\asm.xml"
	${MementoSectionEnd}

	${MementoSection} "Bash" Bash_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\bash.xml"
	${MementoSectionEnd}

	${MementoSection} "SQL" SQL_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\sql.xml"
	${MementoSectionEnd}

	${MementoSection} "PHP" PHP_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\php.xml"
	${MementoSectionEnd}

	${MementoSection} "COBOL section free" COBOL-section-free
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\cobol-free.xml"
	${MementoSectionEnd}

	${MementoSection} "COBOL" COBOL_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\cobol.xml"
	${MementoSectionEnd}

	${MementoSection} "Perl" Perl_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\perl.xml"
	${MementoSectionEnd}

	${MementoSection} "JavaScript" JavaScript_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\javascript.js.xml"
	${MementoSectionEnd}

	${MementoSection} "Python" Python_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\python.xml"
	${MementoSectionEnd}

	${MementoSection} "Lua" Lua_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\lua.xml"
	${MementoSectionEnd}

	${MementoSection} "ini" ini_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\ini.xml"
	${MementoSectionEnd}

	${MementoSection} "Inno Setup" Innosetup_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\inno.xml"
	${MementoSectionEnd}

	${MementoSection} "VHDL" VHDL_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\vhdl.xml"
	${MementoSectionEnd}

	${MementoSection} "KRL" KRL_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\krl.xml"
	${MementoSectionEnd}

	${MementoSection} "NSIS" NSIS_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\nsis.xml"
	${MementoSectionEnd}

	${MementoSection} "PowerShell" PowerShell_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\powershell.xml"
	${MementoSectionEnd}

	${MementoSection} "BATCH" BATCH_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\batch.xml"
	${MementoSectionEnd}

	${MementoSection} "Ruby" Ruby_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\ruby.xml"
	${MementoSectionEnd}

	${MementoSection} "BaanC" BaanC_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\baanc.xml"
	${MementoSectionEnd}

	${MementoSection} "Sinumerik" Sinumerik_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\sinumerik.xml"
	${MementoSectionEnd}

	${MementoSection} "AutoIt" AutoIt_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\autoit.xml"
	${MementoSectionEnd}

	${MementoSection} "UniVerse BASIC" UniVerseBASIC_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\universe_basic.xml"
	${MementoSectionEnd}

	${MementoSection} "XML" XML_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\xml.xml"
	${MementoSectionEnd}

	${MementoSection} "Ada" Ada_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\ada.xml"
	${MementoSectionEnd}

	${MementoSection} "Fortran" Fortran_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\fortran.xml"
	${MementoSectionEnd}

	${MementoSection} "Fortran77" Fortran77_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\fortran77.xml"
	${MementoSectionEnd}

	${MementoSection} "Haskell" Haskell_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\haskell.xml"
	${MementoSectionEnd}

	${MementoSection} "Rust" Rust_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\rust.xml"
	${MementoSectionEnd}

	${MementoSection} "TypeScript" TypeScript_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\typescript.xml"
	${MementoSectionEnd}

	${MementoSection} "Pascal" Pascal_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\pascal.xml"
	${MementoSectionEnd}

	${MementoSection} "GDScript" GDScript_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\gdscript.xml"
	${MementoSectionEnd}

	${MementoSection} "Raku" Raku_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\raku.xml"
	${MementoSectionEnd}

	${MementoSection} "Hollywood" Hollywood_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\hollywood.xml"
	${MementoSectionEnd}

	${MementoSection} "Toml" Toml_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\toml.xml"
	${MementoSectionEnd}

	${MementoSection} "TeX" Tex_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\tex.xml"
	${MementoSectionEnd}

	${MementoSection} "LaTeX" Latex_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\latex.xml"
	${MementoSectionEnd}

	${MementoSection} "VisualBasic" VB_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\vb.xml"
	${MementoSectionEnd}

	${MementoSection} "NppExecScript" NppExecScript_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\nppexec.xml"
	${MementoSectionEnd}

	${MementoSection} "SAS" SAS_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\sas.xml"
	${MementoSectionEnd}

	SetOverwrite off
	${MementoSection} "Override Map" OverrideMap_FL
		SetOutPath "$INSTDIR\functionList"
		File ".\functionList\overrideMap.xml"
	${MementoSectionEnd}
SectionGroupEnd



SectionGroup un.functionListComponent
	Section un.PHP_FL
		Delete "$INSTDIR\functionList\php.xml"
	SectionEnd

	Section un.Assembly_FL
		Delete "$INSTDIR\functionList\asm.xml"
	SectionEnd

	Section un.SQL_FL
		Delete "$INSTDIR\functionList\sql.xml"
	SectionEnd

	Section un.Bash_FL
		Delete "$INSTDIR\functionList\bash.xml"
	SectionEnd

	Section un.COBOL-section-free_FL
		Delete "$INSTDIR\functionList\cobol-free.xml"
	SectionEnd

	Section un.Perl_FL
		Delete "$INSTDIR\functionList\perl.xml"
	SectionEnd

	Section un.C_FL
		Delete "$INSTDIR\functionList\c.xml"
	SectionEnd

	Section un.C++_FL
		Delete "$INSTDIR\functionList\cpp.xml"
	SectionEnd

	Section un.Java_FL
		Delete "$INSTDIR\functionList\java.xml"
	SectionEnd

	Section un.C#_FL
		Delete "$INSTDIR\functionList\cs.xml"
	SectionEnd

	Section un.CSS_FL
		Delete "$INSTDIR\functionList\css.xml"
	SectionEnd

	Section un.JavaScript_FL
		Delete "$INSTDIR\functionList\javascript.js.xml"
	SectionEnd

	Section un.Python_FL
		Delete "$INSTDIR\functionList\python.xml"
	SectionEnd

	Section un.Lua_FL
		Delete "$INSTDIR\functionList\lua.xml"
	SectionEnd

	Section un.COBOL_FL
		Delete "$INSTDIR\functionList\cobol.xml"
	SectionEnd

	Section un.ini_FL
		Delete "$INSTDIR\functionList\ini.xml"
	SectionEnd

	Section un.VHDL_FL
		Delete "$INSTDIR\functionList\vhdl.xml"
	SectionEnd

	Section un.Innosetup_FL
		Delete "$INSTDIR\functionList\inno.xml"
	SectionEnd

	Section un.XML_FL
		Delete "$INSTDIR\functionList\xml.xml"
	SectionEnd

	Section un.NSIS_FL
		Delete "$INSTDIR\functionList\nsis.xml"
	SectionEnd

	Section un.KRL_FL
		Delete "$INSTDIR\functionList\krl.xml"
	SectionEnd

	Section un.BATCH_FL
		Delete "$INSTDIR\functionList\batch.xml"
	SectionEnd

	Section un.OverrideMap_FL
		Delete "$INSTDIR\functionList\overrideMap.xml"
	SectionEnd

	Section un.BaanC_FL
		Delete "$INSTDIR\functionList\baanc.xml"
	SectionEnd

	Section un.PowerShell_FL
		Delete "$INSTDIR\functionList\powershell.xml"
	SectionEnd

	Section un.AutoIt_FL
		Delete "$INSTDIR\functionList\autoit.xml"
	SectionEnd

	Section un.Ruby_FL
		Delete "$INSTDIR\functionList\ruby.xml"
	SectionEnd

	Section un.UniVerseBASIC_FL
		Delete "$INSTDIR\functionList\universe_basic.xml"
	SectionEnd

	Section un.Sinumerik_FL
		Delete "$INSTDIR\functionList\sinumerik.xml"
	SectionEnd

	Section un.Ada_FL
		Delete "$INSTDIR\functionList\ada.xml"
	SectionEnd

	Section un.Fortran_FL
		Delete "$INSTDIR\functionList\fortran.xml"
	SectionEnd

	Section un.Fortran77_FL
		Delete "$INSTDIR\functionList\fortran77.xml"
	SectionEnd

	Section un.Haskell_FL
		Delete "$INSTDIR\functionList\haskell.xml"
	SectionEnd

	Section un.Rust_FL
		Delete "$INSTDIR\functionList\rust.xml"
	SectionEnd

	Section un.TypeScript_FL
		Delete "$INSTDIR\functionList\typescript.xml"
	SectionEnd

	Section un.Pascal_FL
		Delete "$INSTDIR\functionList\pascal.xml"
	SectionEnd

	Section un.GDScript_FL
		Delete "$INSTDIR\functionList\gdscript.xml"
	SectionEnd

	Section un.Raku_FL
		Delete "$INSTDIR\functionList\raku.xml"
	SectionEnd

	Section un.Hollywood_FL
		Delete "$INSTDIR\functionList\hollywood.xml"
	SectionEnd

	Section un.Toml_FL
		Delete "$INSTDIR\functionList\toml.xml"
	SectionEnd

	Section un.Tex_FL
		Delete "$INSTDIR\functionList\tex.xml"
	SectionEnd

	Section un.Latex_FL
		Delete "$INSTDIR\functionList\latex.xml"
	SectionEnd

	Section un.VB_FL
		Delete "$INSTDIR\functionList\vb.xml"
	SectionEnd

	Section un.NppExecScript_FL
		Delete "$INSTDIR\functionList\nppexec.xml"
	SectionEnd

	Section un.SAS_FL
		Delete "$INSTDIR\functionList\sas.xml"
	SectionEnd

SectionGroupEnd
