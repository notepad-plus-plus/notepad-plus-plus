//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef LASTRECENTFILELIST_H
#define LASTRECENTFILELIST_H

#ifndef PARAMETERS_H
#include "Parameters.h"
#endif //PARAMETERS_H

struct RecentItem {
	int _id;
	generic_string _name;
	RecentItem(const TCHAR * name) : _name(name) {};
};

typedef std::deque<RecentItem> recentList;

class LastRecentFileList
{
public :
	LastRecentFileList() : _hasSeparators(false), _size(0), _locked(false) {
		_userMax = (NppParameters::getInstance())->getNbMaxRecentFile();
	};

	void initMenu(HMENU hMenu, int idBase, int posBase, bool doSubMenu = false);

	void updateMenu();

	void add(const TCHAR *fn);
	void remove(const TCHAR *fn);
	void remove(int index);
	void clear();

	int getSize() {
		return _size;
	};


	int getMaxNbLRF() const {
		return NB_MAX_LRF_FILE;
	};

	int getUserMaxNbLRF() const {
		return _userMax;
	};
	
	generic_string & getItem(int id);	//use menu id
	generic_string & getIndex(int index);	//use menu id

	void setUserMaxNbLRF(int size);

	void saveLRFL();

	void setLock(bool lock) {
		_locked = lock;
	};

	void setLangEncoding(int nativeLangEncoding) {
		_nativeLangEncoding = nativeLangEncoding;
	};

	bool isSubMenuMode() const {
		return (_hParentMenu != NULL);
	}

private:
	recentList _lrfl;
	int _userMax;
	int _size;
	int _nativeLangEncoding;

	// For the menu
	HMENU _hParentMenu;
	HMENU _hMenu;
	int _posBase;
	int _idBase;
	bool _idFreeArray[NB_MAX_LRF_FILE];
	bool _hasSeparators;
	bool _locked;

	int find(const TCHAR *fn);

	int popFirstAvailableID();
	void setAvailable(int id);
};

#endif //LASTRECENTFILELIST_H
