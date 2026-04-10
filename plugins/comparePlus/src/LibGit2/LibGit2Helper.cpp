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
#include <windows.h>
#include <wchar.h>
#include <shlwapi.h>

#include "LibGit2Helper.h"
#include "NppHelpers.h"
#include "Tools.h"


namespace
{

inline std::wstring getDllPath()
{
	std::wstring dll = getPluginsHomePath();
	if (dll.empty())
		return dll;

	dll += L"\\ComparePlus\\libs\\git2.dll";

	return dll;
}

}


bool isGITlibFound()
{
	const std::wstring dllPath = getDllPath();

	if (dllPath.size())
		return fileExists(dllPath.c_str());

	return false;
}


std::unique_ptr<LibGit>	LibGit::Inst;


std::unique_ptr<LibGit>& LibGit::load()
{
	if (Inst)
		return Inst;

	const std::wstring dllPath = getDllPath();

	if (dllPath.empty())
		return Inst;

	HMODULE libGit2 = ::LoadLibraryW(dllPath.c_str());
	if (!libGit2)
		return Inst;

	Inst.reset(new LibGit);

	Inst->_isInit = true;

	Inst->version = (PGITLIBVERSION)::GetProcAddress(libGit2, "git_libgit2_version");
	if (!Inst->version)
		Inst->_isInit = false;
	Inst->repository_open_ext = (PGITREPOSITORYOPENEXT)::GetProcAddress(libGit2, "git_repository_open_ext");
	if (!Inst->repository_open_ext)
		Inst->_isInit = false;
	Inst->repository_workdir = (PGITREPOSITORYWORKDIR)::GetProcAddress(libGit2, "git_repository_workdir");
	if (!Inst->repository_workdir)
		Inst->_isInit = false;
	Inst->repository_index = (PGITREPOSITORYINDEX)::GetProcAddress(libGit2, "git_repository_index");
	if (!Inst->repository_index)
		Inst->_isInit = false;
	Inst->index_get_bypath = (PGITINDEXGETBYPATH)::GetProcAddress(libGit2, "git_index_get_bypath");
	if (!Inst->index_get_bypath)
		Inst->_isInit = false;
	Inst->blob_lookup = (PGITBLOBLOOKUP)::GetProcAddress(libGit2, "git_blob_lookup");
	if (!Inst->blob_lookup)
		Inst->_isInit = false;
	Inst->blob_filtered_content = (PGITBLOBFILTERCONTENT)::GetProcAddress(libGit2, "git_blob_filtered_content");
	if (!Inst->blob_filtered_content)
		Inst->_isInit = false;
	Inst->buf_free = (PGITBUFFREE)::GetProcAddress(libGit2, "git_buf_free");
	if (!Inst->buf_free)
		Inst->_isInit = false;
	Inst->blob_free = (PGITBLOBFREE)::GetProcAddress(libGit2, "git_blob_free");
	if (!Inst->blob_free)
		Inst->_isInit = false;
	Inst->index_free = (PGITINDEXFREE)::GetProcAddress(libGit2, "git_index_free");
	if (!Inst->index_free)
		Inst->_isInit = false;
	Inst->repository_free = (PGITREPOSITORYFREE)::GetProcAddress(libGit2, "git_repository_free");
	if (!Inst->repository_free)
		Inst->_isInit = false;

	if (Inst->_isInit)
	{
		int major, minor, rev;
		Inst->version(&major, &minor, &rev);

		Inst->_verStr = std::to_string(major);
		Inst->_verStr += '.';
		Inst->_verStr += std::to_string(minor);
		Inst->_verStr += '.';
		Inst->_verStr += std::to_string(rev);

		if ((major > 0) || (major == 0 && minor >= 22)) // Those API functions are introduced after version 0.22
		{
			Inst->init = (PGITLIBINIT)::GetProcAddress(libGit2, "git_libgit2_init");
			if (!Inst->init)
				Inst->_isInit = false;
			Inst->shutdown = (PGITLIBSHUTDOWN)::GetProcAddress(libGit2, "git_libgit2_shutdown");
			if (!Inst->shutdown)
				Inst->_isInit = false;

			if (Inst->_isInit)
				Inst->init();
		}
	}

	if (!Inst->_isInit)
		Inst.reset();

	return Inst;
}
