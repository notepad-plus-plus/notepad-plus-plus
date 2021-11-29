// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

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

struct EncodingUnit {
   int _codePage = 0;
   const char *_aliasList = nullptr;
};

class EncodingMapper {
public:
	static EncodingMapper& getInstance() {
		static  EncodingMapper  instance;
		return instance;
	}
    int getEncodingFromIndex(int index) const;
	int getIndexFromEncoding(int encoding) const;
	int getEncodingFromString(const char * encodingAlias) const;

private:
	EncodingMapper() = default;
	~EncodingMapper() = default;

	// No copy ctor and assignment
	EncodingMapper(const EncodingMapper&) = delete;
	EncodingMapper& operator=(const EncodingMapper&) = delete;

	// No move ctor and assignment
	EncodingMapper(EncodingMapper&&) = delete;
	EncodingMapper& operator=(EncodingMapper&&) = delete;
};

