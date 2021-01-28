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


SectionGroup "Themes" Themes
	SetOverwrite off
	; UPDATE_PATH: the value is $INSTDIR if doLocalConf.xml exit,
	;              otherwise the value is $APPDATA\${APPNAME}
	${MementoSection} "Black Board" BlackBoard
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Black board.xml"
	${MementoSectionEnd}

	${MementoSection} "Choco" Choco
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Choco.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Hello Kitty" HelloKitty
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Hello Kitty.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Mono Industrial" MonoIndustrial
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Mono Industrial.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Monokai" Monokai
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Monokai.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Obsidian" Obsidian
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\obsidian.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Plastic Code Wrap" PlasticCodeWrap
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Plastic Code Wrap.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Ruby Blue" RubyBlue
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Ruby Blue.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Twilight" Twilight
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Twilight.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Vibrant Ink" VibrantInk
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Vibrant Ink.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Deep Black" DeepBlack
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Deep Black.xml"
	${MementoSectionEnd}
	
	${MementoSection} "vim Dark Blue" vimDarkBlue
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\vim Dark Blue.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Bespin" Bespin
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Bespin.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Zenburn" Zenburn
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Zenburn.xml"
	${MementoSectionEnd}

	${MementoSection} "Solarized" Solarized
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Solarized.xml"
	${MementoSectionEnd}

	${MementoSection} "Solarized Light" Solarized-light
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Solarized-light.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Hot Fudge Sundae" HotFudgeSundae
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\HotFudgeSundae.xml"
	${MementoSectionEnd}
	
	${MementoSection} "khaki" khaki
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\khaki.xml"
	${MementoSectionEnd}

	${MementoSection} "Mossy Lawn" MossyLawn
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\MossyLawn.xml"
	${MementoSectionEnd}

	${MementoSection} "Navajo" Navajo
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\Navajo.xml"
	${MementoSectionEnd}

	${MementoSection} "DansLeRuSH Dark" DansLeRuSHDark
		SetOutPath "$UPDATE_PATH\themes"
		File ".\themes\DansLeRuSH-Dark.xml"
	${MementoSectionEnd}
SectionGroupEnd


SectionGroup un.Themes
	
	Section un.BlackBoard
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Black board.xml"
	${endIf}
	SectionEnd

	Section un.Choco
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Choco.xml"
	${endIf}
	SectionEnd
	
	Section un.HelloKitty
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Hello Kitty.xml"
	${endIf}
	SectionEnd
	
	Section un.MonoIndustrial
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Mono Industrial.xml"
	${endIf}
	SectionEnd
	
	Section un.Monokai
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Monokai.xml"
	${endIf}
	SectionEnd
	
	Section un.Obsidian
	${If} $keepUserData == "false"
		Delete "$installPath\themes\obsidian.xml"
	${endIf}
	SectionEnd
	
	Section un.PlasticCodeWrap
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Plastic Code Wrap.xml"
	${endIf}
	SectionEnd
	
	Section un.RubyBlue
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Ruby Blue.xml"
	${endIf}
	SectionEnd
	
	Section un.Twilight
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Twilight.xml"
	${endIf}
	SectionEnd
	
	Section un.VibrantInk
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Vibrant Ink.xml"
	${endIf}
	SectionEnd

	Section un.DeepBlack
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Deep Black.xml"
	${endIf}
	SectionEnd
	
	Section un.vimDarkBlue
	${If} $keepUserData == "false"
		Delete "$installPath\themes\vim Dark Blue.xml"
	${endIf}
	SectionEnd
	
	Section un.Bespin
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Bespin.xml"
	${endIf}
	SectionEnd
	
	Section un.Zenburn
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Zenburn.xml"
	${endIf}
	SectionEnd

	Section un.Solarized
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Solarized.xml"
	${endIf}
	SectionEnd

	Section un.Solarized-light
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Solarized-light.xml"
	${endIf}
	SectionEnd
	
	Section un.HotFudgeSundae
	${If} $keepUserData == "false"
		Delete "$installPath\themes\HotFudgeSundae.xml"
	${endIf}
	SectionEnd

	Section un.khaki
	${If} $keepUserData == "false"
		Delete "$installPath\themes\khaki.xml"
	${endIf}
	SectionEnd
	
	Section un.MossyLawn
	${If} $keepUserData == "false"
		Delete "$installPath\themes\MossyLawn.xml"
	${endIf}
	SectionEnd

	Section un.Navajo
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Navajo.xml"
	${endIf}
	SectionEnd

	Section un.DansLeRuSHDark
	${If} $keepUserData == "false"
		Delete "$installPath\themes\DansLeRuSH-Dark.xml"
	${endIf}
	SectionEnd
	
SectionGroupEnd
