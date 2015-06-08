// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
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

#include "Directory.h"
#include <Shlwapi.h>

Directory::Directory()
	: _exists(false)
{
	_lastWriteTime.dwLowDateTime = 0;
	_lastWriteTime.dwHighDateTime = 0;
	enablePrivileges();
}

Directory::Directory(const generic_string& path)
{
	read(path);
}

void Directory::read(const generic_string& path)
{
	_path = path;
	_exists = false;
	_lastWriteTime.dwLowDateTime = 0;
	_lastWriteTime.dwHighDateTime = 0;
	_files.clear();
	_dirs.clear();

	generic_string searchPath = _path + TEXT("\\*.*");

	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &fd);

	if (hFind == INVALID_HANDLE_VALUE)
		return;

	_exists = true;

	while (hFind != INVALID_HANDLE_VALUE)
	{
		const generic_string file(fd.cFileName);

		struct _WIN32_FILE_ATTRIBUTE_DATA m_attr;
		if (!GetFileAttributesExW((path + TEXT("\\") + file).c_str(), GetFileExInfoStandard, &m_attr))
		{
			if (!FindNextFile(hFind, &fd))
				break;
			continue;
		}

		if (m_attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (file == TEXT(".") || file == TEXT(".."))
			{
				if (!FindNextFile(hFind, &fd))
					break;
				continue;
			}
			_dirs.insert(file);
		}
		else
		{
			_files.insert(file);
		}

		if (!FindNextFile(hFind, &fd))
			break;
	}

	if (hFind != INVALID_HANDLE_VALUE)
		FindClose(hFind);

	readLastWriteTime(_lastWriteTime);

}

bool Directory::hasChanged() const
{
	FILETIME newFiletime;

	// if the new filetime can not be determined, return true for sure. The exact differences will be tested later.
	if (!readLastWriteTime(newFiletime))
		return true;

	// we could successfully get the filetime, but before this it was 0 - definitely a change
	if (!(_lastWriteTime.dwHighDateTime || _lastWriteTime.dwLowDateTime))
		return true;

	// compare file times
	return _lastWriteTime.dwLowDateTime != newFiletime.dwLowDateTime
		|| _lastWriteTime.dwHighDateTime != newFiletime.dwHighDateTime;

}

void Directory::synchronizeTo(const Directory& other)
{
	onBeginSynchronize(other);
	for (auto it = _dirs.begin(); it != _dirs.end(); ++it)
		if (other._dirs.find(*it) == other._dirs.end())
			onDirRemoved(*it);
	for (auto it = other._dirs.begin(); it != other._dirs.end(); ++it)
		if (_dirs.find(*it) == _dirs.end())
			onDirAdded(*it);
	for (auto it = _files.begin(); it != _files.end(); ++it)
		if (other._files.find(*it) == other._files.end())
			onFileRemoved(*it);
	for (auto it = other._files.begin(); it != other._files.end(); ++it)
		if (_files.find(*it) == _files.end())
			onFileAdded(*it);
	onEndSynchronize(other);

	*this = other;

}

bool Directory::readLastWriteTime(FILETIME& filetime) const
{
	if (_path.empty())
	{
		filetime.dwHighDateTime = 0;
		filetime.dwLowDateTime = 0;
		return true;
	}

	// root directories need an approach with a "CreateFile"
	// The reason is, that a FindFirstFile on a root directory is not allowed.
	// This approach WOULD work for all other directories too, but while the directory is opened, it is also LOCKED, (FILE_SHARE_DELETE does not work for the directory itself)
	// So, we don't use this for other directories.
	// A root directory is not often removed; so locking it for a very short period should not be a problem.
	// The worst thing which could probably happen, is that a "Safe remove hardware" is denied under very awkward circumstances.
	if (PathIsRoot(_path.c_str()))
	{
		HANDLE hDir = CreateFile(_path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (hDir != INVALID_HANDLE_VALUE)
		{
			FILETIME creationTime, lastAccessTime, lastWriteTime;
			bool fileTimeResult = GetFileTime( hDir, &creationTime, &lastAccessTime, &lastWriteTime) != 0;
			CloseHandle(hDir);
			if (fileTimeResult)
			{
				filetime = lastWriteTime;
				return true;
			}
			
		}
		return false;
	}


	generic_string searchPath(_path+TEXT("\\."));

	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &fd);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	FindClose(hFind);

	filetime = fd.ftLastWriteTime;
	return true;

}

bool Directory::operator==(const Directory& other) const
{
	if (_exists != other._exists
		|| _dirs.size() != other._dirs.size()
		|| _files.size() != other._files.size())
		return false;

	return _dirs == other._dirs && _files == other._files;
}

bool Directory::operator!=(const Directory& other) const
{
	return !operator== (other);
}


void Directory::enablePrivileges()
{
	static bool privilegesEnabled = false;
	if (privilegesEnabled)
		return;

	enablePrivilege(SE_BACKUP_NAME);
	enablePrivilege(SE_RESTORE_NAME);

	privilegesEnabled = true;
}

bool Directory::enablePrivilege(LPCTSTR privilegeName)
{

	bool result = false;
	HANDLE hToken;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		TOKEN_PRIVILEGES tokenPrivileges = { 1 };

		if (LookupPrivilegeValue(NULL, privilegeName, &tokenPrivileges.Privileges[0].Luid))
		{
			tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, sizeof(tokenPrivileges), NULL, NULL);
			result = (GetLastError() == ERROR_SUCCESS);
		}
		CloseHandle(hToken);
	}
	return(result);
}
