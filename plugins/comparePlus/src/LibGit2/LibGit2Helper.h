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

#pragma once

#include <memory>
#include <string>


typedef struct git_repository git_repository;
typedef struct git_index git_index;
typedef struct git_blob git_blob;


typedef struct {
	int32_t seconds;
	uint32_t nanoseconds;
} git_index_time;


typedef struct git_oid {
	unsigned char id[20];
} git_oid;


typedef struct git_index_entry {
	git_index_time ctime;
	git_index_time mtime;

	uint32_t dev;
	uint32_t ino;
	uint32_t mode;
	uint32_t uid;
	uint32_t gid;
	uint32_t file_size;

	git_oid id;

	uint16_t flags;
	uint16_t flags_extended;

	const char *path;
} git_index_entry;


typedef struct {
	char   *ptr;
	size_t asize, size;
} git_buf;


/**
 *  \class
 *  \brief
 */
class LibGit
{
private:
	static std::unique_ptr<LibGit>	Inst;

	LibGit() {}

	typedef int (*PGITLIBVERSION) (int *major, int *minor, int *rev);
	typedef int (*PGITLIBINIT) (void);
	typedef int (*PGITLIBSHUTDOWN) (void);
	typedef int (*PGITREPOSITORYOPENEXT) (git_repository **out, const char *path,
			unsigned int flags, const char *ceiling_dirs);
	typedef const char* (*PGITREPOSITORYWORKDIR) (git_repository *repo);
	typedef int (*PGITREPOSITORYINDEX) (git_index **out, git_repository *repo);
	typedef const git_index_entry* (*PGITINDEXGETBYPATH) (git_index *index, const char *path, int stage);
	typedef int (*PGITBLOBLOOKUP) (git_blob **blob, git_repository *repo, const git_oid *id);
	typedef int (*PGITBLOBFILTERCONTENT) (git_buf *out, git_blob *blob, const char *as_path, int check_for_bin_data);
	typedef void (*PGITBUFFREE) (git_buf *buf);
	typedef void (*PGITBLOBFREE) (const git_blob *blob);
	typedef void (*PGITINDEXFREE) (git_index *index);
	typedef void (*PGITREPOSITORYFREE) (git_repository *repo);

	PGITLIBVERSION			version;
	PGITLIBINIT				init;
	PGITLIBSHUTDOWN			shutdown;

	bool	_isInit;

	std::string		_verStr;

public:
	static std::unique_ptr<LibGit>& load();

	~LibGit()
	{
		if (_isInit)
			shutdown();
	}

	const std::string& GetVersion()
	{
		return _verStr;
	}

	PGITREPOSITORYOPENEXT	repository_open_ext;
	PGITREPOSITORYWORKDIR	repository_workdir;
	PGITREPOSITORYINDEX		repository_index;
	PGITINDEXGETBYPATH		index_get_bypath;
	PGITBLOBLOOKUP			blob_lookup;
	PGITBLOBFILTERCONTENT	blob_filtered_content;
	PGITBUFFREE				buf_free;
	PGITBLOBFREE			blob_free;
	PGITINDEXFREE			index_free;
	PGITREPOSITORYFREE		repository_free;
};
