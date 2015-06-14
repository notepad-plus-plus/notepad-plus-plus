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


#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "Common.h"
#include <set>

// directory class for project panel.

class Directory {
protected:
	generic_string _path;
	bool _hideEmptyDirs;
	bool _wasRead;

	struct comparator {
		bool operator() (const generic_string& lhs, const generic_string& rhs) const {
			return lstrcmpi(lhs.c_str(),rhs.c_str()) < 0;
		}
	};
	std::set<generic_string,comparator> _dirs;
	std::set<generic_string,comparator> _files;
	std::vector<generic_string> _invisibleDirs;
	
	std::vector<generic_string> _filters;

	bool _exists;

	FILETIME _lastWriteTime;
public:
	// default directory - does not exist by default
	Directory(bool hideEmptyDirs = true);
	// directory with path - is instantly read in ctor, if autoread==true
	Directory(const generic_string& path, const std::vector<generic_string>& filters = std::vector<generic_string>(), bool hideEmptyDirs=true, bool autoread=true );

	virtual ~Directory() {}

	const generic_string& getPath() const { return _path; }

	// read the directory. Returns true, if the directory exists.
	bool read(const generic_string& path, const std::vector<generic_string>& filters);
	bool read(const generic_string& path) { return read(path,_filters); }
	bool read() { return read(_path, _filters); }

	// re-read the current directory, but only if it was changed.
	// returns true, if directory was re-read, false if directory has not changed.
	bool readIfChanged(bool respectEmptyDirs = true);

	// false when not initialized.
	bool exists() const { return _exists; }

	const std::set<generic_string,comparator>& getDirs() const { return _dirs; }
	const std::set<generic_string,comparator>& getFiles() const { return _files; }

	bool operator== (const Directory& other) const;
	bool operator!= (const Directory& other) const;

	bool empty() const { return _dirs.empty() && _files.empty(); }

	bool writeTimeHasChanged() const;

	void setFilters(const std::vector<generic_string>& filters = std::vector<generic_string>(), bool autoread=true);
	const std::vector<generic_string>& getFilters() const { return _filters; }

	// empty directories can be hidden:
	// "empty" means in this case, neither the directory itself nor its subdirectories contains any data, which match the filters
	bool getHideEmptyDirs() const { return _hideEmptyDirs; }
	void setHideEmptyDirs(bool hideEmptyDirs, bool autoread=true);

	// synchronizeTo is basically like a copy constructor, with the difference, that it calls the
	// following virtual functions onDirAdded(), ...
	// Using this makes only sense, if you have subclassed this and evaluate those virtual methods; otherwise just use the copy ctor/assigment operator.
	// Note that the virtual methods are called BEFORE the contents of this directory are really changed.
	void synchronizeTo(const Directory& other);

protected:

	virtual void onBeginSynchronize(const Directory&) {}
	virtual void onDirAdded(const generic_string&) {}
	virtual void onDirRemoved(const generic_string&) {}
	virtual void onFileAdded(const generic_string&) {}
	virtual void onFileRemoved(const generic_string&) {}
	virtual void onEndSynchronize(const Directory&) {}

private:

	static void enablePrivileges();
	static bool enablePrivilege(LPCTSTR privName);
	void append(const generic_string& path, const generic_string& filter, bool readDirs);
	bool containsDataChanged() const;
	bool containsData(const generic_string& path) const;
	bool containsData(const generic_string& path, const generic_string& filter) const;
	bool readLastWriteTime(FILETIME& filetime) const;


};


#endif