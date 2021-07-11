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

#include "FS.h"

generic_string get1stExt(const generic_string& extSpec)
{
	size_t pos = extSpec.find('.');
	if (pos != generic_string::npos)
	{
		size_t posEnd = extSpec.find(';', pos + 1);
		if (posEnd != generic_string::npos)
		{
			size_t extLen = posEnd - pos;
			return extSpec.substr(pos, extLen);
		}
		return extSpec.substr(pos);
	}
	return {};
}

bool replaceExt(generic_string& name, const generic_string& ext)
{
	if (!name.empty() && !ext.empty())
	{
		// Remove an existing extension from the name.
		size_t posNameExt = name.find_last_of('.');
		if (posNameExt != generic_string::npos)
			name.erase(posNameExt);
		// Append a new extension.
		name += ext;
		return true;
	}
	return false;
}

bool hasExt(const generic_string& name)
{
	return name.find_last_of('.') != generic_string::npos;
}
