#ifndef LASTRECENTFILELIST_H
#define LASTRECENTFILELIST_H

#include <list>
#include <string>

typedef std::list<std::string> stringList;

class LastRecentFileList
{
public :
	LastRecentFileList() : _hasSeparators(false){
		_userMax = (NppParameters::getInstance())->getNbMaxFile();
	};

	void initMenu(HMENU hMenu, int idBase, int posBase) {
		_hMenu = hMenu;
		_idBase = idBase;
		_posBase = posBase;

		for (int i = 0 ; i < sizeof(_idFreeArray) ; i++)
			_idFreeArray[i] = true;
	};


	void add(const char *fn) {
		if (_userMax == 0)
			return;

		int size = _lrfl.size();

		if (size >= _userMax)
		{
			_lrfl.erase(_lrfl.begin());
			
			int id = ::GetMenuItemID(_hMenu, 0 + _posBase);
			::RemoveMenu(_hMenu, id, MF_BYCOMMAND);
			setAvailable(id);
			size--;
		}
		_lrfl.push_back(fn);
		::InsertMenu(_hMenu, _posBase + size, MF_BYPOSITION, popFirstAvailableID() + _idBase, fn);
		
		if (!_hasSeparators)
		{
			::InsertMenu(_hMenu, _posBase + size + 1, MF_BYPOSITION, UINT(-1), 0);
			::InsertMenu(_hMenu, _posBase + size + 2, MF_BYPOSITION, IDM_OPEN_ALL_RECENT_FILE, "Open All Recent Files");
			::InsertMenu(_hMenu, _posBase + size + 3, MF_BYPOSITION, UINT(-1), 0);
			_hasSeparators = true;
		}
	};

	void remove(const char *fn) { 
		if (find2Remove(fn))
		{
			int id = 0;
			char filename[MAX_PATH];
			for (size_t i = 0 ; i < _lrfl.size() + 1 ; i++)
			{
				::GetMenuString(_hMenu, i + _posBase, filename, sizeof(filename), MF_BYPOSITION);

				if (!strcmp(fn, filename))
				{
					id = ::GetMenuItemID(_hMenu, i + _posBase);
					break;
				}
			}
			::RemoveMenu(_hMenu, id, MF_BYCOMMAND);
			setAvailable(id);
			
			int size;
			if (!(size = _lrfl.size()))
			{
				::RemoveMenu(_hMenu, _posBase + 2, MF_BYPOSITION);
				::RemoveMenu(_hMenu, _posBase + 0, MF_BYPOSITION);
				::RemoveMenu(_hMenu, IDM_OPEN_ALL_RECENT_FILE, MF_BYCOMMAND);
				_hasSeparators = false;
			}
		}
	};
	/*
	int getNbLRF() const {
		return _lrfl.size();
	};
*/
	int getMaxNbLRF() const {
		return NB_MAX_LRF_FILE;
	};

	void setUserMaxNbLRF(int size) {
		_userMax = size;
	};

	int getUserMaxNbLRF() const {
		return _userMax;
	};

	void saveLRFL() const {
		NppParameters *pNppParams = NppParameters::getInstance();
		pNppParams->writeNbHistoryFile(_userMax);

		// if user defined nb recent files smaller than the size of list,
		// we just keep the newest ones
		int decal = _lrfl.size() - _userMax;
		decal = (decal >= 0)?decal:0;
		stringList::const_iterator it = _lrfl.begin();
		for (int i = 0 ; i < decal ; i++, it++);

		for (int i = 0 ; it != _lrfl.end() && (i < _userMax) ; it++, i++)
		{
			pNppParams->writeHistory(((const std::string)*it).c_str());
		}
	};

private:
	stringList _lrfl;
	int _userMax;

	// For the menu
	HMENU _hMenu;
	int _posBase;
	int _idBase;
	bool _idFreeArray[NB_MAX_LRF_FILE];
	bool _hasSeparators;

	bool find(const char *fn) const {
		for (stringList::const_iterator it = _lrfl.begin() ; it != _lrfl.end() ; it++)
			if (*it == fn)
				return true;
		return false;
	};

	bool find2Remove(const char *fn) {
		for (stringList::iterator it = _lrfl.begin() ; it != _lrfl.end() ; it++)
		{
			if (*it == fn)
			{
				_lrfl.erase(it);
				return true;
			}
		}
		return false;
	};

	int popFirstAvailableID() {
		for (int i = 0 ; i < NB_MAX_LRF_FILE ; i++)
		{
			if (_idFreeArray[i])
			{
				_idFreeArray[i] = false;
				return i;
			}
		}
		return 0;
	};

	void setAvailable(int id) {
		int index = id - _idBase;
		_idFreeArray[index] = true;
	};
};

#endif //LASTRECENTFILELIST_H
