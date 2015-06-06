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


DirectoryWatcher::DirectoryWatcher(HWND hWnd, DWORD updateFrequencyMs) 
	: _hWnd(hWnd)
	, _hThread(NULL)
	, _hRunningEvent(NULL)
	, _hStopEvent(NULL)
	, _hUpdateEvent(NULL)
	, _running(false)
	, _updateFrequencyMs(updateFrequencyMs)
	, _watching(true)
	, _changeOccurred(false)
{
}

DirectoryWatcher::~DirectoryWatcher()
{
	stopThread();
	removeAllDirs();
}

void DirectoryWatcher::addDir(const generic_string& path, HTREEITEM treeItem)
{
	Scopelock lock(_lock);
	_forcedUpdateToAdd.insert(treeItem);
	_dirItemsToAdd.insert(std::pair<generic_string,HTREEITEM>(path,treeItem));
}

void DirectoryWatcher::removeDir(const generic_string& path, HTREEITEM treeItem)
{
	Scopelock lock(_lock);
	_dirItemsToRemove.insert(std::pair<generic_string,HTREEITEM>(path,treeItem));
}

void DirectoryWatcher::removeAllDirs()
{
	Scopelock lock(_lock);
	for( auto it=_watchdirs.begin(); it != _watchdirs.end(); ++it )
		delete it->second;

	_watchdirs.clear();
	_dirItems.clear();
	_dirItemsToAdd.clear();
	_dirItemsToRemove.clear();

}

void DirectoryWatcher::update()
{
	::SetEvent(_hUpdateEvent);
}

void DirectoryWatcher::startThread()
{

	try {
		Scopelock lock(_lock);
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
	Scopelock lock(_lock);

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

// this is the main thread routine to monitor directory changes.
// Unfortunately, Windows afaik offers no way to monitor a non-existent directory. FindFirstChangeNotification() returns INVALID_HANDLE_VALUE in this case.
// It might be possible to monitor the next valid parent dir, but this is complicated (even drive letters may be missing).
// So we choose another way:
//
// There are two modes, an existing mode and a non-existing mode.
//
// The existing mode waits for either a directory change or the thread stop signal.
// If it finds a directory change, it informs the tree view about it.
// It then checks, if the directory has been removed completely, and if so, it switches to non-existing mode.
// If not, it continues to wait for changes.
//
// The non-existing mode waits for 2 seconds for the thread stop signal.
// If the 2 secs have passed without a stop signal, it checks the existence of the directory. If it now exists, it informs the tree view and switches to existing mode.
// If it still does not exist, it continues waiting.

int DirectoryWatcher::thread()
{

	_running = true;
	::SetEvent(_hRunningEvent);

	HANDLE events[2];
	events[0] = _hStopEvent;
	events[1] = _hUpdateEvent;


	for(;;)
	{
		updateDirs();
		if (_watching)
			iterateDirs();
		DWORD waitresult = WaitForMultipleObjects(2, events, FALSE, _updateFrequencyMs);
		switch (waitresult)
		{
			case WAIT_FAILED:			// error
				return -1;
			case WAIT_OBJECT_0:			// stop event
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
	if (message == DIRECTORYWATCHER_UPDATE)
		_changeOccurred = true;

	LRESULT smResult = SendMessageTimeout(_hWnd, message, 0, (LPARAM)item, SMTO_ABORTIFHUNG, 10000, NULL);
	return smResult != 0;
}

// main function to iterate the directories. Still optimization options.
void DirectoryWatcher::iterateDirs()
{
	std::map<generic_string,bool> changedMap;
	_changeOccurred = false;

	for (auto it=_dirItems.begin(); it!=_dirItems.end(); ++it)
	{
		const generic_string& path = it->first;
		const HTREEITEM& treeItem = it->second;

		bool forced = _forcedUpdate.find(treeItem) != _forcedUpdate.end();

		// first, look if we already checked if the current directory has been changed
		// this may happen, because dirItems is a multimap, and there might be the same directory with multiple tree nodes assigned to it
		auto itAlreadyChanged = changedMap.find(path);
		if (itAlreadyChanged != changedMap.end())
		{
			// yes, we have checked already.

			bool wasChanged = itAlreadyChanged->second;

			// if it was changed or inform is forced, inform tree item.
			if (wasChanged || forced)
				if (!post(treeItem))
					return;
			continue;
		}

		// find the matching watchdir
		auto itWatchdirs = _watchdirs.find(path);

		if (itWatchdirs == _watchdirs.end())
		{
			if (!post(treeItem))
				return;
			// there is no matching watchdir, because the dir has been freshly added. Create a new one.
			_watchdirs[path] = new Directory(path);
			// always inform, when a new directory was posted, and always set change to true.
			changedMap[path] = true;
			continue;
		}

		// fetch the old dir state
		Directory* oldDirState = itWatchdirs->second;
		// if the last write time has not changed,
		if (!oldDirState->hasChanged())
		{
			// only post if forced and continue
			if (forced)
				post(treeItem);
			continue;
		}

		// read in the current state of the directory
		Directory* newDirState = new Directory(path);
		// check if the old state and the new state are not the same
		bool changed = *oldDirState != *newDirState;
		// store this comparison for future comparisons
		changedMap[path] = changed;

		// delete the old dir state
		delete oldDirState;
		// and set the new one. Have to do this always, because at least the last write time is different.
		_watchdirs[path] = newDirState;

		if(changed || forced)
			post(treeItem);

	}

	// at last, remove all watchdirs, which are not used anymore.

	std::vector<std::map<generic_string,Directory*>::iterator> deleteCandidates;
	deleteCandidates.reserve(_watchdirs.size());

	for (auto it=_watchdirs.begin(); it !=_watchdirs.end(); ++it)
	{
		if (_dirItems.find(it->first) == _dirItems.end())
			deleteCandidates.push_back(it);
	}

	for (size_t i=0; i<deleteCandidates.size(); i++ )
	{
		delete deleteCandidates[i]->second;
		_watchdirs.erase( deleteCandidates[i] );
	}
	
	_forcedUpdate.clear();

	if (_changeOccurred)
		post(NULL, DIRECTORYWATCHER_UPDATE_DONE);


}

void DirectoryWatcher::updateDirs()
{
	Scopelock lock(_lock);

	// first, remove all dir items
	for (auto itToRemove = _dirItemsToRemove.begin(); itToRemove != _dirItemsToRemove.end(); ++itToRemove)
	{
		const generic_string& path = itToRemove->first;
		const HTREEITEM& treeItem = itToRemove->second;

		for (auto itDirItems = _dirItems.find(path); itDirItems != _dirItems.end(); ++itDirItems )
		{
			if (itDirItems->second == treeItem)
			{
				_dirItems.erase(itDirItems);
				break;
			}
		}
	}
	_dirItemsToRemove.clear();

	_forcedUpdate.insert(_forcedUpdateToAdd.begin(), _forcedUpdateToAdd.end());
	_forcedUpdateToAdd.clear();

	// last, enter the new items.
	_dirItems.insert(_dirItemsToAdd.begin(),_dirItemsToAdd.end());
	_dirItemsToAdd.clear();
		

}

