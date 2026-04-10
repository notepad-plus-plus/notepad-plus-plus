/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2013 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
 * Copyright (C)2017-2025 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <shlwapi.h>
#include <cstring>

#include "Compare.h"
#include "LibHelpers.h"
#include "SQLite/SqliteHelper.h"
#include "LibGit2/LibGit2Helper.h"
#include "Tools.h"
#include "Strings.h"


namespace // anonymous namespace
{

void WCharToChar(const wchar_t* src, char* dest, int destCharsCount)
{
	::WideCharToMultiByte(CP_UTF8, 0, src, -1, dest, destCharsCount, NULL, NULL);
}


void RelativePath(const char* fullFilePath, const char* baseDir, char* filePath, unsigned filePathSize)
{
	filePath[0] = 0;

	char fullPath[MAX_PATH];

	strcpy_s(fullPath, _countof(fullPath), fullFilePath);
	for (int i = static_cast<int>(strlen(fullPath) - 1); i >= 0; --i)
	{
		if (fullPath[i] == '\\')
			fullPath[i] = '/';
	}

	char basePath[MAX_PATH];

	strcpy_s(basePath, sizeof(basePath), baseDir);
	for (int i = static_cast<int>(strlen(basePath) - 1); i >= 0; --i)
	{
		if (basePath[i] == '\\')
			basePath[i] = '/';
	}

	int relativePathPos = static_cast<int>(strlen(basePath));

	if (!strncmp(fullPath, basePath, relativePathPos))
	{
		if (fullPath[relativePathPos] == '/')
			++relativePathPos;

		strcpy_s(filePath, filePathSize, &fullPath[relativePathPos]);
	}
}


void RelativePath(const wchar_t* fullFilePath, const wchar_t* baseDir, wchar_t* filePath, unsigned filePathSize)
{
	filePath[0] = 0;

	wchar_t fullPath[MAX_PATH];

	wcscpy_s(fullPath, _countof(fullPath), fullFilePath);
	for (int i = static_cast<int>(wcslen(fullPath) - 1); i >= 0; --i)
	{
		if (fullPath[i] == L'\\')
			fullPath[i] = L'/';
	}

	wchar_t basePath[MAX_PATH];

	wcscpy_s(basePath, _countof(basePath), baseDir);
	for (int i = static_cast<int>(wcslen(basePath) - 1); i >= 0; --i)
	{
		if (basePath[i] == L'\\')
			basePath[i] = L'/';
	}

	int relativePathPos = static_cast<int>(wcslen(basePath));

	if (!wcsncmp(fullPath, basePath, relativePathPos))
	{
		if (fullPath[relativePathPos] == L'/')
			++relativePathPos;

		wcscpy_s(filePath, filePathSize, &fullPath[relativePathPos]);
	}
}


// Search recursively upwards for the dirName folder
bool LocateDirUp(const wchar_t* dirName, const wchar_t* currentDir, wchar_t* fullDirPath, unsigned fullDirPathSize)
{
	wchar_t testPath[MAX_PATH];

	wcscpy_s(fullDirPath, fullDirPathSize, currentDir);

	while (true)
	{
		::PathCombineW(testPath, fullDirPath, dirName);
		if (::PathIsDirectoryW(testPath))
			return true;

		if (::PathIsRootW(fullDirPath))
			break;

		// up one folder
		::PathCombineW(testPath, fullDirPath, L"..");
		wcscpy_s(fullDirPath, fullDirPathSize, testPath);
	}

	return false;
}

} // anonymous namespace


bool GetSvnFile(const wchar_t* fullFilePath, wchar_t* svnFile, unsigned svnFileSize)
{
	wchar_t svnTop[MAX_PATH];
	wchar_t svnBase[MAX_PATH];

	wcscpy_s(svnBase, _countof(svnBase), fullFilePath);
	::PathRemoveFileSpecW(svnBase);

	bool ret = LocateDirUp(L".svn", svnBase, svnTop, _countof(svnTop));

	if (ret)
	{
		ret = false;

		wchar_t dotSvnIdx[MAX_PATH];

		::PathCombineW(dotSvnIdx, svnTop, L".svn");
		::PathCombineW(svnBase, dotSvnIdx, L"wc.db");

		// is it SVN 1.7 or above?
		if (::PathFileExistsW(svnBase))
		{
			if (!InitSQLite())
			{
				::MessageBoxW(nppData._nppHandle, Strings::get()["SQLITE_FAIL"].c_str(), PLUGIN_NAME, MB_OK);
				return false;
			}

			sqlite3* ppDb;

			if (sqlite3_open16(svnBase, &ppDb) == SQLITE_OK)
			{
				RelativePath(fullFilePath, svnTop, svnBase, _countof(svnBase));

				wchar_t sqlQuery[MAX_PATH + 64];
				_snwprintf_s(sqlQuery, _countof(sqlQuery), _TRUNCATE,
						L"SELECT checksum FROM nodes_current WHERE local_relpath='%s';", svnBase);

				sqlite3_stmt* pStmt;

				if (sqlite3_prepare16_v2(ppDb, sqlQuery, -1, &pStmt, NULL) == SQLITE_OK)
				{
					if (sqlite3_step(pStmt) == SQLITE_ROW)
					{
						const wchar_t* checksum = (const wchar_t*)sqlite3_column_text16(pStmt, 0);

						if (checksum[0] != 0)
						{
							wchar_t idx[128];

							wcsncpy_s(idx, _countof(idx), checksum + 6, 2);

							::PathCombineW(svnBase, dotSvnIdx, L"pristine");
							::PathCombineW(dotSvnIdx, svnBase, idx);

							wcscpy_s(idx, _countof(idx), checksum + 6);

							::PathCombineW(svnBase, dotSvnIdx, idx);
							wcscat_s(svnBase, _countof(svnBase), L".svn-base");

							if (PathFileExistsW(svnBase))
							{
								wcscpy_s(svnFile, svnFileSize, svnBase);
								ret = true;
							}
						}
					}

					sqlite3_finalize(pStmt);
				}

				sqlite3_close(ppDb);
			}
		}
		else
		{
			::PathCombineW(svnTop, dotSvnIdx, L"text-base");

			const wchar_t* file = ::PathFindFileNameW(fullFilePath);

			::PathCombineW(svnBase, svnTop, file);
			wcscat_s(svnBase, _countof(svnBase), L".svn-base");

			// Is it an old SVN version?
			if (::PathFileExistsW(svnBase))
			{
				wcscpy_s(svnFile, svnFileSize, svnBase);
				ret = true;
			}
		}
	}

	if (!ret)
		::MessageBoxW(nppData._nppHandle, Strings::get()["NO_SVN"].c_str(), PLUGIN_NAME, MB_OK);

	return ret;
}


std::vector<char> GetGitFileContent(const wchar_t* fullFilePath)
{
	std::vector<char> gitFileContent;

	std::unique_ptr<LibGit>& gitLib = LibGit::load();
	if (!gitLib)
	{
		::MessageBoxW(nppData._nppHandle, Strings::get()["LIBGIT_FAIL"].c_str(), PLUGIN_NAME, MB_OK);
		return gitFileContent;
	}

	git_repository* repo = NULL;

	char ansiGitFilePath[MAX_PATH];

	{
		char ansiPath[MAX_PATH * 2];

		WCharToChar(fullFilePath, ansiPath, sizeof(ansiPath));
		::PathRemoveFileSpecA(ansiPath);

		if (!gitLib->repository_open_ext(&repo, ansiPath, 0, NULL))
		{
			const char* ansiGitDir = gitLib->repository_workdir(repo);

			//reinit with fullFilePath after modification by PathRemoveFileSpecA(), needed to get the relative path
			WCharToChar(fullFilePath, ansiPath, sizeof(ansiPath));

			RelativePath(ansiPath, ansiGitDir, ansiGitFilePath, sizeof(ansiGitFilePath));
		}
	}

	if (repo)
	{
		git_index* index;

		if (!gitLib->repository_index(&index, repo))
		{
			const git_index_entry* e = gitLib->index_get_bypath(index, ansiGitFilePath, 0);

			if (e)
			{
				git_blob* blob;

				if (!gitLib->blob_lookup(&blob, repo, &e->id))
				{
					git_buf gitBuf = { 0 };

					if (!gitLib->blob_filtered_content(&gitBuf, blob, ansiGitFilePath, 1))
					{
						gitFileContent.resize(gitBuf.size + 1, 0);
						std::memcpy(gitFileContent.data(), gitBuf.ptr, gitBuf.size);

						gitLib->buf_free(&gitBuf);
					}

					gitLib->blob_free(blob);
				}
			}

			gitLib->index_free(index);
		}

		gitLib->repository_free(repo);
	}

	if (gitFileContent.empty())
		::MessageBoxW(nppData._nppHandle, Strings::get()["NO_GIT"].c_str(), PLUGIN_NAME, MB_OK);

	return gitFileContent;
}


std::wstring GetLibGit2Ver()
{
	std::unique_ptr<LibGit>& gitLib = LibGit::load();
	if (!gitLib)
		return {};

	const std::string& libGit2Ver = gitLib->GetVersion();

	return MBtoWC(libGit2Ver.c_str(), static_cast<int>(libGit2Ver.size()));
}


std::wstring GetSQLite3Ver()
{
	if (!InitSQLite())
		return {};

	const char* sqlite3Ver = sqlite3_libversion();

	return MBtoWC(sqlite3Ver, static_cast<int>(strlen(sqlite3Ver)));
}
