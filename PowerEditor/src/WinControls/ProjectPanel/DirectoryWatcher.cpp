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

#include "DirectoryWatcher.h"

#include <assert.h>


DirectoryWatcher::DirectoryWatcher(HWND hWnd, DWORD updateFrequencyMs, bool hideEmptyDirs, DWORD checkEmptyDirsEvery, bool manualMode) 
	: _hWnd(hWnd)
	, _hThread(NULL)
	, _hRunningEvent(NULL)
	, _hStopEvent(NULL)
	, _hUpdateEvent(NULL)
	, _running(false)
	, _updateFrequencyMs(updateFrequencyMs)
	, _changeOccurred(false)
	, _hideEmptyDirs(hideEmptyDirs)
	, _checkEmptyDirsCount(0)
	, _checkEmptyDirsEvery(checkEmptyDirsEvery)
	, _manualMode(manualMode)
{
}

DirectoryWatcher::~DirectoryWatcher()
{
	Yuni::MutexLocker lock(_lock);
	stopThread();
	removeAllDirs();
}

void DirectoryWatcher::addOrChangeDir(const generic_string& path, HTREEITEM treeItem, const std::vector<generic_string>& filters)
{
	Yuni::MutexLocker lock(_lock);
	
	// remove a probably previously existing item
	// to easily change the filters.
	_dirItemsToRemove.insert(treeItem);

	// each newly added item gets a forced update message.
	_forcedUpdateToAdd.insert(treeItem);
	_dirItemsToAdd.emplace_back(new InsertStruct(path,treeItem,filters));
}

void DirectoryWatcher::removeDir(HTREEITEM treeItem)
{
	Yuni::MutexLocker lock(_lock);
	_dirItemsToRemove.insert(treeItem);
}

void DirectoryWatcher::removeAllDirs()
{
	Yuni::MutexLocker lock(_lock);
	for (auto it=_watchdirs.begin(); it != _watchdirs.end(); ++it)
		delete *it;
	_watchdirs.clear();

	_dirItems.clear();
	_dirItemReferenceCount.clear();
	_forcedUpdate.clear();

	for (auto it=_dirItemsToAdd.begin(); it != _dirItemsToAdd.end(); ++it)
		delete *it;
	_dirItemsToAdd.clear();
	_dirItemsToRemove.clear();
	_forcedUpdateToAdd.clear();
	
}

void DirectoryWatcher::setManualMode(bool val)
{

	Yuni::MutexLocker lock(_lock);
	if (_manualMode != val)
	{
		_manualMode = val;
		// wake up thread if manual mode has been switched off
		if (!_manualMode)
			update();
	}
}

void DirectoryWatcher::forceUpdate(HTREEITEM hItem)
{
	Yuni::MutexLocker lock(_lock);
	assert( _dirItems.find(hItem) != _dirItems.end());
	if( _dirItems.find(hItem) != _dirItems.end())
	{
		_forcedUpdateToAdd.insert(hItem);
		update();
	}
}

void DirectoryWatcher::forceUpdateAll()
{
	Yuni::MutexLocker lock(_lock);
	for (auto it=_dirItems.begin(); it !=_dirItems.end(); ++it)
	{
		_forcedUpdateToAdd.insert(it->first);
	}
	update();
}

void DirectoryWatcher::update()
{
	::SetEvent(_hUpdateEvent);
}

void DirectoryWatcher::startThread()
{

	try {
		Yuni::MutexLocker lock(_lock);
		if (_running)
			return;

		_hRunningEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		_hStopEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		_hUpdateEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!_hStopEvent || !_hRunningEvent || !_hUpdateEvent)
			throw std::runtime_error("Could not create DirectoryWatcher events");

		_hThread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadFunc, (LPVOID)this, 0, NULL);
		if (!_hThread)
			throw std::runtime_error("Could not create DirectoryWatcher thread");

		::WaitForSingleObject(_hRunningEvent, INFINITE);
	}
	catch (...)
	{
		if (_hRunningEvent)
			::CloseHandle(_hRunningEvent);
		if (_hStopEvent)
			::CloseHandle(_hStopEvent);
		if (_hUpdateEvent)
			::CloseHandle(_hUpdateEvent);
		throw;
	}



}

void DirectoryWatcher::stopThread()
{
	Yuni::MutexLocker lock(_lock);

	if (_running)
	{
		::SetEvent(_hStopEvent);
		::WaitForSingleObject(_hThread, INFINITE);
		::CloseHandle(_hThread);
		::CloseHandle(_hRunningEvent);
		::CloseHandle(_hStopEvent);
		_running = false;
		_hThread = NULL;
		_hRunningEvent = NULL;
		_hStopEvent = NULL;
	}

}

int DirectoryWatcher::thread()
{

	_running = true;
	::SetEvent(_hRunningEvent);

	HANDLE events[2];
	events[0] = _hStopEvent;
	events[1] = _hUpdateEvent;

	::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_LOWEST );

	for(;;)
	{
		updateDirs();
		iterateDirs();
		DWORD waitresult = WaitForMultipleObjects(2, events, FALSE, _manualMode ? INFINITE : _updateFrequencyMs);
		switch (waitresult)
		{
			case WAIT_FAILED:			// error
				return -1;
			case WAIT_OBJECT_0:			// stop event
				ResetEvent(_hStopEvent);
				return 0;
			case WAIT_OBJECT_0+1:		// update instantly
				ResetEvent(_hUpdateEvent);
				break;
			case WAIT_TIMEOUT:			// timeout - normal polling frequency
				break;
			default:					// should not happen
				assert(0);
				break;
		}

	}

}

DWORD DirectoryWatcher::threadFunc(LPVOID data)
{
	DirectoryWatcher* pw = static_cast<DirectoryWatcher*>(data);
	return (DWORD)pw->thread();

}

bool DirectoryWatcher::post(HTREEITEM item, UINT message)
{
	if (!_hWnd)
		return false;

	if (message == DIRECTORYWATCHER_UPDATE)
		_changeOccurred = true;

	LRESULT smResult = SendMessageTimeout(_hWnd, message, 0, (LPARAM)item, SMTO_ABORTIFHUNG, 10000, NULL);
	return smResult != 0;
}

// main function to iterate the directories.
void DirectoryWatcher::iterateDirs()
{

	std::map<Directory*,bool> changedMap;

	_changeOccurred = false;

	bool respectEmptyDirs=false;
	if (_hideEmptyDirs && _checkEmptyDirsEvery)
	{
		_checkEmptyDirsCount++;
		if (_checkEmptyDirsCount >= _checkEmptyDirsEvery)
		{
			_checkEmptyDirsCount = 0;
			respectEmptyDirs = true;
		}
	}


	for (auto it=_dirItems.begin(); it!=_dirItems.end(); ++it)
	{
		HTREEITEM hTreeItem = it->first;
		Directory* dir = it->second;
		bool forced = _forcedUpdate.find(hTreeItem) != _forcedUpdate.end();

		auto itAlreadyChanged = changedMap.find(dir);
		if (itAlreadyChanged != changedMap.end())
		{
			bool wasChanged = itAlreadyChanged->second;

			// if it was changed or inform is forced, inform tree item.
			if (wasChanged || forced)
				if (!post(hTreeItem))
					return;
			continue;
		}

		bool changed = dir->readIfChanged(respectEmptyDirs);

		changedMap[dir] = changed;

		if(changed || forced)
		{
			post(hTreeItem);
			_changeOccurred = true;
		}
	}
	
	_forcedUpdate.clear();

	if (_changeOccurred)
		post(NULL, DIRECTORYWATCHER_UPDATE_DONE);


}

void DirectoryWatcher::updateDirs()
{
	Yuni::MutexLocker lock(_lock);

	// first, remove all dir items
	for (auto itToRemove = _dirItemsToRemove.begin(); itToRemove != _dirItemsToRemove.end(); ++itToRemove)
	{
		const HTREEITEM& hTreeItem = *itToRemove;

		// get the directory by treeItem. 
		// Non-existing ones are silently ignored, because each item, which is added, is also deleted previously.
		auto itDir = _dirItems.find(hTreeItem);
		if (itDir == _dirItems.end())
			continue;

		Directory* dir = itDir->second;
		_dirItems.erase(hTreeItem);

		// decrease the dirItem/reference counter
		assert(_dirItemReferenceCount.find(dir) != _dirItemReferenceCount.end());

		// if this was the last reference
		if (--_dirItemReferenceCount[dir] <= 0)
		{
			// remove it from the reference count
			_dirItemReferenceCount.erase(dir);

			// get the directory watch matching to this path
			auto itWatchdir = _watchdirs.find(dir);
			assert(itWatchdir != _watchdirs.end());
			if (itWatchdir != _watchdirs.end())
			{
				// and delete it
				delete *itWatchdir;
				_watchdirs.erase(itWatchdir);
			}
		}
		
	}
	_dirItemsToRemove.clear();

	_forcedUpdate.insert(_forcedUpdateToAdd.begin(), _forcedUpdateToAdd.end());
	_forcedUpdateToAdd.clear();

	// last, enter the new items.
	for (auto itToInsert = _dirItemsToAdd.begin(); itToInsert != _dirItemsToAdd.end(); ++itToInsert)
	{
		const InsertStruct* insertStruct = *itToInsert;
	
		// try to find an already existing watch dir, which matches this request.
		Directory* currentWatchdir = NULL;

		for (auto itWatchdir=_watchdirs.begin(); itWatchdir != _watchdirs.end(); ++itWatchdir)
		{
			Directory* dir = *itWatchdir;
			if (dir->getPath() == insertStruct->_path && dir->getFilters() == insertStruct->_filters)
			{
				currentWatchdir = dir;
				break;
			}
		}

		// if this fails, create a new one.
		// It is created without directly reading it to keep locking short 
		// (This could e.g. be a large network directory, which could otherwise block the main thread for a long time)
		// It is then read in the next directory iteration.
		if (!currentWatchdir)
		{
			currentWatchdir = new Directory(insertStruct->_path, insertStruct->_filters, _hideEmptyDirs, false);
			_watchdirs.insert(currentWatchdir);
		}

		// insert the tree item pointing to the watchdir
		_dirItems[insertStruct->_hTreeItem] = currentWatchdir;

		// set the reference watch counter
		auto itRefCount = _dirItemReferenceCount.find(currentWatchdir);
		if (itRefCount == _dirItemReferenceCount.end())
			_dirItemReferenceCount[currentWatchdir] = 1;
		else
			itRefCount->second += 1;

		delete insertStruct;
	}
	_dirItemsToAdd.clear();
	

}

