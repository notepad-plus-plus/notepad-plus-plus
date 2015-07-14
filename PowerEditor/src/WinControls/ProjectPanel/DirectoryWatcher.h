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


#ifndef DIRECTORYWATCHER_H
#define  DIRECTORYWATCHER_H

#include "Common.h"
#include <CommCtrl.h>

#include <map>
#include <set>

#include "resource.h"
#include "Directory.h"
#include "mutex.h"

#define DIRECTORYWATCHER_UPDATE			DIRECTORYWATCHER_USER
#define DIRECTORYWATCHER_UPDATE_DONE	DIRECTORYWATCHER_USER + 1

// this class monitors specific directories for changes in a separate thread.
// If a change occurs, it sends a DIRECTORYWATCHER_UPDATE message to the owner's window, with a tree item handle in lparam.
//
// It works by polling all directories in a given update frequency.
// If changes are detected, and DIRECTORYWATCHER_UPDATE have been sent during a cycle, at last a DIRECTORYWATCHER_UPDATE_DONE is sent.
//
// The main part of this class is a thread, which periodically polls an arbitrary number of directories for changes.
// I don't really like polling at all, but I tried out the two other obvious attempts, both of which had severe drawbacks:
//
// FindFirstChangeNotification/FindNextChangeNotification: Did not work very good, because a max of ~60 directories per thread
// can be handled (64 objects limit of WaitForMultipleObjects)
// To use a FindFirstChangeNotification for a whole tree is also not an option, because I wanted to allow to add root directories as well,
// and e.g. the continuing changes of C: are insane (think of temp dir, logs etc.) And besides, FindFirstChangeNotification does not tell you WHAT has changed *sigh*
//
// GetQueuedCompletionStatus/ReadDirectoryChangesW: Worked perfect in terms of being notified, BUT locked all open directories against deletion, which is definitely not what we want.
//
// The polling is at least done as efficient as possible.
// The first step is to compare the last write time of the directory.
// Only if this has changed, the directory is examined more in depth, if any files have been added or removed.
// Only if this is the case, the message is sent.
//
// Adding and removing directories works in two steps internally.
// First, the directory is inserted to a set of directories to be removed or added.
// Second, these sets are evaluated and its values are added or removed to the set of watched directories.
// This is done so to keep the locking time as short as possible; only the transfering of the directories is locked, not the directory watch iteration itself.

class DirectoryWatcher
{
	struct InsertStruct {
		HTREEITEM _hTreeItem;
		generic_string _path;
		std::vector<generic_string> _filters;

		InsertStruct(generic_string path, HTREEITEM hTreeItem, const std::vector<generic_string>& filters)
			: _path(path)
			, _hTreeItem(hTreeItem)
			, _filters(filters)
		{}
	};

	HANDLE _hThread;
	HANDLE _hRunningEvent, _hStopEvent, _hUpdateEvent;
	bool _running;
	HWND _hWnd;
	bool _hideEmptyDirs;
	DWORD _checkEmptyDirsEvery;
	DWORD _checkEmptyDirsCount;


	std::set<Directory*> _watchdirs;
	std::map<HTREEITEM, Directory*> _dirItems;
	std::map<Directory*, int> _dirItemReferenceCount;
	std::set<HTREEITEM> _forcedUpdate;

	DWORD _updateFrequencyMs;
	bool _changeOccurred;

	Yuni::Mutex _lock;

	std::vector<InsertStruct*> _dirItemsToAdd;
	std::set<HTREEITEM> _dirItemsToRemove;
	std::set<HTREEITEM> _forcedUpdateToAdd;

	bool _manualMode;

public:

	DirectoryWatcher(HWND hWnd = NULL, DWORD updateFrequencyMs = 1000, bool hideEmptyDirs = true, DWORD checkEmptyDirsEvery = 10, bool manualMode = false);
	virtual ~DirectoryWatcher();

	// startThread() must be called manually after creation. Throws std::runtime_error if fails to create events/resources (not very likely)
	void startThread();
	// stopThread() is called automatically on destruction
	void stopThread();

	void addOrChangeDir(const generic_string& path, HTREEITEM treeItem, const std::vector<generic_string>& filters = std::vector<generic_string>());
	void removeDir(HTREEITEM treeItem);
	void removeAllDirs();

	bool getManualMode() const { 
		return _manualMode; 
	}
	void setManualMode(bool val);

	HWND getWindow() const { 
		return _hWnd; 
	}

	void setWindow(HWND _val) { 
		_hWnd = _val; 
	}

	void forceUpdate(HTREEITEM hItem);
	void forceUpdateAll();
	void update();

private:
	int thread();
	static DWORD threadFunc(LPVOID data);
	bool post(HTREEITEM item, UINT message = DIRECTORYWATCHER_UPDATE);
	void iterateDirs();
	void updateDirs();

	DirectoryWatcher& operator= (const DirectoryWatcher&) = delete;
	DirectoryWatcher(const DirectoryWatcher&) = delete;

};

#endif

