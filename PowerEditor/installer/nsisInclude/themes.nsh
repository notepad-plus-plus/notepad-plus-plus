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
	SetOverwrite on

	Section "-Dark Mode Default" DarkModeDefault
		SetOutPath "$INSTDIR\themes"
		File ".\themes\DarkModeDefault.xml"
	SectionEnd
	
	${MementoSection} "Black Board" BlackBoard
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Black board.xml"
	${MementoSectionEnd}

	${MementoSection} "Choco" Choco
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Choco.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Hello Kitty" HelloKitty
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Hello Kitty.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Mono Industrial" MonoIndustrial
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Mono Industrial.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Monokai" Monokai
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Monokai.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Obsidian" Obsidian
		SetOutPath "$INSTDIR\themes"
		File ".\themes\obsidian.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Plastic Code Wrap" PlasticCodeWrap
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Plastic Code Wrap.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Ruby Blue" RubyBlue
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Ruby Blue.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Twilight" Twilight
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Twilight.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Vibrant Ink" VibrantInk
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Vibrant Ink.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Deep Black" DeepBlack
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Deep Black.xml"
	${MementoSectionEnd}
	
	${MementoSection} "vim Dark Blue" vimDarkBlue
		SetOutPath "$INSTDIR\themes"
		File ".\themes\vim Dark Blue.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Bespin" Bespin
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Bespin.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Zenburn" Zenburn
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Zenburn.xml"
	${MementoSectionEnd}

	${MementoSection} "Solarized" Solarized
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Solarized.xml"
	${MementoSectionEnd}

	${MementoSection} "Solarized Light" Solarized-light
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Solarized-light.xml"
	${MementoSectionEnd}
	
	${MementoSection} "Hot Fudge Sundae" HotFudgeSundae
		SetOutPath "$INSTDIR\themes"
		File ".\themes\HotFudgeSundae.xml"
	${MementoSectionEnd}
	
	${MementoSection} "khaki" khaki
		SetOutPath "$INSTDIR\themes"
		File ".\themes\khaki.xml"
	${MementoSectionEnd}

	${MementoSection} "Mossy Lawn" MossyLawn
		SetOutPath "$INSTDIR\themes"
		File ".\themes\MossyLawn.xml"
	${MementoSectionEnd}

	${MementoSection} "Navajo" Navajo
		SetOutPath "$INSTDIR\themes"
		File ".\themes\Navajo.xml"
	${MementoSectionEnd}

	${MementoSection} "DansLeRuSH Dark" DansLeRuSHDark
		SetOutPath "$INSTDIR\themes"
		File ".\themes\DansLeRuSH-Dark.xml"
	${MementoSectionEnd}
SectionGroupEnd


SectionGroup un.Themes
	Section un.DarkModeDefault
	Delete "$INSTDIR\themes\DarkModeDefault.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\DarkModeDefault.xml"
	${endIf}
	SectionEnd
	
	Section un.BlackBoard
	Delete "$INSTDIR\themes\Black board.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Black board.xml"
	${endIf}
	SectionEnd

	Section un.Choco
	Delete "$INSTDIR\themes\Choco.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Choco.xml"
	${endIf}
	SectionEnd
	
	Section un.HelloKitty
	Delete "$INSTDIR\themes\Hello Kitty.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Hello Kitty.xml"
	${endIf}
	SectionEnd
	
	Section un.MonoIndustrial
	Delete "$INSTDIR\themes\Mono Industrial.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Mono Industrial.xml"
	${endIf}
	SectionEnd
	
	Section un.Monokai
	Delete "$INSTDIR\themes\Monokai.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Monokai.xml"
	${endIf}
	SectionEnd
	
	Section un.Obsidian
	Delete "$INSTDIR\themes\obsidian.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\obsidian.xml"
	${endIf}
	SectionEnd
	
	Section un.PlasticCodeWrap
	Delete "$INSTDIR\themes\Plastic Code Wrap.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Plastic Code Wrap.xml"
	${endIf}
	SectionEnd
	
	Section un.RubyBlue
	Delete "$INSTDIR\themes\Ruby Blue.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Ruby Blue.xml"
	${endIf}
	SectionEnd
	
	Section un.Twilight
	Delete "$INSTDIR\themes\Twilight.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Twilight.xml"
	${endIf}
	SectionEnd
	
	Section un.VibrantInk
	Delete "$INSTDIR\themes\Vibrant Ink.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Vibrant Ink.xml"
	${endIf}
	SectionEnd

	Section un.DeepBlack
	Delete "$INSTDIR\themes\Deep Black.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Deep Black.xml"
	${endIf}
	SectionEnd
	
	Section un.vimDarkBlue
	Delete "$INSTDIR\themes\vim Dark Blue.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\vim Dark Blue.xml"
	${endIf}
	SectionEnd
	
	Section un.Bespin
	Delete "$INSTDIR\themes\Bespin.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Bespin.xml"
	${endIf}
	SectionEnd
	
	Section un.Zenburn
	Delete "$INSTDIR\themes\Zenburn.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Zenburn.xml"
	${endIf}
	SectionEnd

	Section un.Solarized
	Delete "$INSTDIR\themes\Solarized.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Solarized.xml"
	${endIf}
	SectionEnd

	Section un.Solarized-light
	Delete "$INSTDIR\themes\Solarized-light.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Solarized-light.xml"
	${endIf}
	SectionEnd
	
	Section un.HotFudgeSundae
	Delete "$INSTDIR\themes\HotFudgeSundae.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\HotFudgeSundae.xml"
	${endIf}
	SectionEnd

	Section un.khaki
	Delete "$INSTDIR\themes\khaki.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\khaki.xml"
	${endIf}
	SectionEnd
	
	Section un.MossyLawn
	Delete "$INSTDIR\themes\MossyLawn.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\MossyLawn.xml"
	${endIf}
	SectionEnd

	Section un.Navajo
	Delete "$INSTDIR\themes\Navajo.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\Navajo.xml"
	${endIf}
	SectionEnd

	Section un.DansLeRuSHDark
	Delete "$INSTDIR\themes\DansLeRuSH-Dark.xml"
	${If} $keepUserData == "false"
		Delete "$installPath\themes\DansLeRuSH-Dark.xml"
	${endIf}
	SectionEnd
	
SectionGroupEnd
