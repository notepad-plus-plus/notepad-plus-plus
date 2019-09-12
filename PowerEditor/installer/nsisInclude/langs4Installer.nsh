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
  ;!insertmacro MUI_LANGUAGE "Irish"
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

!macro USE_LANG_DEFAULT LANGUAGE XML_FILE
       !define LANG_DEFAULT ${LANGUAGE}
       LangString langFileName ${LANGUAGE} "${XML_FILE}"
       !include "nsisInclude\lang\Default.nsh"
       !undef LANG_DEFAULT
!macroend
!define UseLangDefault "!insertmacro USE_LANG_DEFAULT "

!macro USE_LANG_TRANSLATED LANGUAGE LANGUAGE_NAME
       !define LANG ${LANGUAGE}
       LangString langFileName ${LANGUAGE} "${LANGUAGE_NAME}.xml"
       !include "nsisInclude\lang\${LANGUAGE_NAME}.nsh"
       !undef LANG
!macroend
!define UseLangTranslated "!insertmacro USE_LANG_TRANSLATED "

${UseLangTranslated} ${LANG_ENGLISH} english
${UseLangTranslated} ${LANG_SPANISH} spanish

${UseLangDefault} ${LANG_FRENCH} french.xml
${UseLangDefault} ${LANG_TRADCHINESE} "chinese.xml"
${UseLangDefault} ${LANG_SIMPCHINESE} "chineseSimplified.xml"
${UseLangDefault} ${LANG_KOREAN} "korean.xml"
${UseLangDefault} ${LANG_JAPANESE} "japanese.xml"
${UseLangDefault} ${LANG_GERMAN} "german.xml"
${UseLangDefault} ${LANG_ITALIAN} "italian.xml"
${UseLangDefault} ${LANG_PORTUGUESE} "portuguese.xml"
${UseLangDefault} ${LANG_PORTUGUESEBR} "brazilian_portuguese.xml"
${UseLangDefault} ${LANG_DUTCH} "dutch.xml"
${UseLangDefault} ${LANG_RUSSIAN} "russian.xml"
${UseLangDefault} ${LANG_POLISH} "polish.xml"
${UseLangDefault} ${LANG_CATALAN} "catalan.xml"
${UseLangDefault} ${LANG_CZECH} "czech.xml"
${UseLangDefault} ${LANG_HUNGARIAN} "hungarian.xml"
${UseLangDefault} ${LANG_ROMANIAN} "romanian.xml"
${UseLangDefault} ${LANG_TURKISH} "turkish.xml"
${UseLangDefault} ${LANG_FARSI} "farsi.xml"
${UseLangDefault} ${LANG_UKRAINIAN} "ukrainian.xml"
${UseLangDefault} ${LANG_HEBREW} "hebrew.xml"
${UseLangDefault} ${LANG_HINDI} "hindi.xml"
${UseLangDefault} ${LANG_NORWEGIANNYNORSK} "nynorsk.xml"
${UseLangDefault} ${LANG_NORWEGIAN} "norwegian.xml"
${UseLangDefault} ${LANG_THAI} "thai.xml"
${UseLangDefault} ${LANG_ARABIC} "arabic.xml"
${UseLangDefault} ${LANG_FINNISH} "finnish.xml"
${UseLangDefault} ${LANG_LITHUANIAN} "lithuanian.xml"
${UseLangDefault} ${LANG_GREEK} "greek.xml"
${UseLangDefault} ${LANG_SWEDISH} "swedish.xml"
${UseLangDefault} ${LANG_GALICIAN} "galician.xml"
${UseLangDefault} ${LANG_SLOVENIAN} "slovenian.xml"
${UseLangDefault} ${LANG_SLOVAK} "slovak.xml"
${UseLangDefault} ${LANG_DANISH} "danish.xml"
${UseLangDefault} ${LANG_BULGARIAN} "bulgarian.xml"
${UseLangDefault} ${LANG_INDONESIAN} "indonesian.xml"
${UseLangDefault} ${LANG_ALBANIAN} "albanian.xml"
${UseLangDefault} ${LANG_CROATIAN} "croatian.xml"
${UseLangDefault} ${LANG_BASQUE} "basque.xml"
${UseLangDefault} ${LANG_BELARUSIAN} "belarusian.xml"
${UseLangDefault} ${LANG_SERBIAN} "serbian.xml"
${UseLangDefault} ${LANG_MALAY} "malay.xml"
${UseLangDefault} ${LANG_LUXEMBOURGISH} "luxembourgish.xml"
${UseLangDefault} ${LANG_AFRIKAANS} "afrikaans.xml"
${UseLangDefault} ${LANG_UZBEK} "uzbek.xml"
${UseLangDefault} ${LANG_MACEDONIAN} "macedonian.xml"
${UseLangDefault} ${LANG_LATVIAN} "Latvian.xml"
${UseLangDefault} ${LANG_BOSNIAN} "bosnian.xml"
${UseLangDefault} ${LANG_MONGOLIAN} "mongolian.xml"
${UseLangDefault} ${LANG_ESTONIAN} "estonian.xml"
${UseLangDefault} ${LANG_CORSICAN} "corsican.xml"
${UseLangDefault} ${LANG_BRETON} "breton.xml"
${UseLangDefault} ${LANG_GEORGIAN} "georgian.xml"
${UseLangDefault} ${LANG_VIETNAMESE} "vietnamese.xml"
${UseLangDefault} ${LANG_WELSH} "welsh.xml"
${UseLangDefault} ${LANG_KURDISH} "kurdish.xml"
