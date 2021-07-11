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

#pragma once

#include "Common.h"	// generic_string

// Returns a first extension from the extension specification string.
// Multiple extensions are separated with ';'.
// Example: input - ".c;.cpp;.h", output - ".c"
generic_string get1stExt(const generic_string& extSpec);

// Replace file existing extension in name or add a new one.
bool replaceExt(generic_string& name, const generic_string& ext);

bool hasExt(const generic_string& name);