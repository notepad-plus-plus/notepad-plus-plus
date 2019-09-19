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


#include "FileInterface.h"


CFile::CFile(const char *fname, Mode fmode) : _hMode(fmode)
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


CFile::CFile(const wchar_t *fname, Mode fmode) : _hMode(fmode)
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


void CFile::Close()
{
	if (IsOpened())
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


int_fast64_t CFile::GetSize()
{
	LARGE_INTEGER r;
	r.QuadPart = -1;

	if (IsOpened())
		::GetFileSizeEx(_hFile, &r);

	return static_cast<int_fast64_t>(r.QuadPart);
}


long CFile::Read(void *rbuf, long buf_size)
{
	if (!IsOpened() || (rbuf == nullptr) || (buf_size <= 0))
		return 0;

	DWORD bytes_read = 0;

	if (::ReadFile(_hFile, rbuf, static_cast<DWORD>(buf_size), &bytes_read, NULL) == FALSE)
		return -1;

	return static_cast<long>(bytes_read);
}


bool CFile::Write(const void *wbuf, long buf_size)
{
	if (!IsOpened() || (wbuf == nullptr) || (buf_size <= 0) || ((_hMode != Mode::WRITE) && (_hMode != Mode::APPEND)))
		return false;

	DWORD bytes_written = 0;

	if (::WriteFile(_hFile, wbuf, static_cast<DWORD>(buf_size), &bytes_written, NULL) == FALSE)
		return false;

	if (!_written && (bytes_written != 0))
		_written = true;

	return (static_cast<long>(bytes_written) == buf_size);
}


// Helper function to auto-fill CreateFile params optimized for Notepad++ usage.
void CFile::fillCreateParams(DWORD &access, DWORD &share, DWORD &disp, DWORD &attrib)
{
	access	= GENERIC_READ;
	attrib	= FILE_ATTRIBUTE_NORMAL | FILE_FLAG_POSIX_SEMANTICS; // Distinguish between upper/lower case in name

	if (_hMode == Mode::READ)
	{
		share	=	FILE_SHARE_READ;
		disp	=	OPEN_EXISTING; // Open only if file exists and is not locked by other process

		attrib	|=	FILE_FLAG_SEQUENTIAL_SCAN; // Optimize caching for sequential read
	}
	else
	{
		share	=	0;
		disp	=	OPEN_ALWAYS; // Open existing file for writing without destroying it or create new

		access	|=	GENERIC_WRITE;
		attrib	|=	FILE_FLAG_WRITE_THROUGH; // Write cached data directly to disk (no lazy writer)
	}
}
