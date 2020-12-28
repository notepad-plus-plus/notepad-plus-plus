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

struct EncodingUnit {
   int _codePage;
   const char *_aliasList;
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

