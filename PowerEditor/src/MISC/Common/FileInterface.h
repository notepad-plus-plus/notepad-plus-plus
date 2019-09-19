// This file is part of Notepad++ project
// Copyright (C)2019 Don HO <don.h@free.fr>
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

#include <windows.h>
#include <tchar.h>
#include <cstdint>


class CFile
{
public:
	enum class Mode
	{
		READ,
		WRITE,
		APPEND
	};

	CFile(const char *fname, Mode fmode = Mode::READ);
	CFile(const wchar_t *fname, Mode fmode = Mode::READ);

	~CFile()
	{
		Close();
	}

	bool IsOpened()
	{
		return (_hFile != INVALID_HANDLE_VALUE);
	}

	void Close();

	int_fast64_t GetSize();

	long Read(void *rbuf, long buf_size);
	bool Write(const void *wbuf, long buf_size);


private:
	CFile(const CFile&) = delete;
	CFile& operator=(const CFile&) = delete;

	void fillCreateParams(DWORD &access, DWORD &share, DWORD &disp, DWORD &attrib);

	HANDLE	_hFile		{INVALID_HANDLE_VALUE};
	Mode	_hMode		{Mode::READ};
	bool	_written	{false};
};
