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


#include "FileInterface.h"


Win32_IO_File::Win32_IO_File(const char *fname, Mode fmode) : _hMode(fmode)
{
	if (fname)
	{
		DWORD access, share, disp, attrib;
		fillCreateParams(access, share, disp, attrib);

		_hFile = ::CreateFileA(fname, access, share, NULL, disp, attrib, NULL);
	}

	if ((_hFile != INVALID_HANDLE_VALUE) && (_hMode == Mode::APPEND))
	{
		LARGE_INTEGER offset;
		offset.QuadPart = 0;

		::SetFilePointerEx(_hFile, offset, NULL, FILE_END);
	}
}


Win32_IO_File::Win32_IO_File(const wchar_t *fname, Mode fmode) : _hMode(fmode)
{
	if (fname)
	{
		DWORD access, share, disp, attrib;
		fillCreateParams(access, share, disp, attrib);

		_hFile = ::CreateFileW(fname, access, share, NULL, disp, attrib, NULL);
	}

	if ((_hFile != INVALID_HANDLE_VALUE) && (_hMode == Mode::APPEND))
	{
		LARGE_INTEGER offset;
		offset.QuadPart = 0;

		::SetFilePointerEx(_hFile, offset, NULL, FILE_END);
	}
}


void Win32_IO_File::close()
{
	if (isOpened())
	{
		if (_written)
		{
			::SetEndOfFile(_hFile);
			::FlushFileBuffers(_hFile);
		}

		::CloseHandle(_hFile);

		_hFile = INVALID_HANDLE_VALUE;
	}
}


int_fast64_t Win32_IO_File::getSize()
{
	LARGE_INTEGER r;
	r.QuadPart = -1;

	if (isOpened())
		::GetFileSizeEx(_hFile, &r);

	return static_cast<int_fast64_t>(r.QuadPart);
}


unsigned long Win32_IO_File::read(void *rbuf, unsigned long buf_size)
{
	if (!isOpened() || (rbuf == nullptr) || (buf_size == 0))
		return 0;

	DWORD bytes_read = 0;

	if (::ReadFile(_hFile, rbuf, buf_size, &bytes_read, NULL) == FALSE)
		return 0;

	return bytes_read;
}

bool Win32_IO_File::write(const void *wbuf, unsigned long buf_size)
{
	if (!isOpened() || (wbuf == nullptr) || ((_hMode != Mode::WRITE) && (_hMode != Mode::APPEND)))
		return false;

	DWORD bytes_written = 0;

	if (::WriteFile(_hFile, wbuf, buf_size, &bytes_written, NULL) == FALSE)
		return false;

	if (!_written)
		_written = true;

	return (bytes_written == buf_size);
}


// Helper function to auto-fill CreateFile params optimized for Notepad++ usage.
void Win32_IO_File::fillCreateParams(DWORD &access, DWORD &share, DWORD &disp, DWORD &attrib)
{
	access	= GENERIC_READ;
	share = FILE_SHARE_READ;
	attrib	= FILE_ATTRIBUTE_NORMAL | FILE_FLAG_POSIX_SEMANTICS; // Distinguish between upper/lower case in name

	if (_hMode == Mode::READ)
	{
		disp	=	OPEN_EXISTING; // Open only if file exists and is not locked by other process

		attrib	|=	FILE_FLAG_SEQUENTIAL_SCAN; // Optimize caching for sequential read
	}
	else
	{
		disp	=	OPEN_ALWAYS; // Open existing file for writing without destroying it or create new
		share	|=	FILE_SHARE_WRITE;
		access	|=	GENERIC_WRITE;
		attrib	|=	FILE_FLAG_WRITE_THROUGH; // Write cached data directly to disk (no lazy writer)
	}
}
