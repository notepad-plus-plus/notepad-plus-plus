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


; Set languages (first is default language)
;!insertmacro MUI_LANGUAGE "English"
!define MUI_LANGDLL_ALLLANGUAGES
;Languages

  !insertmacro MUI_LANGUAGE "Afrikaans"
  !insertmacro MUI_LANGUAGE "Albanian"
  !insertmacro MUI_LANGUAGE "Arabic"
  ;!insertmacro MUI_LANGUAGE "Armenian"
  !insertmacro MUI_LANGUAGE "Basque"
  !insertmacro MUI_LANGUAGE "Belarusian"
  !insertmacro MUI_LANGUAGE "Bosnian"
  !insertmacro MUI_LANGUAGE "Breton"
  !insertmacro MUI_LANGUAGE "Bulgarian"
  !insertmacro MUI_LANGUAGE "Catalan"
  !insertmacro MUI_LANGUAGE "Corsican"
  !insertmacro MUI_LANGUAGE "Croatian"
  !insertmacro MUI_LANGUAGE "Czech"
  !insertmacro MUI_LANGUAGE "Danish"
  !insertmacro MUI_LANGUAGE "Dutch"
  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "Farsi"
  !insertmacro MUI_LANGUAGE "Finnish"
  !insertmacro MUI_LANGUAGE "Estonian"
  !insertmacro MUI_LANGUAGE "French"
  !insertmacro MUI_LANGUAGE "Galician"
  !insertmacro MUI_LANGUAGE "Georgian"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Greek"
  !insertmacro MUI_LANGUAGE "Hebrew"
  !insertmacro MUI_LANGUAGE "Hindi"
  !insertmacro MUI_LANGUAGE "Hungarian"
  ;!insertmacro MUI_LANGUAGE "Icelandic"
  !insertmacro MUI_LANGUAGE "Irish"
  !insertmacro MUI_LANGUAGE "Indonesian"
  !insertmacro MUI_LANGUAGE "Italian"
  !insertmacro MUI_LANGUAGE "Japanese"
  !insertmacro MUI_LANGUAGE "Korean"
  !insertmacro MUI_LANGUAGE "Kurdish"
  !insertmacro MUI_LANGUAGE "Latvian"
  !insertmacro MUI_LANGUAGE "Lithuanian"
  !insertmacro MUI_LANGUAGE "Luxembourgish"
  !insertmacro MUI_LANGUAGE "Macedonian"
  !insertmacro MUI_LANGUAGE "Malay"
  !insertmacro MUI_LANGUAGE "Mongolian"
  !insertmacro MUI_LANGUAGE "Norwegian"
  !insertmacro MUI_LANGUAGE "NorwegianNynorsk"
  !insertmacro MUI_LANGUAGE "Polish"
  !insertmacro MUI_LANGUAGE "Romanian"
  !insertmacro MUI_LANGUAGE "Portuguese"
  !insertmacro MUI_LANGUAGE "PortugueseBR"
  !insertmacro MUI_LANGUAGE "Russian"
  !insertmacro MUI_LANGUAGE "Serbian"
  !insertmacro MUI_LANGUAGE "Spanish"
  !insertmacro MUI_LANGUAGE "SimpChinese"
  !insertmacro MUI_LANGUAGE "Slovenian"
  !insertmacro MUI_LANGUAGE "Slovak"
  !insertmacro MUI_LANGUAGE "Swedish"
  !insertmacro MUI_LANGUAGE "Thai"
  !insertmacro MUI_LANGUAGE "TradChinese"
  !insertmacro MUI_LANGUAGE "Turkish"
  !insertmacro MUI_LANGUAGE "Ukrainian"
  !insertmacro MUI_LANGUAGE "Uzbek"
  !insertmacro MUI_LANGUAGE "Vietnamese"
  !insertmacro MUI_LANGUAGE "Welsh"
  
  
!insertmacro MUI_RESERVEFILE_LANGDLL


LangString langFileName ${LANG_ENGLISH} "english.xml"
LangString langFileName ${LANG_FRENCH} "french.xml"
LangString langFileName ${LANG_TRADCHINESE} "taiwaneseMandarin.xml"
LangString langFileName ${LANG_SIMPCHINESE} "chineseSimplified.xml"
LangString langFileName ${LANG_KOREAN} "korean.xml"
LangString langFileName ${LANG_JAPANESE} "japanese.xml"
LangString langFileName ${LANG_GERMAN} "german.xml"
LangString langFileName ${LANG_SPANISH} "spanish.xml"
LangString langFileName ${LANG_ITALIAN} "italian.xml"
LangString langFileName ${LANG_PORTUGUESE} "portuguese.xml"
LangString langFileName ${LANG_PORTUGUESEBR} "brazilian_portuguese.xml"
LangString langFileName ${LANG_DUTCH} "dutch.xml"
LangString langFileName ${LANG_RUSSIAN} "russian.xml"
LangString langFileName ${LANG_POLISH} "polish.xml"
LangString langFileName ${LANG_CATALAN} "catalan.xml"
LangString langFileName ${LANG_CZECH} "czech.xml"
LangString langFileName ${LANG_HUNGARIAN} "hungarian.xml"
LangString langFileName ${LANG_ROMANIAN} "romanian.xml"
LangString langFileName ${LANG_TURKISH} "turkish.xml"
LangString langFileName ${LANG_FARSI} "farsi.xml"
LangString langFileName ${LANG_UKRAINIAN} "ukrainian.xml"
LangString langFileName ${LANG_HEBREW} "hebrew.xml"
LangString langFileName ${LANG_HINDI} "hindi.xml"
LangString langFileName ${LANG_NORWEGIANNYNORSK} "nynorsk.xml"
LangString langFileName ${LANG_NORWEGIAN} "norwegian.xml"
LangString langFileName ${LANG_THAI} "thai.xml"
LangString langFileName ${LANG_ARABIC} "arabic.xml"
LangString langFileName ${LANG_FINNISH} "finnish.xml"
LangString langFileName ${LANG_LITHUANIAN} "lithuanian.xml"
LangString langFileName ${LANG_GREEK} "greek.xml"
LangString langFileName ${LANG_SWEDISH} "swedish.xml"
LangString langFileName ${LANG_GALICIAN} "galician.xml"
LangString langFileName ${LANG_SLOVENIAN} "slovenian.xml"
LangString langFileName ${LANG_SLOVAK} "slovak.xml"
LangString langFileName ${LANG_DANISH} "danish.xml"
LangString langFileName ${LANG_BULGARIAN} "bulgarian.xml"
LangString langFileName ${LANG_INDONESIAN} "indonesian.xml"
LangString langFileName ${LANG_ALBANIAN} "albanian.xml"
LangString langFileName ${LANG_CROATIAN} "croatian.xml"
LangString langFileName ${LANG_BASQUE} "basque.xml"
LangString langFileName ${LANG_BELARUSIAN} "belarusian.xml"
LangString langFileName ${LANG_SERBIAN} "serbian.xml"
LangString langFileName ${LANG_MALAY} "malay.xml"
LangString langFileName ${LANG_LUXEMBOURGISH} "luxembourgish.xml"
LangString langFileName ${LANG_AFRIKAANS} "afrikaans.xml"
LangString langFileName ${LANG_UZBEK} "uzbek.xml"
LangString langFileName ${LANG_MACEDONIAN} "macedonian.xml"
LangString langFileName ${LANG_LATVIAN} "Latvian.xml"
LangString langFileName ${LANG_BOSNIAN} "bosnian.xml"
LangString langFileName ${LANG_MONGOLIAN} "mongolian.xml"
LangString langFileName ${LANG_ESTONIAN} "estonian.xml"
LangString langFileName ${LANG_CORSICAN} "corsican.xml"
LangString langFileName ${LANG_BRETON} "breton.xml"
LangString langFileName ${LANG_GEORGIAN} "georgian.xml"
LangString langFileName ${LANG_VIETNAMESE} "vietnamese.xml"
LangString langFileName ${LANG_WELSH} "welsh.xml"
LangString langFileName ${LANG_KURDISH} "kurdish.xml"
LangString langFileName ${LANG_IRISH} "irish.xml"


Function unsupportedLanguageToEnglish
	${Switch} $LANGUAGE
		${Case} ${LANG_AFRIKAANS}
		${Case} ${LANG_ALBANIAN}
		${Case} ${LANG_ARABIC}
		${Case} ${LANG_BASQUE}
		${Case} ${LANG_BELARUSIAN}
		${Case} ${LANG_BOSNIAN}
		${Case} ${LANG_BRETON}
		${Case} ${LANG_BULGARIAN}
		${Case} ${LANG_CATALAN}
		${Case} ${LANG_CORSICAN}
		${Case} ${LANG_CROATIAN}
		${Case} ${LANG_CZECH}
		${Case} ${LANG_DANISH}
		${Case} ${LANG_DUTCH}
		${Case} ${LANG_ENGLISH}
		${Case} ${LANG_ESTONIAN}
		${Case} ${LANG_FARSI}
		${Case} ${LANG_FINNISH}
		${Case} ${LANG_FRENCH}
		${Case} ${LANG_GALICIAN}
		${Case} ${LANG_GEORGIAN}
		${Case} ${LANG_GERMAN}
		${Case} ${LANG_GREEK}
		${Case} ${LANG_HEBREW}
		${Case} ${LANG_HINDI}
		${Case} ${LANG_HUNGARIAN}
		${Case} ${LANG_INDONESIAN}
		${Case} ${LANG_IRISH}
		${Case} ${LANG_ITALIAN}
		${Case} ${LANG_JAPANESE}
		${Case} ${LANG_KOREAN}
		${Case} ${LANG_KURDISH}
		${Case} ${LANG_LATVIAN}
		${Case} ${LANG_LITHUANIAN}
		${Case} ${LANG_LUXEMBOURGISH}
		${Case} ${LANG_MACEDONIAN}
		${Case} ${LANG_MALAY}
		${Case} ${LANG_MONGOLIAN}
		${Case} ${LANG_NORWEGIANNYNORSK}
		${Case} ${LANG_NORWEGIAN}
		${Case} ${LANG_POLISH}
		${Case} ${LANG_PORTUGUESEBR}
		${Case} ${LANG_PORTUGUESE}
		${Case} ${LANG_ROMANIAN}
		${Case} ${LANG_RUSSIAN}
		${Case} ${LANG_SERBIAN}
		${Case} ${LANG_SIMPCHINESE}
		${Case} ${LANG_SLOVAK}
		${Case} ${LANG_SLOVENIAN}
		${Case} ${LANG_SPANISH}
		${Case} ${LANG_SWEDISH}
		${Case} ${LANG_THAI}
		${Case} ${LANG_TRADCHINESE}
		${Case} ${LANG_TURKISH}
		${Case} ${LANG_UKRAINIAN}
		${Case} ${LANG_UZBEK}
		${Case} ${LANG_VIETNAMESE}
		${Case} ${LANG_WELSH}
			; language is supported
			${Break}
		${Default}
			; set language to English
			StrCpy $LANGUAGE ${LANG_ENGLISH}
			${Break}
	${EndSwitch}
FunctionEnd
