// This file is part of Notepad++ project
// Copyright (C)2020 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once

LocalizationSwitcher::LocalizationDefinition localizationDefs[] =
{
	{TEXT("English"), TEXT("english.xml")},
	{TEXT("English (customizable)"), TEXT("english_customizable.xml")},
	{TEXT("Français"), TEXT("french.xml")},
	{TEXT("台灣繁體"), TEXT("taiwaneseMandarin.xml")},
	{TEXT("中文简体"), TEXT("chineseSimplified.xml")},
	{TEXT("한국어"), TEXT("korean.xml")},
	{TEXT("日本語"), TEXT("japanese.xml")},
	{TEXT("Deutsch"), TEXT("german.xml")},
	{TEXT("Español"), TEXT("spanish.xml")},
	{TEXT("Italiano"), TEXT("italian.xml")},
	{TEXT("Português"), TEXT("portuguese.xml")},
	{TEXT("Português brasileiro"), TEXT("brazilian_portuguese.xml")},
	{TEXT("Nederlands"), TEXT("dutch.xml")},
	{TEXT("Русский"), TEXT("russian.xml")},
	{TEXT("Polski"), TEXT("polish.xml")},
	{TEXT("Català"), TEXT("catalan.xml")},
	{TEXT("Česky"), TEXT("czech.xml")},
	{TEXT("Magyar"), TEXT("hungarian.xml")},
	{TEXT("Română"), TEXT("romanian.xml")},
	{TEXT("Türkçe"), TEXT("turkish.xml")},
	{TEXT("فارسی"), TEXT("farsi.xml")},
	{TEXT("Українська"), TEXT("ukrainian.xml")},
	{TEXT("עברית"), TEXT("hebrew.xml")},
	{TEXT("Nynorsk"), TEXT("nynorsk.xml")},
	{TEXT("Norsk"), TEXT("norwegian.xml")},
	{TEXT("Occitan"), TEXT("occitan.xml")},
	{TEXT("ไทย"), TEXT("thai.xml")},
	{TEXT("Furlan"), TEXT("friulian.xml")},
	{TEXT("العربية"), TEXT("arabic.xml")},
	{TEXT("Suomi"), TEXT("finnish.xml")},
	{TEXT("Lietuvių"), TEXT("lithuanian.xml")},
	{TEXT("Ελληνικά"), TEXT("greek.xml")},
	{TEXT("Svenska"), TEXT("swedish.xml")},
	{TEXT("Galego"), TEXT("galician.xml")},
	{TEXT("Slovenščina"), TEXT("slovenian.xml")},
	{TEXT("Slovenčina"), TEXT("slovak.xml")},
	{TEXT("Dansk"), TEXT("danish.xml")},
	{TEXT("Estremeñu"), TEXT("extremaduran.xml")},
	{TEXT("Žemaitiu ruoda"), TEXT("samogitian.xml")},
	{TEXT("Български"), TEXT("bulgarian.xml")},
	{TEXT("Bahasa Indonesia"), TEXT("indonesian.xml")},
	{TEXT("Gjuha shqipe"), TEXT("albanian.xml")},
	{TEXT("Hrvatski jezik"), TEXT("croatian.xml")},
	{TEXT("ქართული ენა"), TEXT("georgian.xml")},
	{TEXT("Euskara"), TEXT("basque.xml")},
	{TEXT("Español argentina"), TEXT("spanish_ar.xml")},
	{TEXT("Беларуская мова"), TEXT("belarusian.xml")},
	{TEXT("Srpski"), TEXT("serbian.xml")},
	{TEXT("Cрпски"), TEXT("serbianCyrillic.xml")},
	{TEXT("Bahasa Melayu"), TEXT("malay.xml")},
	{TEXT("Lëtzebuergesch"), TEXT("luxembourgish.xml")},
	{TEXT("Tagalog"), TEXT("tagalog.xml")},
	{TEXT("Afrikaans"), TEXT("afrikaans.xml")},
	{TEXT("Қазақша"), TEXT("kazakh.xml")},
	{TEXT("O‘zbekcha"), TEXT("uzbek.xml")},
	{TEXT("Ўзбекча"), TEXT("uzbekCyrillic.xml")},
	{TEXT("Кыргыз тили"), TEXT("kyrgyz.xml")},
	{TEXT("Македонски јазик"), TEXT("macedonian.xml")},
	{TEXT("latviešu valoda"), TEXT("latvian.xml")},
	{TEXT("தமிழ்"), TEXT("tamil.xml")},
	{TEXT("Azərbaycan dili"), TEXT("azerbaijani.xml")},
	{TEXT("Bosanski"), TEXT("bosnian.xml")},
	{TEXT("Esperanto"), TEXT("esperanto.xml")},
	{TEXT("Zeneize"), TEXT("ligurian.xml")},
	{TEXT("हिन्दी"), TEXT("hindi.xml")},
	{TEXT("Sardu"), TEXT("sardinian.xml")},
	{TEXT("ئۇيغۇرچە"), TEXT("uyghur.xml")},
	{TEXT("తెలుగు"), TEXT("telugu.xml")},
	{TEXT("aragonés"), TEXT("aragonese.xml")},
	{TEXT("বাংলা"), TEXT("bengali.xml")},
	{TEXT("සිංහල"), TEXT("sinhala.xml")},
	{TEXT("Taqbaylit"), TEXT("kabyle.xml")},
	{TEXT("मराठी"), TEXT("marathi.xml")},
	{TEXT("tiếng Việt"), TEXT("vietnamese.xml")},
	{TEXT("Aranés"), TEXT("aranese.xml")},
	{TEXT("ગુજરાતી"), TEXT("gujarati.xml")},
	{TEXT("Монгол хэл"), TEXT("mongolian.xml")},
	{TEXT("اُردُو‎"), TEXT("urdu.xml")},
	{TEXT("ಕನ್ನಡ‎"), TEXT("kannada.xml")},
	{TEXT("Cymraeg"), TEXT("welsh.xml")},
	{TEXT("eesti keel"), TEXT("estonian.xml")},
	{TEXT("Тоҷик"), TEXT("tajikCyrillic.xml")},
	{TEXT("татарча"), TEXT("tatar.xml")},
	{TEXT("ਪੰਜਾਬੀ"), TEXT("punjabi.xml")},
	{TEXT("Corsu"), TEXT("corsican.xml")},
	{TEXT("Brezhoneg"), TEXT("breton.xml")},
	{TEXT("کوردی‬"), TEXT("kurdish.xml")},
	{TEXT("Pig latin"), TEXT("piglatin.xml")},
	{TEXT("Zulu"), TEXT("zulu.xml")},
	{TEXT("Vèneto"), TEXT("venetian.xml")},
	{TEXT("Gaeilge"), TEXT("irish.xml")},
	{TEXT("नेपाली"), TEXT("nepali.xml")}
};
