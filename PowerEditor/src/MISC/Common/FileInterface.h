// This file is part of Notepad++ project
// Copyright (C)2021 Pavel Nedev (pg.nedev@gmail.com)

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

#include <windows.h>
#include <string>
#include <tchar.h>
#include <cstdint>


class Win32_IO_File final
{
public:
	Win32_IO_File(const char *fname);
	Win32_IO_File(const wchar_t *fname);

	Win32_IO_File() = delete;
	Win32_IO_File(const Win32_IO_File&) = delete;
	Win32_IO_File& operator=(const Win32_IO_File&) = delete;

	~Win32_IO_File() {
		close();
	};

	bool isOpened() {
		return (_hFile != INVALID_HANDLE_VALUE);
	};

	void close();
	//int_fast64_t getSize();
	//unsigned long read(void *rbuf, unsigned long buf_size);

	bool write(const void *wbuf, unsigned long buf_size);

	bool writeStr(const std::string& str) {
		return write(str.c_str(), static_cast<unsigned long>(str.length()));
	};

private:
	HANDLE	_hFile		{INVALID_HANDLE_VALUE};
	bool	_written	{false};
	std::string _path;

	const DWORD _accessParam  { GENERIC_READ | GENERIC_WRITE };
	const DWORD _shareParam   { FILE_SHARE_READ | FILE_SHARE_WRITE };
	const DWORD _dispParam    { CREATE_ALWAYS };
	const DWORD _attribParam  { FILE_ATTRIBUTE_NORMAL };
};
