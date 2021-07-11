// This file is part of Notepad++ project
// Copyright (C) 2021 The Notepad++ Contributors.

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

#include "StringUtils.h"

bool endsWith(const generic_string& s, const generic_string& suffix)
{
#if defined(_MSVC_LANG) && (_MSVC_LANG > 201402L)
#error Replace this function with basic_string::ends_with
#endif
	size_t pos = s.find(suffix);
	return pos != s.npos && ((s.length() - pos) == suffix.length());
}
