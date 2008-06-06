#ifndef LASTRECENTFILELIST_H
#define LASTRECENTFILELIST_H

#include <deque>
#include <string>
#include "windows.h"
#include "Parameters.h"

struct RecentItem {
	int _id;
	std::string _name;
	RecentItem(const char * name) : _name(name) {};
};

typedef std::deque<RecentItem> recentList;

class LastRecentFileList
{
public :
	LastRecentFileList() : _hasSeparators(false), _size(0), _locked(false) {
		_userMax = (NppParameters::getInstance())->getNbMaxFile();
	};

	void initMenu(HMENU hMenu, int idBase, int posBase);

	void updateMenu();

	void add(const char *fn);
	void remove(const char *fn);
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
	
	std::string & getItem(int id);	//use menu id
	std::string & getIndex(int index);	//use menu id

	void setUserMaxNbLRF(int size);

	void saveLRFL();

	void setLock(bool lock) {
		_locked = lock;
	};
private:
	recentList _lrfl;
	int _userMax;
	int _size;

	// For the menu
	HMENU _hMenu;
	int _posBase;
	int _idBase;
	bool _idFreeArray[NB_MAX_LRF_FILE];
	bool _hasSeparators;
	bool _locked;

	int find(const char *fn);

	int popFirstAvailableID();
	void setAvailable(int id);
};

#endif //LASTRECENTFILELIST_H
