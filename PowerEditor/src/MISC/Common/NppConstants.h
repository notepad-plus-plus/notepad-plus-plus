// This file is part of Notepad++ project
// Copyright (C)2025 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

inline constexpr bool dirUp = true;
inline constexpr bool dirDown = false;

inline constexpr int NPP_CP_WIN_1252 = 1252;
inline constexpr int NPP_CP_DOS_437 = 437;
inline constexpr int NPP_CP_BIG5 = 950;

#define LINKTRIGGERED WM_USER+555

#define BCKGRD_COLOR (RGB(255,102,102))
#define TXT_COLOR    (RGB(255,255,255))

#ifndef __MINGW32__
#define WCSTOK wcstok
#else
#define WCSTOK wcstok_s
#endif


#define NPP_INTERNAL_FUNCTION_STR L"Notepad++::InternalFunction"

#define REBARBAND_SIZE sizeof(REBARBANDINFO)

#define IDT_HIDE_TOOLTIP 1001

#define NPP_UAC_SAVE_SIGN L"#UAC-SAVE#"
#define NPP_UAC_SETFILEATTRIBUTES_SIGN L"#UAC-SETFILEATTRIBUTES#"
#define NPP_UAC_MOVEFILE_SIGN L"#UAC-MOVEFILE#"
#define NPP_UAC_CREATEEMPTYFILE_SIGN L"#UAC-CREATEEMPTYFILE#"

enum class SubclassID : unsigned int
{
	darkMode = 42,
	first = 666,
	second = 1984
};
