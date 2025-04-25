// This file is part of Notepad++ project
// Copyright (C)2025 Randall Joseph Fellmy <software@coises.com>

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

#include <string>
#include <vector>
#include <../ScintillaComponent/ScintillaEditView.h>

class SortLocale
{
public:
	struct Result
	{
		UINT status = 0;       // Will be 0 (successful sort), MB_ICONWARNING or MB_ICONERROR
		std::string tagName;   // The tag name for translation
		std::wstring message;  // A message describing the status, if it isn't 0
	};
	std::wstring localeName;
	bool caseSensitive = false;
	bool digitsAsNumbers = true;
	bool ignoreDiacritics = false;
	bool ignoreSymbols = false;
	Result sort(ScintillaEditView* sci, bool descending) const;
};