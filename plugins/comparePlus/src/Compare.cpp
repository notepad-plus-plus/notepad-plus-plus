/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2011 Jean-Sebastien Leroy (jean.sebastien.leroy@gmail.com)
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

#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include <memory>
#include <utility>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cwchar>

#if defined(__MINGW32__) && !defined(_GLIBCXX_HAS_GTHREADS)
#include "../mingw-std-threads/mingw.thread.h"
#else
#include <thread>
#endif // __MINGW32__ ...

#include <windows.h>
#include <wchar.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <commdlg.h>

#include "Tools.h"
#include "Strings.h"
#include "Compare.h"
#include "NppHelpers.h"
#include "LibHelpers.h"
#include "AboutDialog.h"
#include "SettingsDialog.h"
#include "CompareOptionsDialog.h"
#include "VisualFiltersDialog.h"
#include "NavDialog.h"
#include "Engine.h"
#include "resource.h"


#ifndef NDEBUG

#pragma message("Compiling in debug mode.")

#endif // NDEBUG

#ifdef DLOG

#pragma message("Compiling with debug log messages.")

#endif // DLOG


const wchar_t PLUGIN_NAME[] = L"ComparePlus";

NppData			nppData;
SciFnDirect		sciFunc;
sptr_t			sciPtr[2];

UserSettings	Settings;

int gMarginWidth = 0;

#ifdef DLOG

std::string		dLog("ComparePlus debug log\n\n");
DWORD			dLogTime_ms = 0;
static LRESULT	dLogBuf = -1;

#endif


namespace // anonymous namespace
{

constexpr int MIN_NOTEPADPP_VERSION_MAJOR = 8;
constexpr int MIN_NOTEPADPP_VERSION_MINOR = 420;

constexpr int MIN_NOTEPADPP_VERSION = ((MIN_NOTEPADPP_VERSION_MAJOR << 16) | MIN_NOTEPADPP_VERSION_MINOR);

constexpr intptr_t SCN_MODIFIED_NOTIF_FLAGS_USED =
	SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_MOD_BEFOREDELETE |
	SC_PERFORMED_USER | SC_PERFORMED_UNDO | SC_PERFORMED_REDO;


/**
 *  \class
 *  \brief
 */
class NppState
{
public:
	static NppState& get()
	{
		static NppState instance;
		return instance;
	}

	void enableClearCommands(bool enable) const;
	void enableNppScrollCommands(bool enable) const;

	void updateLocalization();
	void updateMenuAndToolbar();

	void setNormalMode(bool forceUpdate = false);
	void setCompareMode(bool clearHorizontalScroll = false);

	void setMainZoom(int zoom)
	{
		_mainZoom = zoom;
	}

	void setSubZoom(int zoom)
	{
		_subZoom = zoom;
	}

	void setCompareZoom(int zoom)
	{
		_compareZoom = zoom;
	}

	bool compareMode;

private:
	NppState() : compareMode(false), _restoreMultilineTab(false), _mainZoom(0), _subZoom(0), _compareZoom(0) {}

	void save();
	void toSingleLineTab();
	void restoreMultilineTab();
	void refreshTabBar(HWND hTabBar);
	void refreshTabBars();

	bool		_restoreMultilineTab;

	bool		_syncVScroll;
	bool		_syncHScroll;

	int			_lineNumMode;

	int			_mainZoom;
	int			_subZoom;
	int			_compareZoom;
};


/**
 *  \struct
 *  \brief
 */
struct DeletedSection
{
	/**
	 *  \struct
	 *  \brief
	 */
	struct UndoData
	{
		AlignmentInfo_t					alignment;
		std::pair<intptr_t, intptr_t>	selection {-1, -1};
		std::vector<int>				otherViewMarks;
	};

	DeletedSection(int action, intptr_t line, const std::shared_ptr<UndoData>& undo) :
			startLine(line), lineReplace(false), nextLineMarker(0), undoInfo(undo)
	{
		restoreAction = (action == SC_PERFORMED_UNDO) ? SC_PERFORMED_REDO : SC_PERFORMED_UNDO;
	}

	intptr_t			startLine;
	bool				lineReplace;
	int					restoreAction;
	std::vector<int>	markers;
	int					nextLineMarker;

	const std::shared_ptr<UndoData> undoInfo;
};


/**
 *  \struct
 *  \brief
 */
struct DeletedSectionsList
{
	DeletedSectionsList() : _lastPushTimeMark(0) {}

	std::vector<DeletedSection>& get()
	{
		return _sections;
	}

	bool push(int view, int currAction, intptr_t startLine, intptr_t len,
			const std::shared_ptr<DeletedSection::UndoData>& undo, bool recompareOnChange);
	std::shared_ptr<DeletedSection::UndoData> pop(int view, int currAction, intptr_t startLine);

	void clear()
	{
		_sections.clear();
	}

private:
	DWORD						_lastPushTimeMark;
	std::vector<DeletedSection>	_sections;
};


bool DeletedSectionsList::push(int view, int currAction, intptr_t startLine, intptr_t len,
	const std::shared_ptr<DeletedSection::UndoData>& undo, bool recompareOnChange)
{
	if (len < 1)
		return false;

	// Is it line replacement revert operation?
	if (!_sections.empty() && _sections.back().restoreAction == currAction && _sections.back().lineReplace)
		return false;

	DeletedSection delSection(currAction, startLine, undo);

	if (!recompareOnChange)
	{
		delSection.markers = getMarkers(view, startLine, len, MARKER_MASK_ALL);

		if (startLine + len < getLinesCount(view))
			delSection.nextLineMarker = CallScintilla(view, SCI_MARKERGET, startLine + len, 0) & MARKER_MASK_ALL;
	}
	else
	{
		clearMarks(view, startLine, len);
	}

	_sections.push_back(delSection);

	_lastPushTimeMark = ::GetTickCount();

	return true;
}


std::shared_ptr<DeletedSection::UndoData> DeletedSectionsList::pop(int view, int currAction, intptr_t startLine)
{
	if (_sections.empty())
		return nullptr;

	DeletedSection& last = _sections.back();

	if (last.startLine != startLine)
		return nullptr;

	if (last.restoreAction != currAction)
	{
		// Try to guess if this is the insert part of line replacement operation
		if (::GetTickCount() < _lastPushTimeMark + 40)
			last.lineReplace = true;

		return nullptr;
	}

	if (!last.markers.empty())
	{
		setMarkers(view, last.startLine, last.markers);

		if (last.nextLineMarker)
		{
			clearMarks(view, startLine + last.markers.size());
			CallScintilla(view, SCI_MARKERADDSET, startLine + last.markers.size(), last.nextLineMarker);
		}
	}

	std::shared_ptr<DeletedSection::UndoData> undo = last.undoInfo;

	_sections.pop_back();

	return undo;
}


enum Temp_t
{
	NO_TEMP = 0,
	LAST_SAVED_TEMP,
	CLIPBOARD_TEMP,
	SVN_TEMP,
	GIT_TEMP
};


enum HideFlags_t
{
	NO_HIDE = 0,
	HIDE_MATCHES = 1 << 0,
	HIDE_NEW_LINES = 1 << 1,
	HIDE_CHANGED_LINES = 1 << 2,
	HIDE_MOVED_LINES = 1 << 3,
	HIDE_OUTSIDE_SELECTIONS = 1 << 4,
	FORCE_REHIDING = 1 << 5
};


/**
 *  \class
 *  \brief
 */
class ComparedFile
{
public:
	ComparedFile() : isTemp(NO_TEMP) {}

	void initFromCurrent(bool currFileIsNew);
	void updateFromCurrent();
	void updateView();
	void clear(bool keepDeleteHistory = false);
	void onBeforeClose() const;
	void close() const;
	void restore(bool unhideLines = false) const;
	bool isOpen() const;

	bool pushDeletedSection(int sciAction, intptr_t startLine, intptr_t len,
		const std::shared_ptr<DeletedSection::UndoData>& undo, bool recompareOnChange)
	{
		return _deletedSections.push(compareViewId, sciAction, startLine, len, undo, recompareOnChange);
	}

	std::shared_ptr<DeletedSection::UndoData> popDeletedSection(int sciAction, intptr_t startLine)
	{
		return _deletedSections.pop(compareViewId, sciAction, startLine);
	}

	Temp_t	isTemp;
	bool	isNew;

	int		originalViewId;
	int		originalPos;
	int		compareViewId;

	LRESULT		buffId;
	intptr_t	sciDoc;
	wchar_t		name[MAX_PATH];

private:
	DeletedSectionsList _deletedSections;
};


/**
 *  \class
 *  \brief
 */
class ComparedPair
{
public:
	inline ComparedFile& getFileByViewId(int viewId);
	inline ComparedFile& getFileByBuffId(LRESULT buffId);
	inline ComparedFile& getOtherFileByBuffId(LRESULT buffId);
	inline ComparedFile& getFileBySciDoc(intptr_t sciDoc);
	inline ComparedFile& getOldFile();
	inline ComparedFile& getNewFile();

	void positionFiles(bool recompare);
	void restoreFiles(LRESULT currentBuffId);

	void setStatus();
	std::wstring getSummary();

	void adjustAlignment(int view, intptr_t line, intptr_t offset);

	void setCompareDirty(bool nppReplace = false)
	{
		compareDirty = true;

		if (nppReplace)
			nppReplaceDone = true;

		if (!inEqualizeMode)
			manuallyChanged = true;
	}

	ComparedFile	file[2];
	int				relativePos;

	CompareOptions	options;

	CompareSummary	summary;

	bool			forcedIgnoreEOL		= false;
	bool			forcedNoManualSync	= false;

	unsigned		hideFlags			= NO_HIDE;

	bool			compareDirty		= false;
	bool			nppReplaceDone		= false;
	bool			manuallyChanged		= false;
	int				inEqualizeMode		= 0;

	int				autoRecompareDelay	= 0;
};


/**
 *  \class
 *  \brief
 */
class NewCompare
{
public:
	NewCompare(bool currFileIsNew, bool markFirstName);
	~NewCompare();

	ComparedPair	pair;

private:
	wchar_t	_firstTabText[64];
};


using CompareList_t = std::vector<ComparedPair>;


/**
 *  \class
 *  \brief
 */
class DelayedActivate : public DelayedWork
{
public:
	DelayedActivate() : DelayedWork() {}
	virtual ~DelayedActivate() = default;

	virtual void operator()();

	inline void operator()(LRESULT buff)
	{
		buffId = buff;
		operator()();
	}

	LRESULT buffId;
};


/**
 *  \class
 *  \brief
 */
class DelayedClose : public DelayedWork
{
public:
	DelayedClose() : DelayedWork() {}
	virtual ~DelayedClose() = default;

	virtual void operator()();

	std::vector<LRESULT> closedBuffs;
};


/**
 *  \class
 *  \brief
 */
class DelayedAlign : public DelayedWork
{
public:
	DelayedAlign() : DelayedWork() {}
	virtual ~DelayedAlign() = default;

	void post(UINT delay_ms, bool force)
	{
		cancel();
		_force = force;
		DelayedWork::post(delay_ms);
	}

	virtual void operator()();

private:
	bool _force {false};
};


/**
 *  \class
 *  \brief
 */
class DelayedRecompare : public DelayedWork
{
public:
	DelayedRecompare() : DelayedWork() {}
	virtual ~DelayedRecompare() = default;

	virtual void operator()();
};


/**
 *  \class
 *  \brief
 */
class SelectRangeTimeout : public DelayedWork
{
public:
	SelectRangeTimeout(int view, intptr_t startPos, intptr_t endPos) : DelayedWork(), _view(view)
	{
		_sel = getSelection(view);

		setSelection(view, startPos, endPos);
	}

	virtual ~SelectRangeTimeout();

	virtual void operator()();

private:
	int	_view;

	std::pair<intptr_t, intptr_t> _sel;
};


/**
 *  \class
 *  \brief
 */
class LineArrowMarkTimeout : public DelayedWork
{
public:
	LineArrowMarkTimeout(int view, int markerHandle) : DelayedWork(), _view(view), _markerHandle(markerHandle) {}
	virtual ~LineArrowMarkTimeout();

	virtual void operator()();

private:
	int	_view;
	int	_markerHandle;
};


/**
 *  \struct
 *  \brief
 */
struct TempMark_t
{
	const wchar_t*	fileMark;
	const char*		tabMark;
};


static const TempMark_t tempMark[] =
{
	{ L"",				"" },
	{ L"_LastSave",		"MARK_LAST_SAVE" },
	{ L"Clipboard_",	"MARK_CLIPBOARD" },
	{ L"_SVN",			"MARK_SVN" },
	{ L"_Git",			"MARK_GIT" }
};


CompareList_t compareList;
std::unique_ptr<NewCompare> newCompare = nullptr;

int notificationsLock = 0;

std::unique_ptr<ViewLocation> storedLocation = nullptr;
std::vector<int> copiedSectionMarks;

// Compare/Re-compare flags
std::chrono::milliseconds firstUpdateGuardDuration {0};
std::chrono::steady_clock::time_point prevUpdateTime;

LRESULT currentlyActiveBuffID = 0;

DelayedActivate		delayedActivation;
DelayedClose		delayedClosure;
DelayedAlign		delayedAlign;
DelayedRecompare	delayedRecompare;

NavDialog			NavDlg;

toolbarIconsWithDarkMode	tbSetFirst		{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbCompare		{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbCompareSel	{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbClearCompare	{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbFirst			{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbPrev			{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbNext			{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbLast			{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbDiffsFilters	{nullptr, nullptr, nullptr};
toolbarIconsWithDarkMode	tbNavBar		{nullptr, nullptr, nullptr};

HINSTANCE hInstance;
FuncItem funcItem[NB_MENU_COMMANDS] = { 0 };

// Declare local functions that appear before they are defined
void onBufferActivated(LRESULT buffId);
void temporaryRangeSelect(int view, intptr_t startPos = -1, intptr_t endPos = -1);
void setArrowMark(int view, intptr_t line = -1, bool down = true);
intptr_t getAlignmentIdxAfter(const AlignmentViewData AlignmentPair::*pView, const AlignmentInfo_t &alignInfo,
		intptr_t line);
intptr_t getAlignmentLine(const AlignmentInfo_t &alignInfo, int view, intptr_t line);


void NppState::enableClearCommands(bool enable) const
{
	HMENU hMenu = (HMENU)::SendMessageW(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);

	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ACTIVE]._cmdID,
			MF_BYCOMMAND | ((!enable && !compareMode) ? MF_GRAYED : MF_ENABLED));

	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ALL]._cmdID,
			MF_BYCOMMAND | ((!enable && compareList.empty()) ? MF_GRAYED : MF_ENABLED));

	::DrawMenuBar(nppData._nppHandle);

	HWND hNppToolbar = NppToolbarHandleGetter::get();
	if (hNppToolbar)
		::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_CLEAR_ACTIVE]._cmdID, enable || compareMode);
}


void NppState::enableNppScrollCommands(bool enable) const
{
	HMENU hMenu = (HMENU)::SendMessageW(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);
	const int flag = MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED);

	::EnableMenuItem(hMenu, IDM_VIEW_SYNSCROLLH, flag);
	::EnableMenuItem(hMenu, IDM_VIEW_SYNSCROLLV, flag);

	::DrawMenuBar(nppData._nppHandle);

	HWND hNppToolbar = NppToolbarHandleGetter::get();
	if (hNppToolbar)
	{
		::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, IDM_VIEW_SYNSCROLLH, enable);
		::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, IDM_VIEW_SYNSCROLLV, enable);
	}
}


// Update plugin localization and set menu items accordingly
void NppState::updateLocalization()
{
	if (!Strings::get().read(getLocalization()))
	{
		updateMenuAndToolbar();
		return;
	}

	HMENU hMenu = (HMENU)::SendMessageW(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);

	if (!hMenu)
		return;

	const struct { int id; const char* key; } items[] = {
		{ CMD_SET_FIRST,			"CMD_SET_FIRST" },
		{ CMD_COMPARE,				"CMD_COMPARE" },
		{ CMD_COMPARE_SEL,			"CMD_COMPARE_SEL" },
		{ CMD_FIND_UNIQUE,			"CMD_FIND_UNIQUE" },
		{ CMD_FIND_UNIQUE_SEL,		"CMD_FIND_UNIQUE_SEL" },
		{ CMD_LAST_SAVE_DIFF,		"CMD_LAST_SAVE_DIFF" },
		{ CMD_CLIPBOARD_DIFF,		"CMD_CLIPBOARD_DIFF" },
		{ CMD_SVN_DIFF,				"CMD_SVN_DIFF" },
		{ CMD_GIT_DIFF,				"CMD_GIT_DIFF" },
		{ CMD_CLEAR_ACTIVE,			"CMD_CLEAR_ACTIVE" },
		{ CMD_CLEAR_ALL,			"CMD_CLEAR_ALL" },
		{ CMD_PREV,					"CMD_PREV" },
		{ CMD_NEXT,					"CMD_NEXT" },
		{ CMD_FIRST,				"CMD_FIRST" },
		{ CMD_LAST,					"CMD_LAST" },
		{ CMD_PREV_CHANGE_POS,		"CMD_PREV_CHANGE_POS" },
		{ CMD_NEXT_CHANGE_POS,		"CMD_NEXT_CHANGE_POS" },
		{ CMD_COMPARE_SUMMARY,		"CMD_COMPARE_SUMMARY" },
		{ CMD_COPY_VISIBLE,			"CMD_COPY_VISIBLE" },
		{ CMD_DELETE_VISIBLE,		"CMD_DELETE_VISIBLE" },
		{ CMD_BOOKMARK_VISIBLE,		"CMD_BOOKMARK_VISIBLE" },
		{ CMD_GENERATE_PATCH,		"CMD_GENERATE_PATCH" },
		{ CMD_APPLY_PATCH,			"CMD_APPLY_PATCH" },
		{ CMD_REVERT_PATCH,			"CMD_REVERT_PATCH" },
		{ CMD_COMPARE_OPTIONS,		"CMD_COMPARE_OPTIONS" },
		{ CMD_BOOKMARKS_SYNC,		"CMD_BOOKMARKS_SYNC" },
		{ CMD_DIFFS_VISUAL_FILTERS,	"CMD_DIFFS_VISUAL_FILTERS" },
		{ CMD_NAV_BAR,				"CMD_NAV_BAR" },
		{ CMD_AUTO_RECOMPARE,		"CMD_AUTO_RECOMPARE" },
		{ CMD_SETTINGS,				"CMD_SETTINGS" },
		{ CMD_ABOUT,				"CMD_ABOUT" }
	};

	const auto& str = Strings::get();

	wchar_t current[128];

	MENUITEMINFOW mi {0};
	mi.cbSize = sizeof(mi);
	mi.fMask = MIIM_STRING;

	for (const auto& it : items)
	{
		std::wstring cmdStr = str[it.key];

		// Basic menu item length precaution
		if (cmdStr.empty() || cmdStr.size() > 64)
			continue;

		mi.dwTypeData = NULL;

		if (::GetMenuItemInfoW(hMenu, funcItem[it.id]._cmdID, FALSE, &mi))
		{
			if (++mi.cch < _countof(current))
			{
				mi.dwTypeData = current;

				::GetMenuItemInfoW(hMenu, funcItem[it.id]._cmdID, FALSE, &mi);

				wchar_t* tab = wcschr(current, L'\t');
				if (tab)
					cmdStr.append(tab); // keep existing accelerator part (e.g. "\tCtrl+Alt+C")
			}
		}

		mi.dwTypeData = const_cast<wchar_t*>(cmdStr.c_str());

		::SetMenuItemInfoW(hMenu, funcItem[it.id]._cmdID, FALSE, &mi);
	}

	updateMenuAndToolbar();
}


void NppState::updateMenuAndToolbar()
{
	HMENU hMenu = (HMENU)::SendMessageW(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);
	const int flag = MF_BYCOMMAND | (compareMode ? MF_ENABLED : MF_GRAYED);

	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ACTIVE]._cmdID,
			MF_BYCOMMAND | ((!compareMode && !newCompare) ? MF_GRAYED : MF_ENABLED));

	::EnableMenuItem(hMenu, funcItem[CMD_CLEAR_ALL]._cmdID,
			MF_BYCOMMAND | ((compareList.empty() && !newCompare) ? MF_GRAYED : MF_ENABLED));

	::EnableMenuItem(hMenu, funcItem[CMD_FIRST]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_PREV]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_NEXT]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_LAST]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_PREV_CHANGE_POS]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_NEXT_CHANGE_POS]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_COMPARE_SUMMARY]._cmdID, flag);
	::EnableMenuItem(hMenu, funcItem[CMD_GENERATE_PATCH]._cmdID, flag);

	::DrawMenuBar(nppData._nppHandle);

	HWND hNppToolbar = NppToolbarHandleGetter::get();
	if (hNppToolbar)
	{
		::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_CLEAR_ACTIVE]._cmdID, compareMode || newCompare);
		::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_FIRST]._cmdID, compareMode);
		::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_PREV]._cmdID, compareMode);
		::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_NEXT]._cmdID, compareMode);
		::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_LAST]._cmdID, compareMode);
	}
}


void NppState::save()
{
	HMENU hMenu = (HMENU)::SendMessageW(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

	_syncVScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0;
	_syncHScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLH, MF_BYCOMMAND) & MF_CHECKED) != 0;

	_lineNumMode = (int)::SendMessageW(nppData._nppHandle, NPPM_GETLINENUMBERWIDTHMODE, 0, 0);

	if (_mainZoom == 0)
		_mainZoom = static_cast<int>(CallScintilla(MAIN_VIEW, SCI_GETZOOM, 0, 0));

	if (_subZoom == 0)
		_subZoom = static_cast<int>(CallScintilla(SUB_VIEW, SCI_GETZOOM, 0, 0));
}


void NppState::setNormalMode(bool forceUpdate)
{
	if (compareMode)
	{
		compareMode = false;

		restoreMultilineTab();

		if (NavDlg.isVisible())
			NavDlg.Hide();

		if (!isSingleView())
		{
			enableNppScrollCommands(true);

			HMENU hMenu = (HMENU)::SendMessageW(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPMAINMENU, 0);

			bool syncScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLV, MF_BYCOMMAND) & MF_CHECKED) != 0;
			if (syncScroll != _syncVScroll)
				::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLV);

			syncScroll = (::GetMenuState(hMenu, IDM_VIEW_SYNSCROLLH, MF_BYCOMMAND) & MF_CHECKED) != 0;
			if (syncScroll != _syncHScroll)
				::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLH);
		}

		if (_lineNumMode != LINENUMWIDTH_CONSTANT)
			::SendMessageW(nppData._nppHandle, NPPM_SETLINENUMBERWIDTHMODE, 0, _lineNumMode);

		CallScintilla(MAIN_VIEW, SCI_SETZOOM, _mainZoom, 0);
		CallScintilla(SUB_VIEW, SCI_SETZOOM, _subZoom, 0);

		updateMenuAndToolbar();
	}
	else if (forceUpdate)
	{
		restoreMultilineTab();

		CallScintilla(MAIN_VIEW, SCI_SETZOOM, _mainZoom, 0);
		CallScintilla(SUB_VIEW, SCI_SETZOOM, _subZoom, 0);

		updateMenuAndToolbar();
	}
}


void NppState::setCompareMode(bool clearHorizontalScroll)
{
	if (compareMode)
		return;

	compareMode = true;

	save();

	toSingleLineTab();

	if (clearHorizontalScroll)
	{
		CallScintilla(MAIN_VIEW, SCI_GOTOLINE, getCurrentLine(MAIN_VIEW), 0);
		CallScintilla(SUB_VIEW, SCI_GOTOLINE, getCurrentLine(SUB_VIEW), 0);
	}

	// Disable N++ vertical scroll - we handle it manually because of the Word Wrap
	if (_syncVScroll)
		::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLV);

	// Yaron - Enable N++ horizontal scroll sync
	if (!_syncHScroll)
		::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SYNSCROLLH);

	if (_lineNumMode != LINENUMWIDTH_CONSTANT)
		::SendMessageW(nppData._nppHandle, NPPM_SETLINENUMBERWIDTHMODE, 0, LINENUMWIDTH_CONSTANT);

	// synchronize zoom levels
	if (_compareZoom == 0)
	{
		_compareZoom = static_cast<int>(CallScintilla(MAIN_VIEW, SCI_GETZOOM, 0, 0));
		CallScintilla(SUB_VIEW, SCI_SETZOOM, _compareZoom, 0);
	}
	else
	{
		CallScintilla(MAIN_VIEW, SCI_SETZOOM, _compareZoom, 0);
		CallScintilla(SUB_VIEW, SCI_SETZOOM, _compareZoom, 0);
	}

	enableNppScrollCommands(false);
	updateMenuAndToolbar();
}


void NppState::refreshTabBar(HWND hTabBar)
{
	if (::IsWindowVisible(hTabBar) && (TabCtrl_GetItemCount(hTabBar) > 1))
		{
		const int currentTabIdx = TabCtrl_GetCurSel(hTabBar);

		TabCtrl_SetCurFocus(hTabBar, (currentTabIdx) ? 0 : 1);
		TabCtrl_SetCurFocus(hTabBar, currentTabIdx);
	}
}


void NppState::refreshTabBars()
{
	HWND hNppMainTabBar	= NppTabHandleGetter::get(MAIN_VIEW);
	HWND hNppSubTabBar	= NppTabHandleGetter::get(SUB_VIEW);

	if (hNppMainTabBar && hNppSubTabBar)
	{
		HWND currentView = getCurrentView();

		refreshTabBar(hNppSubTabBar);
		refreshTabBar(hNppMainTabBar);

		::SetFocus(currentView);
	}
}


void NppState::toSingleLineTab()
{
	if (!_restoreMultilineTab)
	{
		HWND hNppMainTabBar = NppTabHandleGetter::get(MAIN_VIEW);
		HWND hNppSubTabBar = NppTabHandleGetter::get(SUB_VIEW);

		if (hNppMainTabBar && hNppSubTabBar)
		{
			RECT tabRec;
			::GetWindowRect(hNppMainTabBar, &tabRec);
			const int mainTabYPos = tabRec.top;

			::GetWindowRect(hNppSubTabBar, &tabRec);
			const int subTabYPos = tabRec.top;

			// Both views are side-by-side positioned
			if (mainTabYPos == subTabYPos)
			{
				LONG_PTR tabStyle = ::GetWindowLongPtrW(hNppMainTabBar, GWL_STYLE);

				if ((tabStyle & TCS_MULTILINE) && !(tabStyle & TCS_VERTICAL))
				{
					::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

					::SetWindowLongPtrW(hNppMainTabBar, GWL_STYLE, tabStyle & ~TCS_MULTILINE);
					::SendMessageW(hNppMainTabBar, WM_TABSETSTYLE, 0, 0);

					tabStyle = ::GetWindowLongPtrW(hNppSubTabBar, GWL_STYLE);
					::SetWindowLongPtrW(hNppSubTabBar, GWL_STYLE, tabStyle & ~TCS_MULTILINE);
					::SendMessageW(hNppSubTabBar, WM_TABSETSTYLE, 0, 0);

					::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);

					// Scroll current tab into view
					refreshTabBars();

					_restoreMultilineTab = true;
				}
			}
		}
	}
}


void NppState::restoreMultilineTab()
{
	if (_restoreMultilineTab)
	{
		_restoreMultilineTab = false;

		HWND hNppMainTabBar = NppTabHandleGetter::get(MAIN_VIEW);
		HWND hNppSubTabBar = NppTabHandleGetter::get(SUB_VIEW);

		if (hNppMainTabBar && hNppSubTabBar)
		{
			LONG_PTR tabStyle = ::GetWindowLongPtrW(hNppMainTabBar, GWL_STYLE);

			::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

			::SetWindowLongPtrW(hNppMainTabBar, GWL_STYLE, tabStyle | TCS_MULTILINE);
			::SendMessageW(hNppMainTabBar, WM_TABSETSTYLE, 0, 0);

			tabStyle = ::GetWindowLongPtrW(hNppSubTabBar, GWL_STYLE);
			::SetWindowLongPtrW(hNppSubTabBar, GWL_STYLE, tabStyle | TCS_MULTILINE);
			::SendMessageW(hNppSubTabBar, WM_TABSETSTYLE, 0, 0);

			::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);
		}
	}
}


void ComparedFile::initFromCurrent(bool currFileIsNew)
{
	isNew = currFileIsNew;
	buffId = getCurrentBuffId();
	originalViewId = getCurrentViewId();
	compareViewId = originalViewId;
	originalPos = posFromBuffId(buffId);
	::SendMessageW(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(name), (LPARAM)name);

	updateFromCurrent();
}


void ComparedFile::updateFromCurrent()
{
	sciDoc = getDocId(getCurrentViewId());

	if (isTemp)
	{
		HWND hNppTabBar = NppTabHandleGetter::get(getCurrentViewId());

		if (hNppTabBar)
		{
			const wchar_t* fileExt = ::PathFindExtensionW(name);

			wchar_t tabName[MAX_PATH];

			wcscpy_s(tabName, _countof(tabName), ::PathFindFileNameW(name));
			::PathRemoveExtensionW(tabName);

			size_t i = wcslen(tabName) - 1 - wcslen(tempMark[isTemp].fileMark);
			for (; i > 0 && tabName[i] != L'_'; --i);

			if (i > 0)
			{
				tabName[i] = 0;
				wcscat_s(tabName, _countof(tabName), fileExt);
				wcscat_s(tabName, _countof(tabName), Strings::get()[tempMark[isTemp].tabMark].c_str());

				TCITEMW tab;
				tab.mask = TCIF_TEXT;
				tab.pszText = tabName;

				::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

				TabCtrl_SetItem(hNppTabBar, posFromBuffId(buffId), &tab);

				::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);
			}
		}
	}
}


void ComparedFile::updateView()
{
	compareViewId = isNew ? Settings.NewFileViewId : ((Settings.NewFileViewId == MAIN_VIEW) ? SUB_VIEW : MAIN_VIEW);
}


void ComparedFile::clear(bool keepDeleteHistory)
{
	temporaryRangeSelect(-1);
	setArrowMark(-1);

	if (!keepDeleteHistory)
		_deletedSections.clear();
}


void ComparedFile::onBeforeClose() const
{
	activateBufferID(buffId);

	const int view = getCurrentViewId();

	clearWindow(view);
	setNormalView(view);
	temporaryRangeSelect(-1);
	setArrowMark(-1);

	if (isTemp)
	{
		CallScintilla(view, SCI_SETSAVEPOINT, 0, 0);
		::SetFileAttributesW(name, FILE_ATTRIBUTE_NORMAL);
		::DeleteFileW(name);
	}
}


void ComparedFile::close() const
{
	onBeforeClose();

	::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);
}


void ComparedFile::restore(bool unhideLines) const
{
	if (isTemp)
	{
		close();
		return;
	}

	activateBufferID(buffId);

	const int view = getCurrentViewId();

	intptr_t biasLine = getFirstLine(view);
	biasLine = getDocLineFromVisible(view, getFirstVisibleLine(view) + getLineAnnotation(view, biasLine) +
			getWrapCount(view, biasLine));

	ViewLocation loc(view, biasLine);

	if (unhideLines)
		unhideAllLines(view);

	clearWindow(view);
	setNormalView(view);
	temporaryRangeSelect(-1);
	setArrowMark(-1);

	loc.restore(Settings.FollowingCaret);

	if (viewIdFromBuffId(buffId) != originalViewId)
	{
		moveFileToOtherView();

		if (!isOpen())
			return;

		const int currentPos = posFromBuffId(buffId);

		if (originalPos >= currentPos)
			return;

		for (int i = currentPos - originalPos; i; --i)
			::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_TAB_MOVEBACKWARD);
	}
}


bool ComparedFile::isOpen() const
{
	return (::SendMessageW(nppData._nppHandle, NPPM_GETFULLPATHFROMBUFFERID, buffId, (LPARAM)NULL) >= 0);
}


ComparedFile& ComparedPair::getFileByViewId(int viewId)
{
	return (viewIdFromBuffId(file[0].buffId) == viewId) ? file[0] : file[1];
}


ComparedFile& ComparedPair::getFileByBuffId(LRESULT buffId)
{
	return (file[0].buffId == buffId) ? file[0] : file[1];
}


ComparedFile& ComparedPair::getOtherFileByBuffId(LRESULT buffId)
{
	return (file[0].buffId == buffId) ? file[1] : file[0];
}


ComparedFile& ComparedPair::getFileBySciDoc(intptr_t sciDoc)
{
	return (file[0].sciDoc == sciDoc) ? file[0] : file[1];
}


ComparedFile& ComparedPair::getOldFile()
{
	return file[0].isNew ? file[1] : file[0];
}


ComparedFile& ComparedPair::getNewFile()
{
	return file[0].isNew ? file[0] : file[1];
}


void ComparedPair::positionFiles(bool recompare)
{
	const LRESULT currentBuffId = getCurrentBuffId();

	ComparedFile& oldFile = getOldFile();
	ComparedFile& newFile = getNewFile();

	oldFile.updateView();
	newFile.updateView();

	relativePos = (oldFile.originalViewId != newFile.originalViewId) ? 0 :
			(oldFile.originalViewId == oldFile.compareViewId) ?
			newFile.originalPos - oldFile.originalPos : oldFile.originalPos - newFile.originalPos;

	if (viewIdFromBuffId(oldFile.buffId) != oldFile.compareViewId)
	{
		activateBufferID(oldFile.buffId);

		moveFileToOtherView();
		oldFile.updateFromCurrent();
	}

	if (viewIdFromBuffId(newFile.buffId) != newFile.compareViewId)
	{
		activateBufferID(newFile.buffId);

		moveFileToOtherView();
		newFile.updateFromCurrent();
	}

	if (oldFile.sciDoc != getDocId(oldFile.compareViewId))
		activateBufferID(oldFile.buffId);

	if (newFile.sciDoc != getDocId(newFile.compareViewId))
		activateBufferID(newFile.buffId);

	// If compare type is LastSaved or Git or SVN diff and folds/hidden lines are to be ignored
	// then expand all folds/hidden lines in the new (updated) file as its old version is restored unfolded/unhidden
	// and we shouldn't actually ignore folds/hidden lines
	const bool expandNewFileFolds = !recompare && ((options.ignoreFoldedLines || options.ignoreHiddenLines) &&
			oldFile.isTemp && oldFile.isTemp != CLIPBOARD_TEMP);

	if (expandNewFileFolds)
		CallScintilla(newFile.compareViewId, SCI_FOLDALL, SC_FOLDACTION_EXPAND, 0);

	activateBufferID(currentBuffId);
}


void ComparedPair::restoreFiles(LRESULT currentBuffId = -1)
{
	// Check if position update is needed -
	// this is for relative re-positioning to keep files initial order consistent
	if (relativePos)
	{
		ComparedFile* biasFile;
		ComparedFile* movedFile;

		// One of the files is in its original view and won't be moved - this is the bias file
		if (viewIdFromBuffId(file[0].buffId) == file[0].originalViewId)
		{
			biasFile = &file[0];
			movedFile = &file[1];
		}
		else
		{
			biasFile = &file[1];
			movedFile = &file[0];
		}

		if (biasFile->originalPos > movedFile->originalPos)
		{
			const int newPos = posFromBuffId(biasFile->buffId);

			if (newPos != biasFile->originalPos && newPos < movedFile->originalPos)
				movedFile->originalPos = newPos;
		}
	}

	if (file[0].originalViewId == file[0].compareViewId)
	{
		file[0].restore(file[1].isTemp);
		file[1].restore();
	}
	else
	{
		file[1].restore(file[0].isTemp);
		file[0].restore();
	}

	if (currentBuffId != -1)
		activateBufferID(currentBuffId);
}


void ComparedPair::setStatus()
{
	if (Settings.StatusInfo == StatusType::STATUS_DISABLED)
		return;

	const auto& str = Strings::get();

	std::wstring info;

	if (compareDirty)
	{
		if (manuallyChanged)
			info = str["STATUS_MANUALLY_CHANGED"];
		else
			info = str["STATUS_CHANGED"];
	}
	else
	{
		if (options.selectionCompare)
		{
			if (options.findUniqueMode)
				info = str["STATUS_FIND_UNIQUE_SEL"];
			else
				info = str["STATUS_COMPARE_SEL"];

			info += L": [";
			info += std::to_wstring(options.selections[MAIN_VIEW].first + 1);
			info += L'-';
			info += std::to_wstring(options.selections[MAIN_VIEW].second + 1);
			info += L"] [";
			info += std::to_wstring(options.selections[SUB_VIEW].first + 1);
			info += L'-';
			info += std::to_wstring(options.selections[SUB_VIEW].second + 1);
			info += L']';
		}
		else
		{
			if (options.findUniqueMode)
				info = str["STATUS_FIND_UNIQUE"];
			else
				info = str["STATUS_COMPARE"];
		}

		if (Settings.StatusInfo == StatusType::COMPARE_OPTIONS)
		{
			const bool hasDetectOpts = (!options.findUniqueMode &&
					(options.detectSubBlockDiffs || options.detectMoves));

			if (hasDetectOpts)
			{
				info += L"    ";
				info += str["STATUS_DETECT"];

				if (options.detectMoves)
					info += str["STATUS_MOVES"];

				if (options.detectSubBlockDiffs)
				{
					info += str["STATUS_SUB_BLOCKS"];

					if (options.detectSubLineMoves)
						info += str["STATUS_SUB_MOVES"];
					if (options.detectCharDiffs)
						info += str["STATUS_CHARS"];
				}
			}

			if (options.ignoreChangedSpaces || options.ignoreAllSpaces || options.ignoreEOL ||
				options.ignoreEmptyLines || options.ignoreCase || options.ignoreRegex || options.ignoreFoldedLines ||
				options.ignoreHiddenLines)
			{
				info += L"    ";
				info += str["STATUS_IGNORE"];

				if (options.ignoreEmptyLines)
					info += str["STATUS_EMPTY_LINES"];
				if (options.ignoreFoldedLines)
					info += str["STATUS_FOLDED_LINES"];
				if (options.ignoreHiddenLines)
					info += str["STATUS_HIDDEN_LINES"];
				if (options.ignoreEOL)
					info += str["STATUS_EOL"];
				if (options.ignoreAllSpaces)
					info += str["STATUS_ALL_SPACES"];
				else if (options.ignoreChangedSpaces)
					info += str["STATUS_CHANGED_SPACES"];
				if (options.ignoreCase)
					info += str["STATUS_CASE"];
				if (options.ignoreRegex)
					info += str["STATUS_REGEX"];
			}

			if (options.bookmarksAsSync)
			{
				info += L"    ";
				info += str["STATUS_SYNC_POINTS"];
				info += std::to_wstring(options.syncPoints.size());
			}
		}
		else if (Settings.StatusInfo == StatusType::DIFFS_SUMMARY)
		{
			info += L"    ";
			info += str["STATUS_DIFF_LINES"];
			info += std::to_wstring(summary.diffLines);

			if (summary.added)
			{
				info += str["STATUS_ADDED_LINES"];
				info += std::to_wstring(summary.added);
			}
			if (summary.removed)
			{
				info += str["STATUS_REMOVED_LINES"];
				info += std::to_wstring(summary.removed);
			}
			if (summary.moved)
			{
				info += str["STATUS_MOVED_LINES"];
				info += std::to_wstring(summary.moved);
			}
			if (summary.changed)
			{
				info += str["STATUS_CHANGED_LINES"];
				info += std::to_wstring(summary.changed);
			}
			if (summary.match)
			{
				info += L".  ";
				info += str["STATUS_MATCHING_LINES"];
				info += std::to_wstring(summary.match);
			}
		}
	}

	::SendMessageW(nppData._nppHandle, NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE,
			static_cast<LPARAM>((LONG_PTR)info.c_str()));
}


std::wstring ComparedPair::getSummary()
{
	const auto& str = Strings::get();

	std::wstring info;

	if (options.findUniqueMode)
		info = str["SUMMARY_FIND_UNIQUE"];
	else
		info = str["SUMMARY_COMPARE"];

	info += L'\n';
	info += str["STATUS_DIFF_LINES"];
	info += std::to_wstring(summary.diffLines);
	info += L'\n';

	if (summary.added)
	{
		info += str["STATUS_ADDED_LINES"];
		info += std::to_wstring(summary.added);
		info += L'\n';
	}
	if (summary.removed)
	{
		info += str["STATUS_REMOVED_LINES"];
		info += std::to_wstring(summary.removed);
		info += L'\n';
	}
	if (summary.moved)
	{
		info += str["STATUS_MOVED_LINES"];
		info += std::to_wstring(summary.moved);
		info += L'\n';
	}
	if (summary.changed)
	{
		info += str["STATUS_CHANGED_LINES"];
		info += std::to_wstring(summary.changed);
		info += L'\n';
	}
	if (summary.match)
	{
		info += str["STATUS_MATCHING_LINES"];
		info += std::to_wstring(summary.match);
		info += L'\n';
	}

	info += L'\n';
	info += str["SUMMARY_OPTIONS"];

	const bool hasDetectOpts = (!options.findUniqueMode && (options.detectSubBlockDiffs || options.detectMoves));

	if (hasDetectOpts)
	{
		info += L'\n';
		info += str["STATUS_DETECT"];
		info += L'\n';

		if (options.detectMoves)
		{
			info += str["STATUS_MOVES"];
			info += L'\n';
		}

		if (options.detectSubBlockDiffs)
		{
			info += str["STATUS_SUB_BLOCKS"];
			info += L'\n';

			if (options.detectSubLineMoves)
			{
				info += str["STATUS_SUB_MOVES"];
				info += L'\n';
			}
			if (options.detectCharDiffs)
			{
				info += str["STATUS_CHARS"];
				info += L'\n';
			}
		}
	}

	const bool hasIgnoreOpts = (options.ignoreChangedSpaces || options.ignoreAllSpaces || options.ignoreEOL ||
								options.ignoreEmptyLines || options.ignoreCase || options.ignoreRegex ||
								options.ignoreFoldedLines || options.ignoreHiddenLines);

	if (hasIgnoreOpts)
	{
		info += L'\n';
		info += str["STATUS_IGNORE"];
		info += L'\n';

		if (options.ignoreEmptyLines)
		{
			info += str["STATUS_EMPTY_LINES"];
			info += L'\n';
		}
		if (options.ignoreFoldedLines)
		{
			info += str["STATUS_FOLDED_LINES"];
			info += L'\n';
		}
		if (options.ignoreHiddenLines)
		{
			info += str["STATUS_HIDDEN_LINES"];
			info += L'\n';
		}
		if (options.ignoreEOL)
		{
			info += str["STATUS_EOL"];
			info += L'\n';
		}
		if (options.ignoreAllSpaces)
		{
			info += str["STATUS_ALL_SPACES"];
			info += L'\n';
		}
		else if (options.ignoreChangedSpaces)
		{
			info += str["STATUS_CHANGED_SPACES"];
			info += L'\n';
		}
		if (options.ignoreCase)
		{
			info += str["STATUS_CASE"];
			info += L'\n';
		}
		if (options.ignoreRegex)
		{
			info += str["STATUS_REGEX"];
			info += L'\n';
		}
	}

	if (!hasDetectOpts && !hasIgnoreOpts)
	{
		info += L'\n';
		info += str["SUMMARY_NO_IGNORE"];
	}

	if (options.bookmarksAsSync)
	{
		info += L'\n';
		info += str["STATUS_SYNC_POINTS"];
		info += std::to_wstring(options.syncPoints.size());
		info += L'\n';
	}

	info += L'\n';

	return info;
}


void ComparedPair::adjustAlignment(int view, intptr_t line, intptr_t offset)
{
	AlignmentViewData AlignmentPair::*alignView = (view == MAIN_VIEW) ? &AlignmentPair::main : &AlignmentPair::sub;
	AlignmentInfo_t& alignInfo = summary.alignmentInfo;

	const intptr_t startIdx = getAlignmentIdxAfter(alignView, alignInfo, line);

	if ((startIdx < static_cast<intptr_t>(alignInfo.size())) && ((alignInfo[startIdx].*alignView).line >= line))
	{
		if (offset < 0)
		{
			intptr_t endIdx = startIdx;

			while ((endIdx < static_cast<intptr_t>(alignInfo.size())) &&
					(line > (alignInfo[endIdx].*alignView).line + offset))
				++endIdx;

			if (endIdx > startIdx)
				alignInfo.erase(alignInfo.begin() + startIdx, alignInfo.begin() + endIdx);
		}

		for (intptr_t i = startIdx; i < static_cast<intptr_t>(alignInfo.size()); ++i)
			(alignInfo[i].*alignView).line += offset;
	}
}


NewCompare::NewCompare(bool currFileIsNew, bool markFirstName)
{
	_firstTabText[0] = 0;

	pair.file[0].initFromCurrent(currFileIsNew);

	// Enable commands to be able to clear the first file that was just set
	NppState::get().enableClearCommands(true);

	if (markFirstName)
	{
		HWND hNppTabBar = NppTabHandleGetter::get(pair.file[0].originalViewId);

		if (hNppTabBar)
		{
			TCITEMW tab;
			tab.mask = TCIF_TEXT;
			tab.pszText = _firstTabText;
			tab.cchTextMax = _countof(_firstTabText);

			TabCtrl_GetItem(hNppTabBar, pair.file[0].originalPos, &tab);

			std::wstring tabText = _firstTabText;
			tabText += (currFileIsNew) ? Strings::get()["MARK_NEW"] : Strings::get()["MARK_OLD"];

			tab.pszText = const_cast<wchar_t*>(tabText.c_str());
			tab.cchTextMax = static_cast<int>(tabText.size());

			::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

			TabCtrl_SetItem(hNppTabBar, pair.file[0].originalPos, &tab);

			::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);
		}
	}
}


NewCompare::~NewCompare()
{
	if (_firstTabText[0] != 0)
	{
		HWND hNppTabBar = NppTabHandleGetter::get(pair.file[0].originalViewId);

		if (hNppTabBar)
		{
			// This is workaround for Wine issue with tab bar refresh
			::InvalidateRect(hNppTabBar, NULL, FALSE);

			TCITEMW tab;
			tab.mask = TCIF_TEXT;
			tab.pszText = _firstTabText;
			tab.cchTextMax = _countof(_firstTabText);

			::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

			TabCtrl_SetItem(hNppTabBar, posFromBuffId(pair.file[0].buffId), &tab);

			::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);
		}
	}

	if (!NppState::get().compareMode)
		NppState::get().enableClearCommands(false);
}


SelectRangeTimeout::~SelectRangeTimeout()
{
	(*this)();
}


void SelectRangeTimeout::operator()()
{
	if ((_sel.first >= 0) && (_sel.second > _sel.first))
		setSelection(_view, _sel.first, _sel.second);
	else
		clearSelection(_view);
}


LineArrowMarkTimeout::~LineArrowMarkTimeout()
{
	(*this)();
}


void LineArrowMarkTimeout::operator()()
{
	CallScintilla(_view, SCI_MARKERDELETEHANDLE, _markerHandle, 0);
}


CompareList_t::iterator getCompare(LRESULT buffId)
{
	for (CompareList_t::iterator it = compareList.begin(); it < compareList.end(); ++it)
	{
		if (it->file[0].buffId == buffId || it->file[1].buffId == buffId)
			return it;
	}

	return compareList.end();
}


CompareList_t::iterator getCompareBySciDoc(intptr_t sciDoc)
{
	for (CompareList_t::iterator it = compareList.begin(); it < compareList.end(); ++it)
	{
		if (it->file[0].sciDoc == sciDoc || it->file[1].sciDoc == sciDoc)
			return it;
	}

	return compareList.end();
}


void temporaryRangeSelect(int view, intptr_t startPos, intptr_t endPos)
{
	static std::unique_ptr<SelectRangeTimeout> range;

	if (view < 0 || startPos < 0 || endPos < startPos)
	{
		range = nullptr;
		return;
	}

	range = nullptr;
	range = std::make_unique<SelectRangeTimeout>(view, startPos, endPos);

	if (range)
		range->post(2000);
}


void setArrowMark(int view, intptr_t line, bool down)
{
	static std::unique_ptr<LineArrowMarkTimeout> lineArrowMark;

	lineArrowMark = nullptr;

	if (view < 0 || line < 0)
		return;

	const int markerHandle = showArrowSymbol(view, line, down);

	lineArrowMark = std::make_unique<LineArrowMarkTimeout>(view, markerHandle);

	if (lineArrowMark)
		lineArrowMark->post(2000);
}


void showBlankAdjacentArrowMark(int view, intptr_t line, bool down)
{
	if (view < 0)
	{
		setArrowMark(-1);
		return;
	}

	if (line < 0 && Settings.FollowingCaret)
		line = getCurrentLine(view);

	if (line >= 0 && !isLineMarked(view, line, MARKER_MASK_LINE))
	{
		if (isAdjacentAnnotationVisible(view, line, down))
			setArrowMark(view, line, down);
		else if ((line == getEndLine(view)) && isAdjacentAnnotationVisible(view, line, true))
			setArrowMark(view, line, true);
		else
			setArrowMark(-1);
	}
	else
	{
		setArrowMark(-1);
	}
}


intptr_t getCornerLine(int view, bool down, const CompareList_t::iterator& cmpPair)
{
	intptr_t cornerLine;

	if (cmpPair->options.selectionCompare)
	{
		if (down)
			cornerLine = cmpPair->options.selections[view].first;
		else
			cornerLine = cmpPair->options.selections[view].second;
	}
	else
	{
		if (down)
			cornerLine = 0;
		else
			cornerLine = getEndLine(view);
	}

	return cornerLine;
}


std::pair<int, intptr_t> findNextChange(intptr_t mainStartLine, intptr_t subStartLine, bool down,
		bool goToCornerDiff = false)
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return std::make_pair(-1, -1);

	int view			= getCurrentViewId();
	const int otherView	= getOtherViewId(view);

	const int nextMarker = down ? SCI_MARKERNEXT : SCI_MARKERPREVIOUS;

	intptr_t mainNextLine = -1;
	intptr_t subNextLine = -1;

	if (mainStartLine >= 0)
	{
		mainNextLine = CallScintilla(MAIN_VIEW, nextMarker, mainStartLine, MARKER_MASK_LINE);

		if ((mainNextLine == mainStartLine) && !goToCornerDiff)
			mainNextLine = -1;
	}

	if (subStartLine >= 0)
	{
		subNextLine = CallScintilla(SUB_VIEW, nextMarker, subStartLine, MARKER_MASK_LINE);

		if ((subNextLine == subStartLine) && !goToCornerDiff)
			subNextLine = -1;
	}

	intptr_t line				= (view == MAIN_VIEW)		? mainNextLine : subNextLine;
	const intptr_t otherLine	= (otherView == MAIN_VIEW)	? mainNextLine : subNextLine;

	if (line < 0)
	{
		// End of doc reached - no more diffs
		if (otherLine < 0)
			return std::make_pair(-1, -1);

		if (cmpPair->options.findUniqueMode)
		{
			view = otherView;
			line = otherLine;
		}
		else
		{
			line = otherViewMatchingLine(otherView, otherLine);
		}
	}
	else if (otherLine >= 0)
	{
		const intptr_t visibleLine		= getVisibleFromDocLine(view, line);
		const intptr_t otherVisibleLine	= getVisibleFromDocLine(otherView, otherLine);

		const bool switchViews = down ? (otherVisibleLine < visibleLine) : (otherVisibleLine > visibleLine);

		if (switchViews)
		{
			if (cmpPair->options.findUniqueMode)
			{
				view = otherView;
				line = otherLine;
			}
			else
			{
				line = otherViewMatchingLine(otherView, otherLine);

				if (down && isLineAnnotated(view, line) && (line < getEndLine(view)) &&
					((view == MAIN_VIEW && line <= mainStartLine) || (view == SUB_VIEW && line <= subStartLine)))
					++line;
			}
		}
	}

	if (!down && !Settings.HideMatches && isLineAnnotated(view, line) && (line < getEndLine(view)))
		++line;

	if (cmpPair->options.selectionCompare && !isLineMarked(view, line, MARKER_MASK_LINE))
	{
		if (goToCornerDiff)
		{
			if (down && isLineHidden(view, line))
			{
				line = getPreviousUnhiddenLine(view, line);
			}
			else if (!down && (line > cmpPair->options.selections[view].second) &&
				(isLineMarked(view, cmpPair->options.selections[view].second, MARKER_MASK_LINE) ||
				Settings.ShowOnlySelections))
			{
				line = cmpPair->options.selections[view].second;
			}
		}
		else
		{
			if (!down)
			{
				if (line > cmpPair->options.selections[view].second &&
					(isLineMarked(view, cmpPair->options.selections[view].second, MARKER_MASK_LINE) ||
					Settings.ShowOnlySelections))
				{
					line = cmpPair->options.selections[view].second;
				}
				else if (line < cmpPair->options.selections[view].first)
				{
					line = cmpPair->options.selections[view].first;
				}
			}
		}
	}

	return std::make_pair(view, line);
}


std::pair<int, intptr_t> jumpToNextChange(intptr_t mainStartLine, intptr_t subStartLine, bool down,
		bool goToCornerDiff = false, bool doNotBlink = false)
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return std::make_pair(-1, -1);

	if (!cmpPair->options.findUniqueMode && !goToCornerDiff)
	{
		const int view		= getCurrentViewId();
		const int otherView	= getOtherViewId(view);

		const intptr_t edgeLine		= (down ? getLastLine(view) : getFirstLine(view));
		const intptr_t currentLine	= (Settings.FollowingCaret ? getCurrentLine(view) : edgeLine);

		// Is the caret line manually positioned on a screen edge and adjacent to invisible blank diff?
		// Make sure we don't miss it
		if (!isLineMarked(view, currentLine, MARKER_MASK_LINE) &&
			isAdjacentAnnotation(view, currentLine, down) &&
			!isAdjacentAnnotationVisible(view, currentLine, down) &&
			isLineMarked(otherView, otherViewMatchingLine(view, currentLine) + 1, MARKER_MASK_LINE))
		{
			centerAt(view, currentLine);

			showBlankAdjacentArrowMark(view, currentLine, down);

			return std::make_pair(view, currentLine);
		}
	}

	if (!goToCornerDiff && cmpPair->options.selectionCompare && Settings.ShowOnlySelections && !down)
	{
		const int view				= getCurrentViewId();
		const intptr_t startLine	= (view == MAIN_VIEW) ? mainStartLine : subStartLine;

		// Special selections compare corner case
		if (!isLineMarked(view, startLine, MARKER_MASK_LINE) && (startLine == cmpPair->options.selections[view].first))
		{
			down = !down;
			goToCornerDiff = true;
		}
	}

	if (goToCornerDiff)
	{
		mainStartLine	= getCornerLine(MAIN_VIEW, down, cmpPair);
		subStartLine	= getCornerLine(SUB_VIEW, down, cmpPair);
	}

	std::pair<int, intptr_t> nextDiff = findNextChange(mainStartLine, subStartLine, down, goToCornerDiff);

	int view		= nextDiff.first;
	intptr_t line	= nextDiff.second;

	if (view < 0)
	{
		if (goToCornerDiff || Settings.WrapAround)
			return nextDiff;

		// Last diff reached and direction was inverted to find the corner diff
		down = !down;
		goToCornerDiff = true;

		mainStartLine	= getCornerLine(MAIN_VIEW, down, cmpPair);
		subStartLine	= getCornerLine(SUB_VIEW, down, cmpPair);

		nextDiff = findNextChange(mainStartLine, subStartLine, down, goToCornerDiff);

		view = nextDiff.first;
		line = nextDiff.second;

		if (view < 0)
			return nextDiff;

		const intptr_t edgeLine		= (down ? getFirstLine(view) : getLastLine(view));
		const intptr_t currentLine	= (Settings.FollowingCaret ? getCurrentLine(view) : edgeLine);

		if ((down && (line >= currentLine) && (line > edgeLine)) ||
			(!down && (line <= currentLine) && (line < edgeLine)))
		{
			if (isLineVisible(view, line))
				blinkLine(view, line);
			else
				blinkLine(view, down ? getLastLine(view) : getFirstLine(view));

			// Adjust the direction of the blank annotation mark in case of selections compare corners
			if (cmpPair->options.selectionCompare && Settings.ShowOnlySelections && !down &&
					!isLineMarked(view, line, MARKER_MASK_LINE))
				down = !down;

			showBlankAdjacentArrowMark(view, line, down);

			return nextDiff;
		}
	}

	LOGD(LOG_VISIT, "Jump to " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") +
			" view, center doc line: " + std::to_string(line + 1) + "\n");

	if (cmpPair->options.findUniqueMode && Settings.FollowingCaret)
		::SetFocus(getView(view));

	// Adjust the direction of the blank annotation mark in case of selections compare corners
	if (goToCornerDiff && cmpPair->options.selectionCompare && Settings.ShowOnlySelections && !down &&
			!isLineMarked(view, line, MARKER_MASK_LINE))
		down = !down;

	if (isLineHidden(view, line))
		line = down ? getUnhiddenLine(view, line) : getPreviousUnhiddenLine(view, line);

	// Line is not visible - scroll into view
	if (!isLineVisible(view, line) || (!isLineMarked(view, line, MARKER_MASK_LINE) &&
		!isAdjacentAnnotationVisible(view, line, down) && (down || !goToCornerDiff || !isLineAnnotated(view, line))))
	{
		centerAt(view, line);
		doNotBlink = true;
	}

	CallScintilla(view, SCI_ENSUREVISIBLEENFORCEPOLICY, line, 0);

	if (Settings.FollowingCaret && line != getCurrentLine(view))
	{
		intptr_t pos;

		if (down && (isLineAnnotated(view, line) && isLineWrapped(view, line) &&
				!isLineMarked(view, line, MARKER_MASK_LINE)))
			pos = getLineEnd(view, line);
		else
			pos = getLineStart(view, line);

		CallScintilla(view, SCI_SETEMPTYSELECTION, pos, 0);

		doNotBlink = true;
	}

	if (!doNotBlink)
		blinkLine(view, line);

	showBlankAdjacentArrowMark(view, line, down);

	return std::make_pair(view, line);
}


inline std::pair<int, intptr_t> jumpToFirstChange(bool doNotBlink = false)
{
	return jumpToNextChange(0, 0, true, true, doNotBlink);
}


inline void jumpToLastChange(bool doNotBlink = false)
{
	jumpToNextChange(0, 0, false, true, doNotBlink);
}


void jumpToChange(bool down)
{
	std::pair<int, intptr_t> viewLoc;

	intptr_t mainStartLine	= 0;
	intptr_t subStartLine	= 0;

	const int currentView	= getCurrentViewId();
	const int otherView		= getOtherViewId(currentView);

	intptr_t& currentLine	= (currentView == MAIN_VIEW) ? mainStartLine : subStartLine;
	intptr_t& otherLine		= (currentView != MAIN_VIEW) ? mainStartLine : subStartLine;

	if (down)
	{
		currentLine = (Settings.FollowingCaret ? getCurrentLine(currentView) : getLastLine(currentView));

		if (Settings.FollowingCaret && isLineMarked(currentView, currentLine, MARKER_MASK_LINE) &&
			(currentLine > getLastLine(currentView)))
		{
			// Current line is marked but invisible - get into view
			centerAt(currentView, currentLine);
			return;
		}

		const bool currentLineAnnotated = isLineAnnotated(currentView, currentLine);

		if (currentLineAnnotated && isAdjacentAnnotationVisible(currentView, currentLine, down))
			++currentLine;

		otherLine = (Settings.FollowingCaret ?
				otherViewMatchingLine(currentView, currentLine) : getLastLine(otherView));

		if (!currentLineAnnotated && isLineAnnotated(otherView, otherLine))
			++otherLine;

		viewLoc = jumpToNextChange(getNextUnmarkedLine(MAIN_VIEW, mainStartLine, MARKER_MASK_LINE),
				getNextUnmarkedLine(SUB_VIEW, subStartLine, MARKER_MASK_LINE), down);
	}
	else
	{
		currentLine = (Settings.FollowingCaret ? getCurrentLine(currentView) : getFirstLine(currentView));

		if (Settings.FollowingCaret && isLineMarked(currentView, currentLine, MARKER_MASK_LINE) &&
			(currentLine < getFirstLine(currentView)))
		{
			// Current line is marked but invisible - get into view
			centerAt(currentView, currentLine);
			return;
		}

		if (isAdjacentAnnotationVisible(currentView, currentLine, down))
		{
			CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
			if (cmpPair == compareList.end())
				return;

			if (!(cmpPair->options.selectionCompare && Settings.ShowOnlySelections &&
				!isLineMarked(currentView, currentLine, MARKER_MASK_LINE) &&
				(currentLine == cmpPair->options.selections[currentView].first)))
			{
				if (!(CallScintilla(currentView, SCI_MARKERGET, currentLine - 1, 0) & MARKER_MASK_CHANGED) ||
						!(CallScintilla(otherView, SCI_MARKERGET,
						otherViewMatchingLine(currentView, currentLine) - 1, 0) & MARKER_MASK_CHANGED))
					--currentLine;

				// Special selections compare corner case
				if (cmpPair->options.selectionCompare && Settings.ShowOnlySelections &&
						!isLineMarked(currentView, currentLine, MARKER_MASK_LINE) &&
						(currentLine == cmpPair->options.selections[currentView].first))
					--currentLine;
			}
		}

		otherLine = (Settings.FollowingCaret ?
				otherViewMatchingLine(currentView, currentLine) : getFirstLine(otherView));

		viewLoc = jumpToNextChange(getPrevUnmarkedLine(MAIN_VIEW, mainStartLine, MARKER_MASK_LINE),
				getPrevUnmarkedLine(SUB_VIEW, subStartLine, MARKER_MASK_LINE), down);
	}

	if (viewLoc.first < 0)
	{
		if (Settings.WrapAround)
		{
			if (down)
				jumpToFirstChange(true);
			else
				jumpToLastChange(true);

			FLASHWINFO flashInfo;
			flashInfo.cbSize	= sizeof(flashInfo);
			flashInfo.hwnd		= nppData._nppHandle;
			flashInfo.uCount	= 3;
			flashInfo.dwTimeout	= 100;
			flashInfo.dwFlags	= FLASHW_ALL;
			::FlashWindowEx(&flashInfo);
		}
	}
}


void resetCompareView(int view)
{
	if (!::IsWindowVisible(getView(view)))
		return;

	CompareList_t::iterator cmpPair = getCompareBySciDoc(getDocId(view));
	if (cmpPair != compareList.end())
		setCompareView(view, !Settings.HideMargin, Settings.colors().blank, Settings.colors().caret_line_transparency);
}


void syncViews(int biasView)
{
	const int otherView = getOtherViewId(biasView);

	intptr_t firstVisible		= getFirstVisibleLine(biasView);
	intptr_t otherFirstVisible	= getFirstVisibleLine(otherView);

	const intptr_t endLine		= getUnhiddenLine(biasView, getEndNotEmptyLine(biasView));
	const intptr_t endVisible	= getVisibleFromDocLine(biasView, endLine) + getWrapCount(biasView, endLine);

	intptr_t otherNewFirstVisible = otherFirstVisible;

	if (firstVisible > endVisible)
	{
		firstVisible = endVisible;

		ScopedIncrementerInt incr(notificationsLock);

		CallScintilla(biasView, SCI_SETFIRSTVISIBLELINE, firstVisible, 0);
	}

	if (firstVisible != otherFirstVisible)
	{
		const intptr_t otherEndLine = getUnhiddenLine(otherView, getEndNotEmptyLine(otherView));
		const intptr_t otherEndVisible =
				getVisibleFromDocLine(otherView, otherEndLine) + getWrapCount(otherView, otherEndLine);
		const intptr_t otherLinesOnScreen = CallScintilla(otherView, SCI_LINESONSCREEN, 0, 0);

		if (firstVisible > otherEndVisible && getLineAnnotation(otherView, otherEndLine) >= otherLinesOnScreen)
		{
			if (endVisible - firstVisible < CallScintilla(biasView, SCI_LINESONSCREEN, 0, 0))
				otherNewFirstVisible = otherEndVisible + otherLinesOnScreen - (endVisible - firstVisible);
			else
				otherNewFirstVisible = otherEndVisible;
		}
		else
		{
			otherNewFirstVisible = firstVisible;
		}
	}

	if (otherNewFirstVisible != otherFirstVisible)
	{
		ScopedIncrementerInt incr(notificationsLock);

		CallScintilla(otherView, SCI_SETFIRSTVISIBLELINE, otherNewFirstVisible, 0);

		::UpdateWindow(getView(otherView));

		LOGD(LOG_SYNC, "Syncing to " + std::string(biasView == MAIN_VIEW ? "MAIN" : "SUB") +
			" view, visible doc line: " + std::to_string(getDocLineFromVisible(biasView, firstVisible) + 1) + "\n");
	}

	if (Settings.FollowingCaret && biasView == getCurrentViewId())
	{
		const intptr_t line = getCurrentLine(biasView);

		otherNewFirstVisible = otherViewMatchingLine(biasView, line);

		if ((otherNewFirstVisible != getCurrentLine(otherView)) && !isSelection(otherView))
		{
			intptr_t pos;

			if (!isLineMarked(otherView, otherNewFirstVisible, MARKER_MASK_LINE) &&
					isLineAnnotated(otherView, otherNewFirstVisible) && isLineWrapped(otherView, otherNewFirstVisible))
				pos = getLineEnd(otherView, otherNewFirstVisible);
			else
				pos = getLineStart(otherView, otherNewFirstVisible);

			ScopedIncrementerInt incr(notificationsLock);

			CallScintilla(otherView, SCI_SETEMPTYSELECTION, pos, 0);

			::UpdateWindow(getView(otherView));
		}
	}

	NavDlg.Update();
}


intptr_t getAlignmentIdxAfter(const AlignmentViewData AlignmentPair::*pView, const AlignmentInfo_t &alignInfo,
	intptr_t line)
{
	intptr_t idx = 0;

	for (intptr_t i = static_cast<intptr_t>(alignInfo.size()) / 2; i; i /= 2)
	{
		if ((alignInfo[idx + i].*pView).line < line)
			idx += i;
	}

	while ((idx < static_cast<intptr_t>(alignInfo.size())) && ((alignInfo[idx].*pView).line < line))
		++idx;

	return idx;
}


intptr_t getAlignmentLine(const AlignmentInfo_t &alignInfo, int view, intptr_t line)
{
	if (line < 0)
		return -1;

	AlignmentViewData AlignmentPair::*alignView = (view == MAIN_VIEW) ? &AlignmentPair::main : &AlignmentPair::sub;

	const intptr_t alignIdx = getAlignmentIdxAfter(alignView, alignInfo, line);

	if ((alignIdx >= static_cast<intptr_t>(alignInfo.size())) || ((alignInfo[alignIdx].*alignView).line != line))
		return -1;

	alignView = (view == MAIN_VIEW) ? &AlignmentPair::sub : &AlignmentPair::main;

	return (alignInfo[alignIdx].*alignView).line;
}


bool updateViewsHideState(CompareList_t::iterator& cmpPair)
{
	unsigned currentHideFlags = NO_HIDE;

	if (Settings.HideMatches)
		currentHideFlags |= HIDE_MATCHES;
	if (Settings.HideNewLines)
		currentHideFlags |= HIDE_NEW_LINES;
	if (Settings.HideChangedLines)
		currentHideFlags |= HIDE_CHANGED_LINES;
	if (Settings.HideMovedLines)
		currentHideFlags |= HIDE_MOVED_LINES;
	if (cmpPair->options.selectionCompare && Settings.ShowOnlySelections)
		currentHideFlags |= HIDE_OUTSIDE_SELECTIONS;

	if (cmpPair->hideFlags == currentHideFlags)
		return false;

	unhideAllLines(MAIN_VIEW);
	unhideAllLines(SUB_VIEW);

	if (cmpPair->options.ignoreHiddenLines)
	{
		hideNotepadHiddenLines(MAIN_VIEW);
		hideNotepadHiddenLines(SUB_VIEW);
	}

	if (currentHideFlags & HIDE_OUTSIDE_SELECTIONS)
	{
		hideLinesOutsideRange(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].first,
				cmpPair->options.selections[MAIN_VIEW].second);
		hideLinesOutsideRange(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].first,
				cmpPair->options.selections[SUB_VIEW].second);
	}

	int hideMarkMask = 0;

	if (Settings.HideNewLines)
		hideMarkMask |= MARKER_MASK_NEW_LINE;
	if (Settings.HideChangedLines)
		hideMarkMask |= MARKER_MASK_CHANGED_LINE;
	if (Settings.HideMovedLines)
		hideMarkMask |= MARKER_MASK_MOVED_LINE;

	if (Settings.HideMatches || hideMarkMask)
	{
		hideLines(MAIN_VIEW, hideMarkMask, Settings.HideMatches);
		hideLines(SUB_VIEW, hideMarkMask, Settings.HideMatches);
	}

	cmpPair->hideFlags = currentHideFlags;

	return true;
}


bool isAlignmentNeeded(int view, CompareList_t::iterator& cmpPair)
{
	if (updateViewsHideState(cmpPair))
	{
		CallScintilla(MAIN_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);
		CallScintilla(SUB_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);

		return true;
	}

	const AlignmentInfo_t& alignmentInfo = cmpPair->summary.alignmentInfo;

	const AlignmentViewData AlignmentPair::*pView = (view == MAIN_VIEW) ? &AlignmentPair::main : &AlignmentPair::sub;

	const intptr_t firstLine	= getFirstLine(view);
	const intptr_t lastLine		= getLastLine(view);

	const intptr_t maxSize = static_cast<intptr_t>(alignmentInfo.size());

	intptr_t i = getAlignmentIdxAfter(pView, alignmentInfo, firstLine);

	if (i >= maxSize)
		return false;

	if (i)
		--i;

	// Ignore alignment on line 0 as it is currently not supported by Scintilla
	while ((i < maxSize) && ((alignmentInfo[i].main.line == 0) || (alignmentInfo[i].sub.line == 0)))
		++i;

	if (Settings.HideMatches)
	{
		for (; i < maxSize; ++i)
		{
			if (isLineHidden(MAIN_VIEW, alignmentInfo[i].main.line) ||
				isLineHidden(SUB_VIEW, alignmentInfo[i].sub.line))
				continue;

			if ((alignmentInfo[i].main.diffMask != 0) && (alignmentInfo[i].sub.diffMask != 0) &&
					(getVisibleFromDocLine(MAIN_VIEW, alignmentInfo[i].main.line) !=
					getVisibleFromDocLine(SUB_VIEW, alignmentInfo[i].sub.line)))
				return true;

			if ((alignmentInfo[i].*pView).line > lastLine)
				return false;
		}
	}
	else
	{
		for (; i < maxSize; ++i)
		{
			if (isLineHidden(MAIN_VIEW, alignmentInfo[i].main.line) ||
				isLineHidden(SUB_VIEW, alignmentInfo[i].sub.line))
				continue;

			if ((alignmentInfo[i].main.diffMask == alignmentInfo[i].sub.diffMask) &&
					(getVisibleFromDocLine(MAIN_VIEW, alignmentInfo[i].main.line) !=
					getVisibleFromDocLine(SUB_VIEW, alignmentInfo[i].sub.line)))
				return true;

			if ((alignmentInfo[i].*pView).line > lastLine)
				return false;
		}
	}

	intptr_t mainEndLine;
	intptr_t subEndLine;

	if (!cmpPair->options.selectionCompare)
	{
		mainEndLine	= getEndNotEmptyLine(MAIN_VIEW);
		subEndLine	= getEndNotEmptyLine(SUB_VIEW);
	}
	else
	{
		mainEndLine	= cmpPair->options.selections[MAIN_VIEW].second;
		subEndLine	= cmpPair->options.selections[SUB_VIEW].second;
	}

	if (isLineHidden(MAIN_VIEW, mainEndLine))
		mainEndLine = getPreviousUnhiddenLine(MAIN_VIEW, mainEndLine);

	if (isLineHidden(SUB_VIEW, subEndLine))
		subEndLine = getPreviousUnhiddenLine(SUB_VIEW, subEndLine);

	const intptr_t mainEndVisible = getVisibleFromDocLine(MAIN_VIEW, mainEndLine) +
			getWrapCount(MAIN_VIEW, mainEndLine) - 1;
	const intptr_t subEndVisible = getVisibleFromDocLine(SUB_VIEW, subEndLine) +
			getWrapCount(SUB_VIEW, subEndLine) - 1;

	const intptr_t mismatchLen = std::abs(mainEndVisible - subEndVisible);
	const intptr_t linesOnScreen = CallScintilla(MAIN_VIEW, SCI_LINESONSCREEN, 0, 0);
	const intptr_t endMisalignment = (mismatchLen < linesOnScreen) ? mismatchLen : linesOnScreen;

	const intptr_t mainEndLineAnnotation	= getLineAnnotation(MAIN_VIEW, mainEndLine);
	const intptr_t subEndLineAnnotation		= getLineAnnotation(SUB_VIEW, subEndLine);

	if ((!cmpPair->options.selectionCompare && mainEndLineAnnotation && subEndLineAnnotation) ||
		(std::abs(mainEndLineAnnotation - subEndLineAnnotation) != endMisalignment))
		return true;

	return false;
}


void alignDiffs(CompareList_t::iterator& cmpPair)
{
	LOGD(LOG_NOTIF, "Aligning diffs\n");

	if (updateViewsHideState(cmpPair))
	{
		CallScintilla(MAIN_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);
		CallScintilla(SUB_VIEW, SCI_ANNOTATIONCLEARALL, 0, 0);
	}

	const AlignmentInfo_t& alignmentInfo = cmpPair->summary.alignmentInfo;

	const intptr_t maxSize = static_cast<intptr_t>(alignmentInfo.size());

	intptr_t mainEndLine;
	intptr_t subEndLine;

	if (!cmpPair->options.selectionCompare)
	{
		mainEndLine	= getEndNotEmptyLine(MAIN_VIEW);
		subEndLine	= getEndNotEmptyLine(SUB_VIEW);
	}
	else
	{
		mainEndLine	= cmpPair->options.selections[MAIN_VIEW].second;
		subEndLine	= cmpPair->options.selections[SUB_VIEW].second;
	}

	bool skipFirst = false;

	intptr_t i = 0;

	// Handle zero line diffs that cannot be aligned because annotation on line 0 is not supported by Scintilla
	for (; i < maxSize && alignmentInfo[i].main.line <= mainEndLine && alignmentInfo[i].sub.line <= subEndLine; ++i)
	{
		intptr_t previousUnhiddenLine = getPreviousUnhiddenLine(MAIN_VIEW, alignmentInfo[i].main.line);

		if (isLineAnnotated(MAIN_VIEW, previousUnhiddenLine))
			clearAnnotation(MAIN_VIEW, previousUnhiddenLine);

		previousUnhiddenLine = getPreviousUnhiddenLine(SUB_VIEW, alignmentInfo[i].sub.line);

		if (isLineAnnotated(SUB_VIEW, previousUnhiddenLine))
			clearAnnotation(SUB_VIEW, previousUnhiddenLine);

		if (alignmentInfo[i].main.line == 0 || alignmentInfo[i].sub.line == 0)
		{
			skipFirst = (alignmentInfo[i].main.line == alignmentInfo[i].sub.line);
			continue;
		}

		if (i == 0 || skipFirst)
			break;

		const intptr_t mismatchLen =
				getVisibleFromDocLine(MAIN_VIEW, alignmentInfo[i].main.line) -
				getVisibleFromDocLine(SUB_VIEW, alignmentInfo[i].sub.line);

		if (cmpPair->options.selectionCompare)
		{
			const intptr_t mainOffset	= (mismatchLen < 0) ? -mismatchLen : 0;
			const intptr_t subOffset	= (mismatchLen > 0) ? mismatchLen : 0;

			if (alignmentInfo[i - 1].main.line != 0)
			{
				addBlankSection(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].first, mainOffset + 1, mainOffset + 1,
						"--- Selection Compare Block Start ---");
				addBlankSection(SUB_VIEW, alignmentInfo[i].sub.line, subOffset + 1, subOffset + 1,
						"Lines above cannot be properly aligned.");
			}
			else if (alignmentInfo[i - 1].sub.line != 0)
			{
				addBlankSection(MAIN_VIEW, alignmentInfo[i].main.line, mainOffset + 1, mainOffset + 1,
						"Lines above cannot be properly aligned.");
				addBlankSection(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].first, subOffset + 1, subOffset + 1,
						"--- Selection Compare Block Start ---");
			}
			else
			{
				break;
			}
		}
		else
		{
			constexpr char lineZeroAlignInfo[] =
						"Lines above cannot be properly aligned.\n"
						"To see them aligned, please manually insert one empty line\n"
						"in the beginning of each file and then re-compare.";

			if (mismatchLen > 0)
			{
				addBlankSection(MAIN_VIEW, alignmentInfo[i].main.line, 1, 1, lineZeroAlignInfo);
				addBlankSection(SUB_VIEW, alignmentInfo[i].sub.line, mismatchLen + 1, mismatchLen + 1,
						lineZeroAlignInfo);
			}
			else if (mismatchLen < 0)
			{
				addBlankSection(MAIN_VIEW, alignmentInfo[i].main.line, -mismatchLen + 1, -mismatchLen + 1,
						lineZeroAlignInfo);
				addBlankSection(SUB_VIEW, alignmentInfo[i].sub.line, 1, 1, lineZeroAlignInfo);
			}
		}

		++i;
		break;
	}

	// Align all other diffs
	for (; i < maxSize && alignmentInfo[i].main.line <= mainEndLine && alignmentInfo[i].sub.line <= subEndLine; ++i)
	{
		intptr_t previousUnhiddenLine = getPreviousUnhiddenLine(MAIN_VIEW, alignmentInfo[i].main.line);

		if (isLineAnnotated(MAIN_VIEW, previousUnhiddenLine))
			clearAnnotation(MAIN_VIEW, previousUnhiddenLine);

		previousUnhiddenLine = getPreviousUnhiddenLine(SUB_VIEW, alignmentInfo[i].sub.line);

		if (isLineAnnotated(SUB_VIEW, previousUnhiddenLine))
			clearAnnotation(SUB_VIEW, previousUnhiddenLine);

		if (isLineHidden(MAIN_VIEW, alignmentInfo[i].main.line) || isLineHidden(SUB_VIEW, alignmentInfo[i].sub.line))
			continue;

		const intptr_t mismatchLen =
				getVisibleFromDocLine(MAIN_VIEW, alignmentInfo[i].main.line) -
				getVisibleFromDocLine(SUB_VIEW, alignmentInfo[i].sub.line);

		if (mismatchLen > 0)
		{
			if ((i + 1 < maxSize) && (alignmentInfo[i].sub.line == alignmentInfo[i + 1].sub.line))
				continue;

			addBlankSection(SUB_VIEW, alignmentInfo[i].sub.line, mismatchLen);
		}
		else if (mismatchLen < 0)
		{
			if ((i + 1 < maxSize) && (alignmentInfo[i].main.line == alignmentInfo[i + 1].main.line))
				continue;

			addBlankSection(MAIN_VIEW, alignmentInfo[i].main.line, -mismatchLen);
		}
	}

	if (isLineHidden(MAIN_VIEW, mainEndLine))
		mainEndLine = getPreviousUnhiddenLine(MAIN_VIEW, mainEndLine);

	if (isLineHidden(SUB_VIEW, subEndLine))
		subEndLine = getPreviousUnhiddenLine(SUB_VIEW, subEndLine);

	const intptr_t mainEndVisible	= getVisibleFromDocLine(MAIN_VIEW, mainEndLine) +
			getWrapCount(MAIN_VIEW, mainEndLine) - 1;
	const intptr_t subEndVisible	= getVisibleFromDocLine(SUB_VIEW, subEndLine) +
			getWrapCount(SUB_VIEW, subEndLine) - 1;

	const intptr_t mismatchLen		= mainEndVisible - subEndVisible;
	const intptr_t absMismatchLen	= std::abs(mismatchLen);
	const intptr_t linesOnScreen	= CallScintilla(MAIN_VIEW, SCI_LINESONSCREEN, 0, 0);
	const intptr_t endMisalignment	= (absMismatchLen < linesOnScreen) ? absMismatchLen : linesOnScreen;

	const intptr_t mainEndLineAnnotation	= getLineAnnotation(MAIN_VIEW, mainEndLine);
	const intptr_t subEndLineAnnotation		= getLineAnnotation(SUB_VIEW, subEndLine);

	if ((!cmpPair->options.selectionCompare && mainEndLineAnnotation && subEndLineAnnotation) ||
		(std::abs(mainEndLineAnnotation - subEndLineAnnotation) != endMisalignment))
	{
		if (mismatchLen == 0)
		{
			clearAnnotation(MAIN_VIEW, mainEndLine);
			clearAnnotation(SUB_VIEW, subEndLine);
		}
		else if (mismatchLen > 0)
		{
			clearAnnotation(MAIN_VIEW, mainEndLine);
			addBlankSectionAfter(SUB_VIEW, subEndLine, endMisalignment);
		}
		else
		{
			clearAnnotation(SUB_VIEW, subEndLine);
			addBlankSectionAfter(MAIN_VIEW, mainEndLine, endMisalignment);
		}
	}

	// Mark selections for clarity
	if (cmpPair->options.selectionCompare)
	{
		// Line zero selections are already covered
		if (cmpPair->options.selections[MAIN_VIEW].first > 0 && cmpPair->options.selections[SUB_VIEW].first > 0)
		{
			intptr_t mainAnnotation = getLineAnnotation(MAIN_VIEW,
					getPreviousUnhiddenLine(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].first));

			intptr_t subAnnotation = getLineAnnotation(SUB_VIEW,
					getPreviousUnhiddenLine(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].first));

			const intptr_t visibleBlockStartMismatch =
				getVisibleFromDocLine(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].first) -
				getVisibleFromDocLine(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].first);

			++mainAnnotation;
			++subAnnotation;

			intptr_t mainAnnotPos	= mainAnnotation;
			intptr_t subAnnotPos	= subAnnotation;

			if (visibleBlockStartMismatch > 0)
				mainAnnotPos -= visibleBlockStartMismatch;
			else if (visibleBlockStartMismatch < 0)
				subAnnotPos += visibleBlockStartMismatch;

			addBlankSection(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].first, mainAnnotation, mainAnnotPos,
					"--- Selection Compare Block Start ---");
			addBlankSection(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].first, subAnnotation, subAnnotPos,
					"--- Selection Compare Block Start ---");
		}

		{
			intptr_t mainAnnotation = getLineAnnotation(MAIN_VIEW,
					getPreviousUnhiddenLine(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].second + 1));

			intptr_t subAnnotation = getLineAnnotation(SUB_VIEW,
					getPreviousUnhiddenLine(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].second + 1));

			if (mainAnnotation == 0 || subAnnotation == 0)
			{
				++mainAnnotation;
				++subAnnotation;
			}

			addBlankSection(MAIN_VIEW, cmpPair->options.selections[MAIN_VIEW].second + 1,
					mainAnnotation, mainAnnotation, "--- Selection Compare Block End ---");
			addBlankSection(SUB_VIEW, cmpPair->options.selections[SUB_VIEW].second + 1,
					subAnnotation, subAnnotation, "--- Selection Compare Block End ---");
		}
	}
}


void doAlignment(bool forceAlign = false)
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());

	if (cmpPair == compareList.end())
		return;

	if (cmpPair->autoRecompareDelay)
	{
		delayedRecompare.post(cmpPair->autoRecompareDelay);

		return;
	}

	bool goToFirst = (firstUpdateGuardDuration.count() != 0);

	if (goToFirst)
	{
		goToFirst = (std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - prevUpdateTime) < firstUpdateGuardDuration);

		if (!goToFirst)
			firstUpdateGuardDuration = std::chrono::milliseconds(0);
	}

	bool realign = forceAlign;

	if (forceAlign)
		cmpPair->hideFlags = FORCE_REHIDING;

	if (!realign)
		realign = isAlignmentNeeded(storedLocation ? storedLocation->getView() : getCurrentViewId(), cmpPair);

	ScopedIncrementerInt incr(notificationsLock);

	if (realign)
	{
		if (!storedLocation && !goToFirst)
			storedLocation = std::make_unique<ViewLocation>(getCurrentViewId());

		alignDiffs(cmpPair);

		delayedAlign.post(300, false);
	}

	if (goToFirst)
	{
		LOGD(LOG_NOTIF, "Go to first diff\n");

		std::pair<int, intptr_t> viewLoc = jumpToFirstChange(true);

		const int view = (viewLoc.first >= 0) ? viewLoc.first : getCurrentViewId();

		if (forceAlign && (getFirstVisibleLine(MAIN_VIEW) == getFirstVisibleLine(SUB_VIEW)))
		{
			const int otherView = getOtherViewId(view);

			prevUpdateTime = std::chrono::steady_clock::now();

			// Force initial refresh of the other view
			CallScintilla(otherView, SCI_SETFIRSTVISIBLELINE, getFirstVisibleLine(otherView) + 1, 0);

			return;
		}
		else
		{
			syncViews(view);

			cmpPair->setStatus();
		}
	}
	else if (storedLocation)
	{
		if (realign)
			storedLocation->restore();

		syncViews(storedLocation->getView());

		cmpPair->setStatus();
	}
	else if (cmpPair->options.findUniqueMode)
	{
		syncViews(getCurrentViewId());
	}

	storedLocation = nullptr;

	if (cmpPair->nppReplaceDone)
	{
		cmpPair->nppReplaceDone = false;

		::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_REPLACED"].c_str(), PLUGIN_NAME, MB_OK | MB_ICONWARNING);
	}

	if (goToFirst)
		prevUpdateTime = std::chrono::steady_clock::now();
}


void showNavBar()
{
	if (!NavDlg.SetColors(Settings.colors(), isDarkMode()))
		NavDlg.Show();
}


bool isFileCompared(int view)
{
	const intptr_t sciDoc = getDocId(view);

	CompareList_t::iterator cmpPair = getCompareBySciDoc(sciDoc);
	if (cmpPair != compareList.end())
	{
		const wchar_t* fname = ::PathFindFileNameW(cmpPair->getFileBySciDoc(sciDoc).name);

		wchar_t msg[MAX_PATH];
		_snwprintf_s(msg, _countof(msg), _TRUNCATE, Strings::get()["MSG_ALREADY_COMPARED"].c_str(), fname);
		::MessageBoxW(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK);

		return true;
	}

	return false;
}


bool isEncodingOK(const ComparedPair& cmpPair)
{
	// Warn about encoding mismatches as that might compromise the compare
	if (getEncoding(cmpPair.file[0].buffId) != getEncoding(cmpPair.file[1].buffId))
	{
		if (::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_ENCODINGS"].c_str(),
			PLUGIN_NAME, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1) != IDYES)
		{
			return false;
		}
	}

	return true;
}


// Call it with no arguments when re-comparing (the files are active in both views)
bool areSelectionsValid(LRESULT currentBuffId = -1, LRESULT otherBuffId = -1)
{
	int view1 = (currentBuffId == otherBuffId) ? MAIN_VIEW : viewIdFromBuffId(currentBuffId);
	int view2 = (currentBuffId == otherBuffId) ? SUB_VIEW : viewIdFromBuffId(otherBuffId);

	if (view1 == view2)
		activateBufferID(otherBuffId);

	std::pair<intptr_t, intptr_t> viewSel = getSelectionLines(view2);
	bool valid = !(viewSel.first < 0);

	if (view1 == view2)
		activateBufferID(currentBuffId);

	if (valid)
	{
		viewSel = getSelectionLines(view1);
		valid = !(viewSel.first < 0);
	}

	if (!valid)
		::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_NO_SELECTIONS"].c_str(), PLUGIN_NAME, MB_OK);

	return valid;
}


bool setFirst(bool currFileIsNew, bool markName = false)
{
	if (isFileCompared(getCurrentViewId()))
		return false;

	// Done on purpose: First wipe the std::unique_ptr so ~NewCompare is called before the new object constructor.
	// This is important because the N++ plugin menu is updated on NewCompare construct/destruct.
	newCompare = nullptr;
	newCompare = std::make_unique<NewCompare>(currFileIsNew, markName);

	return true;
}


void setContent(const char* content)
{
	const int view = getCurrentViewId();

	ScopedViewUndoCollectionBlocker undoBlock(view);
	ScopedViewWriteEnabler writeEn(view);

	CallScintilla(view, SCI_SETTEXT, 0, (LPARAM)content);
	CallScintilla(view, SCI_SETSAVEPOINT, 0, 0);
}


bool checkFileExists(const wchar_t *file)
{
	if (::PathFileExistsW(file) == FALSE)
	{
		::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_NOT_WRITTEN"].c_str(), PLUGIN_NAME, MB_OK);
		return false;
	}

	return true;
}


bool createTempFile(const wchar_t *file, Temp_t tempType)
{
	if (!setFirst(true))
		return false;

	wchar_t tempFile[MAX_PATH];

	if (::GetTempPathW(_countof(tempFile), tempFile))
	{
		const wchar_t* fileExt = ::PathFindExtensionW(newCompare->pair.file[0].name);

		BOOL success = (tempType == CLIPBOARD_TEMP);

		if (tempType != CLIPBOARD_TEMP)
		{
			const wchar_t* fileName = ::PathFindFileNameW(newCompare->pair.file[0].name);

			success = ::PathAppendW(tempFile, fileName);

			if (success)
				::PathRemoveExtensionW(tempFile);
		}

		if (success)
		{
			wcscat_s(tempFile, _countof(tempFile), tempMark[tempType].fileMark);

			size_t idxPos = wcslen(tempFile);

			// Make sure temp file is unique
			for (int i = 1; ; ++i)
			{
				wchar_t idx[32];

				_itow_s(i, idx, _countof(idx), 10);

				if (wcslen(idx) + idxPos + 1 > _countof(tempFile))
				{
					idxPos = _countof(tempFile);
					break;
				}

				wcscat_s(tempFile, _countof(tempFile), idx);
				wcscat_s(tempFile, _countof(tempFile), fileExt);

				if (!::PathFileExistsW(tempFile))
					break;

				tempFile[idxPos] = 0;
			}

			if (idxPos + 1 <= _countof(tempFile))
			{
				if (file)
				{
					success = ::CopyFileW(file, tempFile, TRUE);

					if (success)
						::SetFileAttributesW(tempFile, FILE_ATTRIBUTE_TEMPORARY);
				}
				else
				{
					HANDLE hFile = ::CreateFileW(tempFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
							FILE_ATTRIBUTE_TEMPORARY, NULL);

					success = (hFile != INVALID_HANDLE_VALUE);

					if (success)
						::CloseHandle(hFile);
				}

				if (success)
				{
					const int langType = static_cast<int>(::SendMessageW(nppData._nppHandle, NPPM_GETBUFFERLANGTYPE,
							newCompare->pair.file[0].buffId, 0));

					const int view = getCurrentViewId();
					const int encoding = static_cast<int>(CallScintilla(view, SCI_GETCODEPAGE, 0, 0));

					ScopedIncrementerInt incr(notificationsLock);

					if (::SendMessageW(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)tempFile))
					{
						const LRESULT buffId = getCurrentBuffId();

						::SendMessageW(nppData._nppHandle, NPPM_SETBUFFERLANGTYPE, buffId, langType);
						::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_EDIT_SETREADONLY);

						CallScintilla(view, SCI_SETCODEPAGE, encoding, 0);

						newCompare->pair.file[1].isTemp = tempType;

						return true;
					}
				}
			}
		}
	}

	::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_TEMP_FAIL"].c_str(), PLUGIN_NAME, MB_OK);

	newCompare = nullptr;

	return false;
}


void clearComparePair(LRESULT buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	ScopedIncrementerInt incr(notificationsLock);

	cmpPair->restoreFiles(buffId);

	compareList.erase(cmpPair);

	onBufferActivated(getCurrentBuffId());
}


void closeComparePair(CompareList_t::iterator cmpPair)
{
	HWND currentView = getCurrentView();

	ScopedIncrementerInt incr(notificationsLock);

	// First close the file in the SUB_VIEW as closing a file may lead to a single view mode
	// and if that happens we want to be in single main view
	cmpPair->getFileByViewId(SUB_VIEW).close();
	cmpPair->getFileByViewId(MAIN_VIEW).close();

	compareList.erase(cmpPair);

	if (::IsWindowVisible(currentView))
		::SetFocus(currentView);

	onBufferActivated(getCurrentBuffId());
}


bool initNewCompare()
{
	bool firstIsSet = (bool)newCompare;

	// Compare to self?
	if (firstIsSet && (newCompare->pair.file[0].buffId == getCurrentBuffId()))
		firstIsSet = false;

	if (!firstIsSet)
	{
		const bool singleView = isSingleView();
		const bool isNew = singleView ? Settings.FirstFileIsNew : getCurrentViewId() == Settings.NewFileViewId;

		if (!setFirst(isNew))
			return false;

		if (singleView)
		{
			if (getNumberOfFiles(getCurrentViewId()) < 2)
			{
				::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_ONLY_ONE"].c_str(), PLUGIN_NAME, MB_OK);
				return false;
			}

			::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0,
					Settings.CompareToPrev ? IDM_VIEW_TAB_PREV : IDM_VIEW_TAB_NEXT);
		}
		else
		{
			// Check if the file in the other view is compared already
			if (isFileCompared(getOtherViewId()))
				return false;

			// Check if comparing to cloned self
			if (getDocId(MAIN_VIEW) == getDocId(SUB_VIEW))
			{
				::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_COMPARE_TO_CLONE"].c_str(), PLUGIN_NAME, MB_OK);
				return false;
			}

			::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_SWITCHTO_OTHER_VIEW);
		}
	}

	newCompare->pair.file[1].initFromCurrent(!newCompare->pair.file[0].isNew);

	return true;
}


CompareList_t::iterator addComparePair()
{
	compareList.push_back(std::move(newCompare->pair));
	newCompare = nullptr;

	return compareList.end() - 1;
}


bool setupCompare(CompareList_t::iterator& cmpPair, bool selectionCompare, bool findUniqueMode, bool recompare,
	bool recompareSameSelections)
{
	cmpPair->options.newFileViewId	= Settings.NewFileViewId;
	cmpPair->options.findUniqueMode	= findUniqueMode;

	cmpPair->options.neverMarkIgnored	 = Settings.NeverMarkIgnored;
	cmpPair->options.detectMoves		 = Settings.DetectMoves && !findUniqueMode;
	cmpPair->options.detectSubBlockDiffs = Settings.DetectSubBlockDiffs && !findUniqueMode;
	cmpPair->options.detectSubLineMoves	 = Settings.DetectSubLineMoves && cmpPair->options.detectSubBlockDiffs;
	cmpPair->options.detectCharDiffs	 = Settings.DetectCharDiffs && cmpPair->options.detectSubBlockDiffs;
	cmpPair->options.ignoreEmptyLines	 = Settings.IgnoreEmptyLines;
	cmpPair->options.ignoreFoldedLines	 = Settings.IgnoreFoldedLines;
	cmpPair->options.ignoreHiddenLines	 = Settings.IgnoreHiddenLines;
	cmpPair->options.ignoreChangedSpaces = Settings.IgnoreChangedSpaces;
	cmpPair->options.ignoreAllSpaces	 = Settings.IgnoreAllSpaces;
	cmpPair->options.ignoreEOL			 = Settings.IgnoreEOL || cmpPair->forcedIgnoreEOL;
	cmpPair->options.ignoreCase			 = Settings.IgnoreCase;
	cmpPair->options.bookmarksAsSync	 = Settings.BookmarksAsSync && !cmpPair->forcedNoManualSync && !findUniqueMode;
	cmpPair->options.recompareOnChange	 = Settings.RecompareOnChange;

	if (Settings.IgnoreRegex)
		cmpPair->options.setIgnoreRegex(Settings.IgnoreRegexStr[0],
				Settings.InvertRegex, Settings.InclRegexNomatchLines, Settings.HighlightRegexIgnores,
				Settings.IgnoreCase);
	else
		cmpPair->options.clearIgnoreRegex();

	cmpPair->options.changedResemblPercent	= Settings.ChangedResemblPercent;
	cmpPair->options.selectionCompare		= selectionCompare;

	cmpPair->positionFiles(recompare);

	// Re-get selections
	if (selectionCompare && !recompareSameSelections)
	{
		if (cmpPair->getOldFile().isTemp != CLIPBOARD_TEMP)
		{
			cmpPair->options.selections[MAIN_VIEW]	= getSelectionLines(MAIN_VIEW);
			cmpPair->options.selections[SUB_VIEW]	= getSelectionLines(SUB_VIEW);
		}
		else
		{
			const int newView = cmpPair->getNewFile().compareViewId;
			const int tmpView = cmpPair->getOldFile().compareViewId;

			cmpPair->options.selections[newView] = getSelectionLines(newView);
			cmpPair->options.selections[tmpView] = std::make_pair(1, getEndNotEmptyLine(tmpView));
		}

		if (cmpPair->options.selections[MAIN_VIEW].first > cmpPair->options.selections[MAIN_VIEW].second ||
			cmpPair->options.selections[SUB_VIEW].first > cmpPair->options.selections[SUB_VIEW].second)
			return false;
	}

	cmpPair->options.syncPoints.clear();

	if (cmpPair->options.bookmarksAsSync)
	{
		intptr_t bookmark1 = -1;
		intptr_t bookmark2 = -1;

		intptr_t endLine1 = getLinesCount(MAIN_VIEW) - 1;
		intptr_t endLine2 = getLinesCount(SUB_VIEW) - 1;

		if (selectionCompare)
		{
			bookmark1 = cmpPair->options.selections[MAIN_VIEW].first;
			bookmark2 = cmpPair->options.selections[SUB_VIEW].first;

			endLine1 = cmpPair->options.selections[MAIN_VIEW].second;
			endLine2 = cmpPair->options.selections[SUB_VIEW].second;
		}

		while (true)
		{
			bookmark1 = getNextBookmarkedLine(MAIN_VIEW, bookmark1 + 1);
			if (bookmark1 < 0 || bookmark1 > endLine1)
				break;

			bookmark2 = getNextBookmarkedLine(SUB_VIEW, bookmark2 + 1);
			if (bookmark2 < 0 || bookmark2 > endLine2)
				break;

			cmpPair->options.syncPoints.emplace_back(std::make_pair(bookmark1, bookmark2));
		}

		cmpPair->options.bookmarksAsSync = !cmpPair->options.syncPoints.empty();
	}

	cmpPair->hideFlags = NO_HIDE;

	// New compare?
	if (!recompare)
	{
		if (Settings.SizesCheck)
		{
			constexpr int cLinesCountWarningLimit = 50000;

			bool largeFilesWarning = false;

			if (selectionCompare)
				largeFilesWarning =
					(cmpPair->options.selections[MAIN_VIEW].second -
					cmpPair->options.selections[MAIN_VIEW].first + 1 > cLinesCountWarningLimit) &&
					(cmpPair->options.selections[SUB_VIEW].second -
					cmpPair->options.selections[SUB_VIEW].first + 1 > cLinesCountWarningLimit);
			else
				largeFilesWarning =
					(getLinesCount(MAIN_VIEW) > cLinesCountWarningLimit) &&
					(getLinesCount(SUB_VIEW) > cLinesCountWarningLimit);

			if (largeFilesWarning)
			{
				if (::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_LARGE_FILES"].c_str(),
						PLUGIN_NAME, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1) != IDYES)
				{
					return false;
				}
			}
		}

		if ((cmpPair->getOldFile().isTemp != CLIPBOARD_TEMP) &&
			(CallScintilla(MAIN_VIEW, SCI_GETEOLMODE, 0, 0) != CallScintilla(SUB_VIEW, SCI_GETEOLMODE, 0, 0)) &&
			!cmpPair->options.ignoreEOL)
		{
			if (::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_EOL_DIFFERENT"].c_str(),
					cmpPair->options.findUniqueMode ?
					Strings::get()["STATUS_FIND_UNIQUE"].c_str() : Strings::get()["STATUS_COMPARE"].c_str(),
					MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1) == IDYES)
			{
				cmpPair->options.ignoreEOL = true;
				cmpPair->forcedIgnoreEOL = true;
			}
		}

		if (cmpPair->options.bookmarksAsSync && Settings.ManualSyncCheck)
		{
			if (::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_MANUAL_SYNC"].c_str(),
					Strings::get()["STATUS_COMPARE"].c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1) == IDYES)
			{
				cmpPair->options.bookmarksAsSync = false;
				cmpPair->options.syncPoints.clear();
				cmpPair->forcedNoManualSync = true;
			}
		}
	}

	return true;
}


inline bool compareSHAs(const CompareList_t::iterator& cmpPair)
{
    bool filesSHAsDiffer = true;

    const std::vector<uint8_t> mainComparedContentSHA2 =
        cmpPair->options.selectionCompare ? generateContentsSha256(MAIN_VIEW,
        cmpPair->options.selections[MAIN_VIEW].first, cmpPair->options.selections[MAIN_VIEW].second) :
        generateContentsSha256(MAIN_VIEW);

    const std::vector<uint8_t> subComparedContentSHA2 =
        cmpPair->options.selectionCompare ? generateContentsSha256(SUB_VIEW,
        cmpPair->options.selections[SUB_VIEW].first, cmpPair->options.selections[SUB_VIEW].second) :
        generateContentsSha256(SUB_VIEW);

    if (mainComparedContentSHA2.size() == subComparedContentSHA2.size())
        filesSHAsDiffer = !std::equal(
                mainComparedContentSHA2.begin(), mainComparedContentSHA2.end(),
                subComparedContentSHA2.begin(), subComparedContentSHA2.end());

    return filesSHAsDiffer;
}


CompareResult runCompare(CompareList_t::iterator& cmpPair)
{
	setStyles(Settings);

	const wchar_t* newName = ::PathFindFileNameW(cmpPair->getNewFile().name);
	const wchar_t* oldName = ::PathFindFileNameW(cmpPair->getOldFile().name);

	wchar_t progressInfo[MAX_PATH];
	_snwprintf_s(progressInfo, _countof(progressInfo), _TRUNCATE, cmpPair->options.selectionCompare ?
			Strings::get()["MSG_SEL_COMPARING"].c_str() : Strings::get()["MSG_COMPARING"].c_str(), newName, oldName);

	return compareViews(cmpPair->options, progressInfo, cmpPair->summary);
}


void compare(bool selectionCompare = false, bool findUniqueMode = false, bool autoUpdating = false)
{
	delayedRecompare.cancel();

	ScopedIncrementerInt incr(notificationsLock);

	// Just to be sure any old state is cleared
	firstUpdateGuardDuration = std::chrono::milliseconds(0);
	storedLocation = nullptr;
	copiedSectionMarks.clear();

	temporaryRangeSelect(-1);
	setArrowMark(-1);

	const bool				doubleView		= !isSingleView();
	const LRESULT			currentBuffId	= getCurrentBuffId();
	CompareList_t::iterator	cmpPair			= getCompare(currentBuffId);
	const bool				recompare		= (cmpPair != compareList.end());

	bool recompareSameSelections = false;

	if (recompare)
	{
		newCompare = nullptr;

		cmpPair->autoRecompareDelay = 0;

		if (!autoUpdating && selectionCompare)
		{
			bool checkSelections = false;

			// New selections to compare - validate them
			if (isSelection(MAIN_VIEW) && isSelection(SUB_VIEW))
			{
				checkSelections = true;
			}
			else if (isSelection(MAIN_VIEW) && (cmpPair->options.selections[SUB_VIEW].first != -1))
			{
				std::pair<intptr_t, intptr_t> newSelection = getSelectionLines(MAIN_VIEW);
				checkSelections = (newSelection.first == -1);

				if (!checkSelections)
					cmpPair->options.selections[MAIN_VIEW] = newSelection;

				recompareSameSelections = true;
			}
			else if (isSelection(SUB_VIEW) && (cmpPair->options.selections[MAIN_VIEW].first != -1))
			{
				std::pair<intptr_t, intptr_t> newSelection = getSelectionLines(SUB_VIEW);
				checkSelections = (newSelection.first == -1);

				if (!checkSelections)
					cmpPair->options.selections[SUB_VIEW] = newSelection;

				recompareSameSelections = true;
			}
			else
			{
				if ((cmpPair->options.selections[MAIN_VIEW].first == -1) ||
						(cmpPair->options.selections[SUB_VIEW].first == -1))
					checkSelections = true;

				recompareSameSelections = true;
			}

			if (checkSelections && !areSelectionsValid())
				return;
		}

		if ((!Settings.GotoFirstDiff && !selectionCompare) || autoUpdating)
			storedLocation = std::make_unique<ViewLocation>(getCurrentViewId());

		cmpPair->getOldFile().clear(autoUpdating);
		cmpPair->getNewFile().clear(autoUpdating);

		if (cmpPair->hideFlags != NO_HIDE)
		{
			unhideAllLines(MAIN_VIEW);
			unhideAllLines(SUB_VIEW);

			cmpPair->hideFlags = NO_HIDE;
		}
	}
	// New compare
	else
	{
		if (!initNewCompare())
		{
			newCompare = nullptr;
			return;
		}

		cmpPair = addComparePair();

		if (cmpPair->getOldFile().isTemp)
		{
			activateBufferID(cmpPair->getNewFile().buffId);

			if (cmpPair->getOldFile().isTemp == CLIPBOARD_TEMP)
			{
				const int currentView = getCurrentViewId();

				if (selectionCompare && (isSelectionVertical(currentView) || isMultiSelection(currentView)))
					selectionCompare = false;
			}
		}
		else
		{
			activateBufferID(currentBuffId);

			if (selectionCompare &&
				!areSelectionsValid(currentBuffId, cmpPair->getOtherFileByBuffId(currentBuffId).buffId))
			{
				compareList.erase(cmpPair);
				return;
			}
		}

		if (Settings.EncodingsCheck && !isEncodingOK(*cmpPair))
		{
			clearComparePair(getCurrentBuffId());
			return;
		}
	}

	// Compare is triggered manually - get/re-get compare settings and position/reposition files
	if (!autoUpdating)
	{
		if (!setupCompare(cmpPair, selectionCompare, findUniqueMode, recompare, recompareSameSelections))
		{
			clearComparePair(getCurrentBuffId());
			return;
		}
	}

	const auto compareStartTime = std::chrono::steady_clock::now();

    // Don't compute views SHAs if recomparing - they most probably differ
	bool filesSHAsDiffer = recompare ? true : compareSHAs(cmpPair);

	const CompareResult cmpResult = filesSHAsDiffer ? runCompare(cmpPair) : CompareResult::COMPARE_MATCH;

	cmpPair->compareDirty		= false;
	cmpPair->nppReplaceDone		= false;
	cmpPair->manuallyChanged	= false;

	const auto compareDuration =
		std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - compareStartTime);

	LOGD(LOG_ALL, "COMPARE took " + std::to_string(compareDuration.count()) + " milliseconds\n");

	switch (cmpResult)
	{
		case CompareResult::COMPARE_MISMATCH:
		{
			// Honour 'Auto Re-compare On Change' user setting only if compare time is less than 5 sec.
			cmpPair->options.recompareOnChange = cmpPair->options.recompareOnChange && (compareDuration.count() < 5000);

			if (Settings.ShowNavBar)
				showNavBar();

			NppState::get().setCompareMode(true);

			setCompareView(MAIN_VIEW, !Settings.HideMargin,
					Settings.colors().blank, Settings.colors().caret_line_transparency);
			setCompareView(SUB_VIEW, !Settings.HideMargin,
					Settings.colors().blank, Settings.colors().caret_line_transparency);

			if (recompare)
			{
				updateViewsHideState(cmpPair);
				cmpPair->setStatus();
			}

			if (!storedLocation)
			{
				if (!doubleView)
					activateBufferID(cmpPair->getNewFile().buffId);

				if (selectionCompare)
				{
					clearSelection(getCurrentViewId());
					clearSelection(getOtherViewId());
				}

				firstUpdateGuardDuration = std::chrono::milliseconds(300);
				prevUpdateTime = std::chrono::steady_clock::now();

				doAlignment(true);
			}

			currentlyActiveBuffID = getCurrentBuffId();

			LOGD(LOG_ALL, "COMPARE READY\n");
		}
		return;

		case CompareResult::COMPARE_MATCH:
		{
			const auto& str = Strings::get();

            // Files match when recompared - check if they fully match or because of active ignored options
            if (recompare)
                filesSHAsDiffer = compareSHAs(cmpPair);

			const ComparedFile& oldFile = cmpPair->getOldFile();

			const wchar_t* newName = ::PathFindFileNameW(cmpPair->getNewFile().name);

			wchar_t msg[2 * MAX_PATH + 512];

			int choice = IDNO;

			if (oldFile.isTemp)
			{
				if (recompare)
				{
					if (selectionCompare)
					{
						if (cmpPair->options.findUniqueMode)
							_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_SEL_NO_UNIQUE"].c_str(),
									newName, ::PathFindFileNameW(oldFile.name));
						else
							_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_SEL_MATCH"].c_str(),
									newName, ::PathFindFileNameW(oldFile.name));
					}
					else
					{
						if (cmpPair->options.findUniqueMode)
							_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_NO_UNIQUE"].c_str(),
									newName, ::PathFindFileNameW(oldFile.name));
						else
							_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_MATCH"].c_str(),
									newName, ::PathFindFileNameW(oldFile.name));
					}

					wcscat_s(msg, _countof(msg), str["MSG_IGNORED_DIFFS"].c_str());
				}
				else
				{
					if (oldFile.isTemp == LAST_SAVED_TEMP)
					{
						_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_NOT_MODIFIED"].c_str(), newName);
					}
					else if (oldFile.isTemp == CLIPBOARD_TEMP)
					{
						if (selectionCompare)
							_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_SEL_CLIPBOARD_MATCH"].c_str(),
										newName);
						else
							_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_CLIPBOARD_MATCH"].c_str(), newName);
					}
					else
					{
						if (oldFile.isTemp == GIT_TEMP)
							_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_GIT_MATCH"].c_str(), newName);
						else
							_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_SVN_MATCH"].c_str(), newName);
					}
				}

				if (filesSHAsDiffer)
					wcscat_s(msg, _countof(msg), str["MSG_IGNORED_DIFFS"].c_str());

				::MessageBoxW(nppData._nppHandle, msg, cmpPair->options.findUniqueMode ?
						str["STATUS_FIND_UNIQUE"].c_str() : str["STATUS_COMPARE"].c_str(), MB_OK);
			}
			else
			{
				if (selectionCompare)
				{
					if (cmpPair->options.findUniqueMode)
						_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_SEL_NO_UNIQUE"].c_str(),
								newName, ::PathFindFileNameW(oldFile.name));
					else
						_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_SEL_MATCH"].c_str(),
								newName, ::PathFindFileNameW(oldFile.name));
				}
				else
				{
					if (cmpPair->options.findUniqueMode)
						_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_NO_UNIQUE"].c_str(),
								newName, ::PathFindFileNameW(oldFile.name));
					else
						_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_MATCH"].c_str(),
								newName, ::PathFindFileNameW(oldFile.name));
				}

				if (filesSHAsDiffer)
					wcscat_s(msg, _countof(msg), str["MSG_IGNORED_DIFFS"].c_str());

				if (Settings.PromptToCloseOnMatch)
				{
					wcscat_s(msg, _countof(msg), str["MSG_PROMPT_CLOSE"].c_str());

					choice = ::MessageBoxW(nppData._nppHandle, msg, cmpPair->options.findUniqueMode ?
										str["STATUS_FIND_UNIQUE"].c_str() : str["STATUS_COMPARE"].c_str(),
										MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
				}
				else
				{
					::MessageBoxW(nppData._nppHandle, msg, cmpPair->options.findUniqueMode ?
								str["STATUS_FIND_UNIQUE"].c_str() : str["STATUS_COMPARE"].c_str(), MB_OK);
				}
			}

			if (choice == IDYES)
				closeComparePair(cmpPair);
			else
				clearComparePair(getCurrentBuffId());
		}
		break;

		case CompareResult::COMPARE_ERROR:
			::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_COMPARE_FAIL"].c_str(),
						PLUGIN_NAME, MB_OK | MB_ICONERROR);

		default:
			clearComparePair(getCurrentBuffId());
	}

	storedLocation = nullptr;
}


void SetAsFirst()
{
	if (!setFirst(Settings.FirstFileIsNew, true))
		newCompare = nullptr;
}


void CompareWhole()
{
	compare();
}


void CompareSelections()
{
	compare(true);
}


void FindUnique()
{
	compare(false, true);
}


void FindSelectionsUnique()
{
	compare(true, true);
}


void LastSaveDiff()
{
	wchar_t file[MAX_PATH];

	::SendMessageW(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	if (!checkFileExists(file))
		return;

	if (createTempFile(file, LAST_SAVED_TEMP))
		compare();
}


void ClipboardDiff()
{
	const int view = getCurrentViewId();

	if (CallScintilla(view, SCI_GETLENGTH, 0, 0) == 0)
	{
		wchar_t file[MAX_PATH];

		::SendMessageW(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

		if (::PathFileExistsW(file) == FALSE)
		{
			::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_FILE_EMPTY"].c_str(), PLUGIN_NAME, MB_OK);
			return;
		}
	}

	const bool isSel = isSelection(view);

	std::vector<wchar_t> content = getFromClipboard(isSel);

	if (content.empty())
	{
		::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_CLIPBOARD_EMPTY"].c_str(), PLUGIN_NAME, MB_OK);
		return;
	}

	if (!createTempFile(nullptr, CLIPBOARD_TEMP))
		return;

	setContent(WCtoMB(content.data(), static_cast<int>(content.size())).c_str());
	content.clear();

	compare(isSel);
}


void SvnDiff()
{
	wchar_t file[MAX_PATH];
	wchar_t svnFile[MAX_PATH];

	::SendMessageW(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	if (!checkFileExists(file))
		return;

	if (!GetSvnFile(file, svnFile, _countof(svnFile)))
		return;

	if (createTempFile(svnFile, SVN_TEMP))
		compare();
}


void GitDiff()
{
	wchar_t file[MAX_PATH];

	::SendMessageW(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, _countof(file), (LPARAM)file);

	if (!checkFileExists(file))
		return;

	std::vector<char> content = GetGitFileContent(file);

	if (content.empty())
		return;

	if (!createTempFile(nullptr, GIT_TEMP))
		return;

	setContent(content.data());
	content.clear();

	compare();
}


void ClearActiveCompare()
{
	newCompare = nullptr;

	if (NppState::get().compareMode)
		clearComparePair(getCurrentBuffId());
}


void ClearAllCompares()
{
	newCompare = nullptr;

	if (!compareList.size())
		return;

	const LRESULT buffId = getCurrentBuffId();

	ScopedIncrementerInt incr(notificationsLock);

	::SetFocus(getOtherView());

	const LRESULT otherBuffId = getCurrentBuffId();

	for (int i = static_cast<int>(compareList.size()) - 1; i >= 0; --i)
		compareList[i].restoreFiles();

	compareList.clear();

	NppState::get().setNormalMode(true);

	if (!isSingleView())
		activateBufferID(otherBuffId);

	activateBufferID(buffId);
}


void First()
{
	if (NppState::get().compareMode)
	{
		ScopedIncrementerInt incr(notificationsLock);

		jumpToFirstChange();
	}
}


void Prev()
{
	if (NppState::get().compareMode)
	{
		ScopedIncrementerInt incr(notificationsLock);

		jumpToChange(false);
	}
}


void Next()
{
	if (NppState::get().compareMode)
	{
		ScopedIncrementerInt incr(notificationsLock);

		jumpToChange(true);
	}
}


void Last()
{
	if (NppState::get().compareMode)
	{
		ScopedIncrementerInt incr(notificationsLock);

		jumpToLastChange();
	}
}


void PrevChangePos()
{
	if (NppState::get().compareMode)
	{
		ScopedIncrementerInt incr(notificationsLock);

		const int viewId = getCurrentViewId();
		const intptr_t line = getCurrentLine(viewId);
		const intptr_t currentPos = CallScintilla(viewId, SCI_GETCURRENTPOS, 0, 0);
		const intptr_t pos = getIndicatorStartPos(viewId, currentPos - 1);

		if (pos)
		{
			if (line == CallScintilla(viewId, SCI_LINEFROMPOSITION, pos, 0))
				CallScintilla(viewId, SCI_GOTOPOS, pos, 0);
			else
				blinkLine(viewId, line);
		}
		else
		{
			blinkLine(viewId, line);
		}
	}
}


void NextChangePos()
{
	if (NppState::get().compareMode)
	{
		ScopedIncrementerInt incr(notificationsLock);

		const int viewId = getCurrentViewId();
		const intptr_t line = getCurrentLine(viewId);
		const intptr_t currentPos = CallScintilla(viewId, SCI_GETCURRENTPOS, 0, 0);
		const intptr_t pos = getIndicatorEndPos(viewId, currentPos);

		if (pos)
		{
			if (line == CallScintilla(viewId, SCI_LINEFROMPOSITION, pos, 0))
				CallScintilla(viewId, SCI_GOTOPOS, pos, 0);
			else
				blinkLine(viewId, line);
		}
		else
		{
			blinkLine(viewId, line);
		}
	}
}


void ActiveCompareSummary()
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return;

	::MessageBoxW(nppData._nppHandle, cmpPair->getSummary().c_str(), PLUGIN_NAME, MB_OK);
}


// First line can never be hidden due to Scintilla limitations so explicitly check if it should be hidden
bool shouldFirstLineBeHidden(int view)
{
	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end() || view > 1)
		return false;

	return (
		((cmpPair->hideFlags & HIDE_MATCHES) && !isLineMarked(view, 0, MARKER_MASK_LINE)) ||
		((cmpPair->hideFlags & HIDE_NEW_LINES) && isLineMarked(view, 0, MARKER_MASK_NEW_LINE)) ||
		((cmpPair->hideFlags & HIDE_CHANGED_LINES) && isLineMarked(view, 0, MARKER_MASK_CHANGED_LINE)) ||
		((cmpPair->hideFlags & HIDE_MOVED_LINES) && isLineMarked(view, 0, MARKER_MASK_MOVED_LINE)) ||
		(cmpPair->options.selectionCompare && (cmpPair->hideFlags & HIDE_OUTSIDE_SELECTIONS) &&
			cmpPair->options.selections[view].first > 0)
	);
}


void CopyVisibleLines()
{
	const int view = getCurrentViewId();
	const std::vector<intptr_t> lines = getVisibleLines(view, shouldFirstLineBeHidden(view));

	if (lines.empty())
		return;

	const int codepage = getCodepage(view);

	std::vector<wchar_t> txt;

	for (auto l: lines)
	{
		const auto lineTxt	= getLineText(view, l, true);
		const auto wLineTxt	= MBtoWC(lineTxt.data(), static_cast<int>(lineTxt.size()), codepage);
		txt.insert(txt.end(), wLineTxt.begin(), wLineTxt.end());
	}

	txt.emplace_back(L'\0');

	setToClipboard(txt);
}


void DeleteVisibleLines()
{
	const int view = getCurrentViewId();
	const std::vector<intptr_t> lines = getVisibleLines(view, shouldFirstLineBeHidden(view));

	if (lines.empty())
		return;

	ScopedViewUndoAction scopedUndo(view);

	for (intptr_t i = lines.size() - 1; i >= 0; --i)
	{
		const intptr_t	endLine		= lines[i];
		intptr_t		startLine	= endLine;

		for (--i, --startLine; i >= 0 && startLine == lines[i]; --i, --startLine);

		++i;
		++startLine;

		deleteRange(view, getLineStart(view, startLine),
				getLineStart(view, endLine) + CallScintilla(view, SCI_LINELENGTH, endLine, 0));
	}
}


void BookmarkVisibleLines()
{
	const int view = getCurrentViewId();
	const std::vector<intptr_t> lines = getVisibleLines(view, shouldFirstLineBeHidden(view));

	if (lines.empty())
		return;

	for (auto l: lines)
		bookmarkLine(view, l);
}


void formatAndWritePatch(ComparedPair& cmpPair, std::ofstream& patchFile, int matchContextLen)
{
	const auto& oldFile = cmpPair.getOldFile();
	const auto& newFile = cmpPair.getNewFile();

	const auto sciEOL = CallScintilla(oldFile.compareViewId, SCI_GETEOLMODE, 0, 0);

	const char* eol = (sciEOL == SC_EOL_CRLF ? "\r\n" : (sciEOL == SC_EOL_LF ? "\n" : "\r"));

	// Write file names with least possible paths information to distinguish them
	{
		const int oldLen = static_cast<int>(wcslen(oldFile.name));
		const int newLen = static_cast<int>(wcslen(newFile.name));
		int oldPos = oldLen ? oldLen - 1 : 0;
		int newPos = newLen ? newLen - 1 : 0;

		for (; oldPos && newPos && oldFile.name[oldPos] == newFile.name[newPos]; --oldPos, --newPos);

		for (; oldPos && oldFile.name[oldPos] != L'/' && oldFile.name[oldPos] != L'\\'; --oldPos);

		if (oldPos)
		{
			for (; newPos && newFile.name[newPos] != L'/' && newFile.name[newPos] != L'\\'; --newPos);

			if (!newPos)
				oldPos = 0;
		}
		else
		{
			newPos = 0;
		}

		if (oldPos) ++oldPos;
		if (newPos) ++newPos;

		patchFile << "--- " << WCtoMB(&oldFile.name[oldPos], oldLen - oldPos) << eol;
		patchFile << "+++ " << WCtoMB(&newFile.name[newPos], newLen - newPos);
	}

	intptr_t line1 = cmpPair.options.selectionCompare ? cmpPair.options.selections[0].first : 0;
	intptr_t line2 = cmpPair.options.selectionCompare ? cmpPair.options.selections[1].first : 0;
	intptr_t len1 = 0;
	intptr_t len2 = 0;

	const bool oldIs1 = (cmpPair.summary.diff1view == oldFile.compareViewId);

	const auto& rFile1 = oldIs1 ? oldFile : newFile;
	const auto& rFile2 = oldIs1 ? newFile : oldFile;

	const intptr_t endLine1 = getEndLine(rFile1.compareViewId);
	const intptr_t endLine2 = getEndLine(rFile2.compareViewId);

	intptr_t& rOldLine	= oldIs1 ? line1 : line2;
	intptr_t& rOldLen	= oldIs1 ? len1 : len2;
	intptr_t& rNewLine	= oldIs1 ? line2 : line1;
	intptr_t& rNewLen	= oldIs1 ? len2 : len1;

	const char diffMark1 = oldIs1 ? '-' : '+';
	const char diffMark2 = oldIs1 ? '+' : '-';

	for (auto dsi = cmpPair.summary.diffSections.begin(); dsi != cmpPair.summary.diffSections.end();)
	{
		intptr_t matchContextStart	= 0;
		intptr_t matchContextEnd	= 0;

		len1 = 0;
		len2 = 0;

		// Calculate current diff sections lines and lengths
		if (dsi->type == MATCH)
		{
			matchContextStart = (dsi->sec1.len < matchContextLen ? dsi->sec1.len : matchContextLen);

			line1 += dsi->sec1.len - matchContextStart;
			line2 += dsi->sec2.len - matchContextStart;
			len1 = matchContextStart;
			len2 = matchContextStart;
		}
		else if (dsi->type == IN_1)
		{
			line1 = dsi->sec1.off;
			len1 = dsi->sec1.len;
		}
		else
		{
			line2 = dsi->sec2.off;
			len2 = dsi->sec2.len;
		}

		auto dsn = dsi + 1;

		for (; dsn != cmpPair.summary.diffSections.end(); dsn++)
		{
			if (dsn->type == MATCH)
			{
				if (dsn->sec1.len > 2 * matchContextLen)
				{
					matchContextEnd = matchContextLen;

					len1 += matchContextEnd;
					len2 += matchContextEnd;

					break;
				}

				if (dsn + 1 == cmpPair.summary.diffSections.end())
				{
					matchContextEnd = dsn->sec1.len < matchContextLen ? dsn->sec1.len : matchContextLen;

					len1 += matchContextEnd;
					len2 += matchContextEnd;

					break;
				}

				len1 += dsn->sec1.len;
				len2 += dsn->sec2.len;
			}
			else if (dsn->type == IN_1)
			{
				len1 += dsn->sec1.len;
			}
			else
			{
				len2 += dsn->sec2.len;
			}
		}

		patchFile << eol << "@@ -" << rOldLine + (rOldLen ? 1 : 0) << ',' << rOldLen <<
								" +" << rNewLine + (rNewLen ? 1 : 0) << ',' << rNewLen << " @@";

		// Write current diff sections and context
		for (auto dsr = dsi; dsr != dsn; dsr++)
		{
			if (dsr->type == MATCH)
			{
				if (!matchContextStart && dsr != dsi)
					matchContextStart = dsr->sec1.len;

				rNewLine += matchContextStart;

				for (; matchContextStart; --matchContextStart)
				{
					patchFile << eol << ' ';
					const auto txt = getLineText(oldFile.compareViewId, rOldLine++);
					patchFile.write(txt.data(), txt.size());
				}
			}
			else if (dsr->type == IN_1)
			{
				auto dsrn = dsr + 1;

				// Replacement (changed) diff type - put old diff lines first
				if (dsrn != dsn && dsrn->type == IN_2)
				{
					if (oldIs1)
					{
						for (intptr_t i = dsr->sec1.len; i; --i)
						{
							patchFile << eol << diffMark1;
							const auto txt = getLineText(rFile1.compareViewId, line1++);
							patchFile.write(txt.data(), txt.size());
						}

						if (line1 > endLine1)
							patchFile << eol << "\\ No newline at end of file";

						for (intptr_t i = dsrn->sec2.len; i; --i)
						{
							patchFile << eol << diffMark2;
							const auto txt = getLineText(rFile2.compareViewId, line2++);
							patchFile.write(txt.data(), txt.size());
						}

						if (line2 > endLine2)
							patchFile << eol << "\\ No newline at end of file";
					}
					else
					{
						for (intptr_t i = dsrn->sec2.len; i; --i)
						{
							patchFile << eol << diffMark2;
							const auto txt = getLineText(rFile2.compareViewId, line2++);
							patchFile.write(txt.data(), txt.size());
						}

						if (line2 > endLine2)
							patchFile << eol << "\\ No newline at end of file";

						for (intptr_t i = dsr->sec1.len; i; --i)
						{
							patchFile << eol << diffMark1;
							const auto txt = getLineText(rFile1.compareViewId, line1++);
							patchFile.write(txt.data(), txt.size());
						}

						if (line1 > endLine1)
							patchFile << eol << "\\ No newline at end of file";
					}

					dsr++;
				}
				else
				{
					for (intptr_t i = dsr->sec1.len; i; --i)
					{
						patchFile << eol << diffMark1;
						const auto txt = getLineText(rFile1.compareViewId, line1++);
						patchFile.write(txt.data(), txt.size());
					}
				}
			}
			else
			{
				for (intptr_t i = dsr->sec2.len; i; --i)
				{
					patchFile << eol << diffMark2;
					const auto txt = getLineText(rFile2.compareViewId, line2++);
					patchFile.write(txt.data(), txt.size());
				}
			}
		}

		if (dsn == cmpPair.summary.diffSections.end())
		{
			const auto dsSize = cmpPair.summary.diffSections.size();

			// Replaced section at the end does not require additional processing
			if (dsSize > 1 && cmpPair.summary.diffSections[dsSize - 2].type != MATCH)
			{
				patchFile << eol;
				return;
			}

			break;
		}

		intptr_t oldLine = rOldLine;

		for (; matchContextEnd; --matchContextEnd)
		{
			patchFile << eol << ' ';
			const auto txt = getLineText(oldFile.compareViewId, oldLine++);
			patchFile.write(txt.data(), txt.size());
		}

		if (dsn + 1 == cmpPair.summary.diffSections.end())
		{
			rNewLine += oldLine - rOldLine;
			rOldLine = oldLine;
			break;
		}

		dsi = dsn;
	}

	if (cmpPair.summary.diffSections.back().type == IN_2)
	{
		if (line2 > endLine2)
			patchFile << eol << "\\ No newline at end of file" << eol;
		else
			patchFile << eol;
	}
	else
	{
		if (line1 > endLine1)
			patchFile << eol << "\\ No newline at end of file" << eol;
		else
			patchFile << eol;
	}
}


void GeneratePatch()
{
	CompareList_t::iterator cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return;

	const auto& str = Strings::get();

	if (cmpPair->options.findUniqueMode)
	{
		::MessageBoxW(nppData._nppHandle, str["MSG_PATCH_NO_UNIQUE"].c_str(), PLUGIN_NAME, MB_OK);
		return;
	}

	if (cmpPair->compareDirty)
	{
		::MessageBoxW(nppData._nppHandle, str["MSG_PATCH_NO_MODIFIED"].c_str(), PLUGIN_NAME, MB_OK | MB_ICONWARNING);
		return;
	}

	const bool hasIgnoreOpts =
		(cmpPair->options.ignoreChangedSpaces || cmpPair->options.ignoreAllSpaces || cmpPair->options.ignoreEOL ||
		cmpPair->options.ignoreEmptyLines || cmpPair->options.ignoreCase || cmpPair->options.ignoreRegex ||
		cmpPair->options.ignoreFoldedLines || cmpPair->options.ignoreHiddenLines);

	if (hasIgnoreOpts)
		::MessageBoxW(nppData._nppHandle, str["MSG_PATCH_WARN_IGNORE"].c_str(), PLUGIN_NAME, MB_OK | MB_ICONWARNING);

	std::ofstream ofs;
	{
		wchar_t fname[2048];

		OPENFILENAMEW ofn;

		::ZeroMemory(&fname, sizeof(fname));
		::ZeroMemory(&ofn, sizeof(ofn));

		std::wstring filter = str["PATCH_FILTER_ALL"];
		filter.insert(filter.end(), L'\0');
		filter += L"*.*";
		filter.insert(filter.end(), L'\0');

		ofn.lStructSize		= sizeof(ofn);
		ofn.hwndOwner		= nppData._nppHandle;
		ofn.lpstrFilter		= filter.c_str();
		ofn.lpstrFile		= fname;
		ofn.nMaxFile		= _countof(fname);
		ofn.lpstrInitialDir	= nullptr;
		ofn.lpstrTitle		= str["PATCH_SAVE_AS"].c_str();
		ofn.Flags			= OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

		if (!::GetSaveFileNameW(&ofn))
			return;

		ofs.open(fname, std::ios_base::trunc | std::ios_base::binary);

		if (!ofs.is_open())
		{
			::MessageBoxW(nppData._nppHandle, str["MSG_PATCH_SAVE_FAIL"].c_str(), PLUGIN_NAME, MB_OK | MB_ICONERROR);
			return;
		}
	}

	formatAndWritePatch(*cmpPair, ofs, hasIgnoreOpts ? 0 : 3);

	ofs.flush();
	ofs.close();
}


intptr_t lineNumFromPatchDiff(const std::string& patchDiff, char linePrefix)
{
	const size_t numPos = patchDiff.find(linePrefix);

	if ((numPos == std::string::npos) || (numPos == patchDiff.size() - 1))
		return -1;

	return (intptr_t)strtoll(patchDiff.c_str() + numPos + 1, NULL, 10) - 1;
}


bool readAndApplyPatch(std::ifstream& patchFile, bool revert)
{
	IFStreamLineGetter patchLineGetter(patchFile);

	std::string lineStr = patchLineGetter.get();

	// Skip patch file lines until the first diff section
	for (; !lineStr.empty() && *(lineStr.c_str()) != '@'; lineStr = patchLineGetter.get());

	if (lineStr.empty())
		return false;

	const char oldMark = revert ? '+' : '-';
	const char newMark = revert ? '-' : '+';

	intptr_t oldLine = lineNumFromPatchDiff(lineStr, oldMark);
	if (oldLine < 0)
		return false;

	bool res = true;
	int view = getCurrentViewId();

	std::string searchStr;
	std::string replaceStr;

	std::string* lastUpdatedStr = nullptr;

	intptr_t oldLineOffset = 0;

	intptr_t searchedLines = 0;
	intptr_t replacedLines = 0;

	intptr_t replacements = 0;

	CallScintilla(view, SCI_BEGINUNDOACTION, 0, 0);

	for (lineStr = patchLineGetter.get(); !lineStr.empty(); lineStr = patchLineGetter.get())
	{
		if (*(lineStr.c_str()) == ' ')
		{
			searchStr.append(lineStr, 1);
			replaceStr.append(lineStr, 1);
			++searchedLines;
			++replacedLines;
			lastUpdatedStr = nullptr;
		}
		else if (*(lineStr.c_str()) == oldMark)
		{
			searchStr.append(lineStr, 1);
			++searchedLines;
			lastUpdatedStr = &searchStr;
		}
		else if (*(lineStr.c_str()) == newMark)
		{
			replaceStr.append(lineStr, 1);
			++replacedLines;
			lastUpdatedStr = &replaceStr;
		}
		else if (*(lineStr.c_str()) == '@')
		{
			if (!searchedLines)
				++oldLine;

			res = (replaceText(view, searchStr, replaceStr, oldLine + oldLineOffset) >= 0);
			if (!res)
				break;

			++replacements;

			oldLine = lineNumFromPatchDiff(lineStr, oldMark);
			if (oldLine < 0)
			{
				res = false;
				break;
			}

			oldLineOffset += replacedLines - searchedLines;

			searchStr.clear();
			replaceStr.clear();

			searchedLines = 0;
			replacedLines = 0;
		}
		// Check for "\ No newline at end of file" - remove last line endings as artificially added to the patch file
		else if (*(lineStr.c_str()) == '\\')
		{
			if (lastUpdatedStr)
			{
				while (!lastUpdatedStr->empty() && (lastUpdatedStr->back() == '\n' || lastUpdatedStr->back() == '\r'))
					lastUpdatedStr->pop_back();
			}
			else
			{
				while (!searchStr.empty() && (searchStr.back() == '\n' || searchStr.back() == '\r'))
					searchStr.pop_back();
				while (!replaceStr.empty() && (replaceStr.back() == '\n' || replaceStr.back() == '\r'))
					replaceStr.pop_back();
			}
		}
	}

	if (!patchFile.eof())
		res = false;

	if (res && (searchedLines || replacedLines))
	{
		if (!searchedLines)
			++oldLine;

		res = (replaceText(view, searchStr, replaceStr, oldLine + oldLineOffset) >= 0);
	}

	CallScintilla(view, SCI_ENDUNDOACTION, 0, 0);

	if (!res && replacements)
		CallScintilla(view, SCI_UNDO, 0, 0);

	return res;
}


void applyPatch(bool revert = false)
{
	const auto& str = Strings::get();

	std::ifstream ifs;
	{
		wchar_t fname[2048];

		OPENFILENAMEW ofn;

		::ZeroMemory(&fname, sizeof(fname));
		::ZeroMemory(&ofn, sizeof(ofn));

		std::wstring filter = str["PATCH_FILTER_PATCH"];
		filter += L" (*.patch, *.diff)";
		filter.insert(filter.end(), L'\0');
		filter += L"*.patch;*.diff";
		filter.insert(filter.end(), L'\0');
		filter += str["PATCH_FILTER_ALL"];
		filter.insert(filter.end(), L'\0');
		filter += L"*.*";
		filter.insert(filter.end(), L'\0');

		ofn.lStructSize		= sizeof(ofn);
		ofn.hwndOwner		= nppData._nppHandle;
		ofn.lpstrFilter		= filter.c_str();
		ofn.lpstrFile		= fname;
		ofn.nMaxFile		= _countof(fname);
		ofn.lpstrInitialDir	= nullptr;
		ofn.lpstrTitle		= str["PATCH_SELECT"].c_str();
		ofn.Flags			= OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

		if (!::GetOpenFileNameW(&ofn))
			return;

		ifs.open(fname, std::ios_base::binary);

		if (!ifs.is_open())
		{
			::MessageBoxW(nppData._nppHandle, str["MSG_PATCH_OPEN_FAIL"].c_str(), PLUGIN_NAME, MB_OK | MB_ICONERROR);
			return;
		}
	}

	if (!readAndApplyPatch(ifs, revert))
	{
		wchar_t fname[MAX_PATH];

		::SendMessageW(nppData._nppHandle, NPPM_GETFILENAME, _countof(fname), (LPARAM)fname);

		wchar_t msg[MAX_PATH + 256];

		_snwprintf_s(msg, _countof(msg), _TRUNCATE, str["MSG_PATCH_APPLY_FAIL"].c_str(), fname);

		::MessageBoxW(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK | MB_ICONERROR);
	}

	ifs.close();
}


void ApplyPatch()
{
	applyPatch();
}


void RevertPatch()
{
	applyPatch(true);
}


void OpenCompareOptionsDlg()
{
	CompareOptionsDialog compareOptionsDlg(hInstance, nppData._nppHandle);

	if (compareOptionsDlg.doDialog(&Settings) != IDOK)
		return;

	::SendMessageW(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_COMPARE_OPTIONS]._cmdID, (LPARAM)
		(Settings.IgnoreChangedSpaces || Settings.IgnoreAllSpaces || Settings.IgnoreEOL || Settings.IgnoreCase ||
		Settings.IgnoreRegex || Settings.IgnoreEmptyLines || Settings.IgnoreFoldedLines || Settings.IgnoreHiddenLines));
}


void BookmarksAsSyncPoints()
{
	Settings.BookmarksAsSync = !Settings.BookmarksAsSync;
	::SendMessageW(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_BOOKMARKS_SYNC]._cmdID,
			(LPARAM)Settings.BookmarksAsSync);
	Settings.markAsDirty();
}


void OpenVisualFiltersDlg()
{
	VisualFiltersDialog visualFiltersDlg(hInstance, nppData._nppHandle);

	if (visualFiltersDlg.doDialog(&Settings) != IDOK)
		return;

	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());

	if (cmpPair != compareList.end())
	{
		ScopedIncrementerInt incr(notificationsLock);

		const int view			= getCurrentViewId();
		intptr_t currentLine	= (Settings.FollowingCaret ? getCurrentLine(view) : getFirstLine(view));
		bool currentLineChanged = false;

		if (Settings.ShowOnlySelections)
		{
			if (currentLine < cmpPair->options.selections[view].first)
			{
				currentLine = cmpPair->options.selections[view].first;
				currentLineChanged = true;
			}
			else if (currentLine > cmpPair->options.selections[view].second)
			{
				currentLine = cmpPair->options.selections[view].second;
				currentLineChanged = true;
			}
		}

		if (Settings.FollowingCaret && currentLineChanged)
			CallScintilla(view, SCI_GOTOLINE, currentLine, 0);

		ViewLocation loc(view, currentLine);

		alignDiffs(cmpPair);

		loc.restore(Settings.FollowingCaret);

		NavDlg.Update();
	}

	::SendMessageW(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DIFFS_VISUAL_FILTERS]._cmdID, (LPARAM)
		(Settings.HideMatches || Settings.HideNewLines || Settings.HideChangedLines || Settings.HideMovedLines));
}


void AutoRecompare()
{
	Settings.RecompareOnChange = !Settings.RecompareOnChange;
	::SendMessageW(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_AUTO_RECOMPARE]._cmdID,
			(LPARAM)Settings.RecompareOnChange);
	Settings.markAsDirty();

	if (Settings.RecompareOnChange)
	{
		CompareList_t::iterator cmpPair = getCompare(getCurrentBuffId());

		if ((cmpPair != compareList.end()) && cmpPair->compareDirty && cmpPair->options.recompareOnChange)
			delayedRecompare.post(30);
	}
}


void OpenSettingsDlg(void)
{
	SettingsDialog settingsDlg(hInstance, nppData._nppHandle);

	if (settingsDlg.doDialog(&Settings) == IDOK)
	{
		Settings.save();

		newCompare = nullptr;

		if (!compareList.empty())
		{
			setStyles(Settings);
			NavDlg.SetColors(Settings.colors(), isDarkMode());

			if (NppState::get().compareMode)
			{
				setCompareView(MAIN_VIEW, !Settings.HideMargin,
						Settings.colors().blank, Settings.colors().caret_line_transparency);
				setCompareView(SUB_VIEW, !Settings.HideMargin,
						Settings.colors().blank, Settings.colors().caret_line_transparency);
			}
		}
	}
}


void OpenAboutDlg()
{
#ifdef DLOG

	if (dLogBuf == -1)
	{
		::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

		dLogBuf = getCurrentBuffId();

		HWND hNppTabBar = NppTabHandleGetter::get(getCurrentViewId());

		if (hNppTabBar)
		{
			wchar_t name[] = { L"CP_debug_log" };

			TCITEMW tab;
			tab.mask = TCIF_TEXT;
			tab.pszText = name;

			TabCtrl_SetItem(hNppTabBar, posFromBuffId(dLogBuf), &tab);
		}
	}
	else
	{
		activateBufferID(dLogBuf);
	}

	const int view = getCurrentViewId();

	CallScintilla(view, SCI_APPENDTEXT, dLog.size(), (LPARAM)dLog.c_str());
	CallScintilla(view, SCI_SETSAVEPOINT, 0, 0);

	dLog.clear();

#else

	AboutDialog aboutDlg(hInstance, nppData._nppHandle, GetLibGit2Ver(), GetSQLite3Ver());
	aboutDlg.doDialog();

#endif
}


void createMenu()
{
	const auto& str = Strings::get();

	wcscpy_s(funcItem[CMD_SET_FIRST]._itemName, menuItemSize, str["CMD_SET_FIRST"].c_str());
	funcItem[CMD_SET_FIRST]._pFunc					= SetAsFirst;
	funcItem[CMD_SET_FIRST]._pShKey					= new ShortcutKey;
	funcItem[CMD_SET_FIRST]._pShKey->_isAlt			= true;
	funcItem[CMD_SET_FIRST]._pShKey->_isCtrl		= true;
	funcItem[CMD_SET_FIRST]._pShKey->_isShift		= false;
	funcItem[CMD_SET_FIRST]._pShKey->_key			= '1';

	wcscpy_s(funcItem[CMD_COMPARE]._itemName, menuItemSize, str["CMD_COMPARE"].c_str());
	funcItem[CMD_COMPARE]._pFunc					= CompareWhole;
	funcItem[CMD_COMPARE]._pShKey					= new ShortcutKey;
	funcItem[CMD_COMPARE]._pShKey->_isAlt			= true;
	funcItem[CMD_COMPARE]._pShKey->_isCtrl			= true;
	funcItem[CMD_COMPARE]._pShKey->_isShift			= false;
	funcItem[CMD_COMPARE]._pShKey->_key				= 'C';

	wcscpy_s(funcItem[CMD_COMPARE_SEL]._itemName, menuItemSize, str["CMD_COMPARE_SEL"].c_str());
	funcItem[CMD_COMPARE_SEL]._pFunc				= CompareSelections;
	funcItem[CMD_COMPARE_SEL]._pShKey				= new ShortcutKey;
	funcItem[CMD_COMPARE_SEL]._pShKey->_isAlt		= true;
	funcItem[CMD_COMPARE_SEL]._pShKey->_isCtrl		= true;
	funcItem[CMD_COMPARE_SEL]._pShKey->_isShift		= false;
	funcItem[CMD_COMPARE_SEL]._pShKey->_key			= 'N';

	wcscpy_s(funcItem[CMD_FIND_UNIQUE]._itemName, menuItemSize, str["CMD_FIND_UNIQUE"].c_str());
	funcItem[CMD_FIND_UNIQUE]._pFunc				= FindUnique;
	funcItem[CMD_FIND_UNIQUE]._pShKey				= new ShortcutKey;
	funcItem[CMD_FIND_UNIQUE]._pShKey->_isAlt		= true;
	funcItem[CMD_FIND_UNIQUE]._pShKey->_isCtrl		= true;
	funcItem[CMD_FIND_UNIQUE]._pShKey->_isShift		= true;
	funcItem[CMD_FIND_UNIQUE]._pShKey->_key			= 'C';

	wcscpy_s(funcItem[CMD_FIND_UNIQUE_SEL]._itemName, menuItemSize, str["CMD_FIND_UNIQUE_SEL"].c_str());
	funcItem[CMD_FIND_UNIQUE_SEL]._pFunc			= FindSelectionsUnique;
	funcItem[CMD_FIND_UNIQUE_SEL]._pShKey			= new ShortcutKey;
	funcItem[CMD_FIND_UNIQUE_SEL]._pShKey->_isAlt	= true;
	funcItem[CMD_FIND_UNIQUE_SEL]._pShKey->_isCtrl	= true;
	funcItem[CMD_FIND_UNIQUE_SEL]._pShKey->_isShift	= true;
	funcItem[CMD_FIND_UNIQUE_SEL]._pShKey->_key		= 'N';

	wcscpy_s(funcItem[CMD_LAST_SAVE_DIFF]._itemName, menuItemSize, str["CMD_LAST_SAVE_DIFF"].c_str());
	funcItem[CMD_LAST_SAVE_DIFF]._pFunc				= LastSaveDiff;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey 			= new ShortcutKey;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_isAlt 	= true;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_isCtrl 	= true;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_isShift	= false;
	funcItem[CMD_LAST_SAVE_DIFF]._pShKey->_key 		= 'D';

	wcscpy_s(funcItem[CMD_CLIPBOARD_DIFF]._itemName, menuItemSize, str["CMD_CLIPBOARD_DIFF"].c_str());
	funcItem[CMD_CLIPBOARD_DIFF]._pFunc 			= ClipboardDiff;
	funcItem[CMD_CLIPBOARD_DIFF]._pShKey 			= new ShortcutKey;
	funcItem[CMD_CLIPBOARD_DIFF]._pShKey->_isAlt 	= true;
	funcItem[CMD_CLIPBOARD_DIFF]._pShKey->_isCtrl 	= true;
	funcItem[CMD_CLIPBOARD_DIFF]._pShKey->_isShift	= false;
	funcItem[CMD_CLIPBOARD_DIFF]._pShKey->_key 		= 'M';

	wcscpy_s(funcItem[CMD_SVN_DIFF]._itemName, menuItemSize, str["CMD_SVN_DIFF"].c_str());
	funcItem[CMD_SVN_DIFF]._pFunc 					= SvnDiff;
	funcItem[CMD_SVN_DIFF]._pShKey 					= new ShortcutKey;
	funcItem[CMD_SVN_DIFF]._pShKey->_isAlt 			= true;
	funcItem[CMD_SVN_DIFF]._pShKey->_isCtrl 		= true;
	funcItem[CMD_SVN_DIFF]._pShKey->_isShift		= false;
	funcItem[CMD_SVN_DIFF]._pShKey->_key 			= 'V';

	wcscpy_s(funcItem[CMD_GIT_DIFF]._itemName, menuItemSize, str["CMD_GIT_DIFF"].c_str());
	funcItem[CMD_GIT_DIFF]._pFunc 					= GitDiff;
	funcItem[CMD_GIT_DIFF]._pShKey 					= new ShortcutKey;
	funcItem[CMD_GIT_DIFF]._pShKey->_isAlt 			= true;
	funcItem[CMD_GIT_DIFF]._pShKey->_isCtrl 		= true;
	funcItem[CMD_GIT_DIFF]._pShKey->_isShift		= false;
	funcItem[CMD_GIT_DIFF]._pShKey->_key 			= 'G';

	wcscpy_s(funcItem[CMD_CLEAR_ACTIVE]._itemName, menuItemSize, str["CMD_CLEAR_ACTIVE"].c_str());
	funcItem[CMD_CLEAR_ACTIVE]._pFunc				= ClearActiveCompare;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey 				= new ShortcutKey;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_isAlt 		= true;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_isCtrl		= true;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_isShift	= false;
	funcItem[CMD_CLEAR_ACTIVE]._pShKey->_key 		= 'X';

	wcscpy_s(funcItem[CMD_CLEAR_ALL]._itemName, menuItemSize, str["CMD_CLEAR_ALL"].c_str());
	funcItem[CMD_CLEAR_ALL]._pFunc	= ClearAllCompares;

	wcscpy_s(funcItem[CMD_FIRST]._itemName, menuItemSize, str["CMD_FIRST"].c_str());
	funcItem[CMD_FIRST]._pFunc 				= First;
	funcItem[CMD_FIRST]._pShKey 			= new ShortcutKey;
	funcItem[CMD_FIRST]._pShKey->_isAlt 	= true;
	funcItem[CMD_FIRST]._pShKey->_isCtrl 	= true;
	funcItem[CMD_FIRST]._pShKey->_isShift	= false;
	funcItem[CMD_FIRST]._pShKey->_key 		= VK_PRIOR;

	wcscpy_s(funcItem[CMD_PREV]._itemName, menuItemSize, str["CMD_PREV"].c_str());
	funcItem[CMD_PREV]._pFunc 				= Prev;
	funcItem[CMD_PREV]._pShKey 				= new ShortcutKey;
	funcItem[CMD_PREV]._pShKey->_isAlt 		= true;
	funcItem[CMD_PREV]._pShKey->_isCtrl 	= false;
	funcItem[CMD_PREV]._pShKey->_isShift	= false;
	funcItem[CMD_PREV]._pShKey->_key 		= VK_PRIOR;

	wcscpy_s(funcItem[CMD_NEXT]._itemName, menuItemSize, str["CMD_NEXT"].c_str());
	funcItem[CMD_NEXT]._pFunc 				= Next;
	funcItem[CMD_NEXT]._pShKey 				= new ShortcutKey;
	funcItem[CMD_NEXT]._pShKey->_isAlt 		= true;
	funcItem[CMD_NEXT]._pShKey->_isCtrl 	= false;
	funcItem[CMD_NEXT]._pShKey->_isShift	= false;
	funcItem[CMD_NEXT]._pShKey->_key 		= VK_NEXT;

	wcscpy_s(funcItem[CMD_LAST]._itemName, menuItemSize, str["CMD_LAST"].c_str());
	funcItem[CMD_LAST]._pFunc 				= Last;
	funcItem[CMD_LAST]._pShKey 				= new ShortcutKey;
	funcItem[CMD_LAST]._pShKey->_isAlt 		= true;
	funcItem[CMD_LAST]._pShKey->_isCtrl 	= true;
	funcItem[CMD_LAST]._pShKey->_isShift	= false;
	funcItem[CMD_LAST]._pShKey->_key 		= VK_NEXT;

	wcscpy_s(funcItem[CMD_PREV_CHANGE_POS]._itemName, menuItemSize, str["CMD_PREV_CHANGE_POS"].c_str());
	funcItem[CMD_PREV_CHANGE_POS]._pFunc 			= PrevChangePos;
	funcItem[CMD_PREV_CHANGE_POS]._pShKey 			= new ShortcutKey;
	funcItem[CMD_PREV_CHANGE_POS]._pShKey->_isAlt 	= true;
	funcItem[CMD_PREV_CHANGE_POS]._pShKey->_isCtrl 	= true;
	funcItem[CMD_PREV_CHANGE_POS]._pShKey->_isShift	= true;
	funcItem[CMD_PREV_CHANGE_POS]._pShKey->_key 	= VK_PRIOR;

	wcscpy_s(funcItem[CMD_NEXT_CHANGE_POS]._itemName, menuItemSize, str["CMD_NEXT_CHANGE_POS"].c_str());
	funcItem[CMD_NEXT_CHANGE_POS]._pFunc 			= NextChangePos;
	funcItem[CMD_NEXT_CHANGE_POS]._pShKey 			= new ShortcutKey;
	funcItem[CMD_NEXT_CHANGE_POS]._pShKey->_isAlt 	= true;
	funcItem[CMD_NEXT_CHANGE_POS]._pShKey->_isCtrl 	= true;
	funcItem[CMD_NEXT_CHANGE_POS]._pShKey->_isShift	= true;
	funcItem[CMD_NEXT_CHANGE_POS]._pShKey->_key 	= VK_NEXT;

	wcscpy_s(funcItem[CMD_COMPARE_SUMMARY]._itemName, menuItemSize, str["CMD_COMPARE_SUMMARY"].c_str());
	funcItem[CMD_COMPARE_SUMMARY]._pFunc = ActiveCompareSummary;

	wcscpy_s(funcItem[CMD_COPY_VISIBLE]._itemName, menuItemSize, str["CMD_COPY_VISIBLE"].c_str());
	funcItem[CMD_COPY_VISIBLE]._pFunc = CopyVisibleLines;

	wcscpy_s(funcItem[CMD_DELETE_VISIBLE]._itemName, menuItemSize, str["CMD_DELETE_VISIBLE"].c_str());
	funcItem[CMD_DELETE_VISIBLE]._pFunc = DeleteVisibleLines;

	wcscpy_s(funcItem[CMD_BOOKMARK_VISIBLE]._itemName, menuItemSize, str["CMD_BOOKMARK_VISIBLE"].c_str());
	funcItem[CMD_BOOKMARK_VISIBLE]._pFunc = BookmarkVisibleLines;

	wcscpy_s(funcItem[CMD_GENERATE_PATCH]._itemName, menuItemSize, str["CMD_GENERATE_PATCH"].c_str());
	funcItem[CMD_GENERATE_PATCH]._pFunc = GeneratePatch;

	wcscpy_s(funcItem[CMD_APPLY_PATCH]._itemName, menuItemSize, str["CMD_APPLY_PATCH"].c_str());
	funcItem[CMD_APPLY_PATCH]._pFunc = ApplyPatch;

	wcscpy_s(funcItem[CMD_REVERT_PATCH]._itemName, menuItemSize, str["CMD_REVERT_PATCH"].c_str());
	funcItem[CMD_REVERT_PATCH]._pFunc = RevertPatch;

	wcscpy_s(funcItem[CMD_COMPARE_OPTIONS]._itemName, menuItemSize, str["CMD_COMPARE_OPTIONS"].c_str());
	funcItem[CMD_COMPARE_OPTIONS]._pFunc = OpenCompareOptionsDlg;

	wcscpy_s(funcItem[CMD_BOOKMARKS_SYNC]._itemName, menuItemSize, str["CMD_BOOKMARKS_SYNC"].c_str());
	funcItem[CMD_BOOKMARKS_SYNC]._pFunc = BookmarksAsSyncPoints;

	wcscpy_s(funcItem[CMD_DIFFS_VISUAL_FILTERS]._itemName, menuItemSize, str["CMD_DIFFS_VISUAL_FILTERS"].c_str());
	funcItem[CMD_DIFFS_VISUAL_FILTERS]._pFunc = OpenVisualFiltersDlg;

	wcscpy_s(funcItem[CMD_NAV_BAR]._itemName, menuItemSize, str["CMD_NAV_BAR"].c_str());
	funcItem[CMD_NAV_BAR]._pFunc = ToggleNavigationBar;

	wcscpy_s(funcItem[CMD_AUTO_RECOMPARE]._itemName, menuItemSize, str["CMD_AUTO_RECOMPARE"].c_str());
	funcItem[CMD_AUTO_RECOMPARE]._pFunc = AutoRecompare;

	wcscpy_s(funcItem[CMD_SETTINGS]._itemName, menuItemSize, str["CMD_SETTINGS"].c_str());
	funcItem[CMD_SETTINGS]._pFunc = OpenSettingsDlg;

	wcscpy_s(funcItem[CMD_ABOUT]._itemName, menuItemSize, str["CMD_ABOUT"].c_str());
	funcItem[CMD_ABOUT]._pFunc = OpenAboutDlg;
}


void freeToolbarObjects(toolbarIconsWithDarkMode& tb)
{
	if (tb.hToolbarBmp)
		::DeleteObject(tb.hToolbarBmp);
	if (tb.hToolbarIcon)
		::DestroyIcon(tb.hToolbarIcon);
	if (tb.hToolbarIconDarkMode)
		::DestroyIcon(tb.hToolbarIconDarkMode);
}


void deinitPlugin()
{
	// Always close it, else N++'s plugin manager would call 'ToggleNavigationBar'
	// on startup, when N++ has been shut down before with opened navigation bar
	if (NavDlg.isVisible())
		NavDlg.Hide();

	freeToolbarObjects(tbSetFirst);
	freeToolbarObjects(tbCompare);
	freeToolbarObjects(tbCompareSel);
	freeToolbarObjects(tbClearCompare);
	freeToolbarObjects(tbFirst);
	freeToolbarObjects(tbPrev);
	freeToolbarObjects(tbNext);
	freeToolbarObjects(tbLast);
	freeToolbarObjects(tbDiffsFilters);
	freeToolbarObjects(tbNavBar);

	NavDlg.destroy();

	// Deallocate shortcut
	for (int i = 0; i < NB_MENU_COMMANDS; i++)
	{
		if (funcItem[i]._pShKey != NULL)
		{
			delete funcItem[i]._pShKey;
			funcItem[i]._pShKey = NULL;
		}
	}
}


void comparedFileActivated()
{
	if (!NppState::get().compareMode)
	{
		if (Settings.ShowNavBar && !NavDlg.isVisible())
			showNavBar();

		ScopedIncrementerInt incr(notificationsLock);

		NppState::get().setCompareMode();
	}

	CallScintilla(MAIN_VIEW,	SCI_MARKERDELETEALL, MARKER_ARROW_SYMBOL, 0);
	CallScintilla(SUB_VIEW,		SCI_MARKERDELETEALL, MARKER_ARROW_SYMBOL, 0);

	temporaryRangeSelect(-1);
	setArrowMark(-1);

	setCompareView(MAIN_VIEW, !Settings.HideMargin, Settings.colors().blank, Settings.colors().caret_line_transparency);
	setCompareView(SUB_VIEW, !Settings.HideMargin, Settings.colors().blank, Settings.colors().caret_line_transparency);

	CompareList_t::iterator	cmpPair = getCompare(getCurrentBuffId());

	if (cmpPair != compareList.end())
		cmpPair->hideFlags = FORCE_REHIDING;
}


std::pair<std::wstring, std::wstring> getTwoFilenamesFromCmdLine(const wchar_t* cmdLine)
{
	if (cmdLine == nullptr)
		return std::make_pair(std::wstring{}, std::wstring{});

	std::wstring firstFile;
	std::wstring secondFile;

	for (; *cmdLine != L'\0'; ++cmdLine)
	{
		if (*cmdLine == L' ')
			continue;

		wchar_t sectionEnd = L' ';

		if (*cmdLine == L'-')
		{
			for (++cmdLine; *cmdLine != L'\0'; ++cmdLine)
			{
				if (*cmdLine == sectionEnd)
					break;

				if (*cmdLine == L'"' && sectionEnd == L' ')
					sectionEnd = L'"';
				else if (*cmdLine == L'\'' && sectionEnd == L' ')
					sectionEnd = L'\'';
			}
		}
		else
		{
			if (*cmdLine == L'"')
			{
				sectionEnd = L'"';
				++cmdLine;
			}
			else if (*cmdLine == L'\'')
			{
				sectionEnd = L'\'';
				++cmdLine;
			}

			if (*cmdLine == L'\0')
				break;

			const wchar_t* startPos = cmdLine;

			for (++cmdLine; *cmdLine != sectionEnd && *cmdLine != L'\0'; ++cmdLine);

			if (firstFile.empty())
			{
				firstFile.assign(startPos, cmdLine);

				for (size_t i = 0; i < firstFile.size(); ++i)
					if (firstFile.at(i) == L'/')
						firstFile.at(i) = L'\\';
			}
			else
			{
				secondFile.assign(startPos, cmdLine);

				for (size_t i = 0; i < secondFile.size(); ++i)
					if (secondFile.at(i) == L'/')
						secondFile.at(i) = L'\\';

				break;
			}

			if (*cmdLine == L'\0')
				break;
		}
	}

	return std::make_pair(firstFile, secondFile);
}


// Find command line files full paths
// Because Notepad++ uses ::SetCurrentDirectory() the folder from which it was started is lost so no way to
// retrieve easily the command line files relative paths. That's why we try to parse all opened files to
// try to construct the command line files full paths
bool constructFullFilePaths(std::pair<std::wstring, std::wstring>& files)
{
	std::vector<std::wstring> openedFiles = getOpenedFiles();
	const int openedFilesCount = static_cast<int>(openedFiles.size());

	if (!openedFilesCount)
		return false;

	std::wstring& longerName = files.first.size() >= files.second.size() ? files.first : files.second;
	std::wstring& shorterName = files.first.size() >= files.second.size() ? files.second : files.first;

	const size_t longerLen = longerName.size();
	const size_t shorterLen = shorterName.size();

	int longerFound = 0;
	int shorterFound = 0;

	int longerIdx = 0;
	int shorteridx = 0;

	for (int i = 0; i < openedFilesCount; ++i)
	{
		const size_t pathLen = openedFiles[i].size();

		if (pathLen >= longerLen && (pathLen == longerLen ||
			openedFiles[i][pathLen - longerLen - 1] == L'\\' || openedFiles[i][pathLen - longerLen - 1] == L'/') &&
			!openedFiles[i].compare(pathLen - longerLen, longerLen, longerName))
		{
			if (++longerFound == 1)
				longerIdx = i;
			else
				return false;
		}
		else if (pathLen >= shorterLen && (pathLen == shorterLen ||
			openedFiles[i][pathLen - shorterLen - 1] == L'\\' || openedFiles[i][pathLen - shorterLen - 1] == L'/') &&
			!openedFiles[i].compare(pathLen - shorterLen, shorterLen, shorterName))
		{
			if (++shorterFound == 1)
				shorteridx = i;
			else
				return false;
		}
	}

	if (longerFound && shorterFound)
	{
		longerName = std::move(openedFiles[longerIdx]);
		shorterName = std::move(openedFiles[shorteridx]);

		return true;
	}

	return false;
}


void checkCmdLine()
{
	constexpr wchar_t compareRunCmd[]	= L"-pluginMessage=compare";
	constexpr int minCmdLineLen			= sizeof(compareRunCmd) / sizeof(wchar_t);

	wchar_t cmdLine[2048];

	if (::SendMessageW(nppData._nppHandle, NPPM_GETCURRENTCMDLINE, 2048, (LPARAM)cmdLine) <= minCmdLineLen)
		return;

	wchar_t* pos = wcsstr(cmdLine, compareRunCmd);

	if (pos == nullptr)
		return;

	if (pos == cmdLine)
		pos += minCmdLineLen;
	else
		pos = cmdLine;

	auto files = getTwoFilenamesFromCmdLine(pos);

	if (files.first.empty() || files.second.empty())
		return;

	{
		wchar_t tmp[MAX_PATH];

		::PathCanonicalizeW(tmp, files.first.c_str());

		files.first = tmp[0] == L'\\' || tmp[0] == L'/' ? tmp + 1 : tmp;

		::PathCanonicalizeW(tmp, files.second.c_str());

		files.second = tmp[0] == L'\\' || tmp[0] == L'/' ? tmp + 1 : tmp;
	}

	if (!constructFullFilePaths(files))
	{
		::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_CMD_LINE_AMBIGUOUS"].c_str(), PLUGIN_NAME, MB_OK);
		return;
	}

	{
		ScopedIncrementerInt incr(notificationsLock);

		::SendMessageW(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)files.first.c_str());

		// First file on the command line is the new one
		setFirst(true);

		::SendMessageW(nppData._nppHandle, NPPM_SWITCHTOFILE, 0, (LPARAM)files.second.c_str());
	}

	compare();
}


void onToolBarReady()
{
	int bmpX = 0;
	int bmpY = 0;

	int icoX = 0;
	int icoY = 0;

	HDC hdc = ::GetDC(NULL);
	if (hdc)
	{
		// Bitmaps are 16x16
		bmpX = ::MulDiv(16, ::GetDeviceCaps(hdc, LOGPIXELSX), 96);
		bmpY = ::MulDiv(16, ::GetDeviceCaps(hdc, LOGPIXELSY), 96);

		gMarginWidth = bmpX;

		// Icons are 32x32
		icoX = ::MulDiv(32, ::GetDeviceCaps(hdc, LOGPIXELSX), 96);
		icoY = ::MulDiv(32, ::GetDeviceCaps(hdc, LOGPIXELSY), 96);

		ReleaseDC(NULL, hdc);
	}

	if (!Settings.EnableToolbar)
		return;

	constexpr UINT style		= (LR_LOADTRANSPARENT);
	constexpr UINT styleDark	= (LR_LOADTRANSPARENT);

	if (Settings.SetAsFirstTB)
	{
		if (isRTLwindow(nppData._nppHandle))
		{
			tbSetFirst.hToolbarBmp			= (HBITMAP)
				::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_SETFIRST_RTL), IMAGE_BITMAP, bmpX, bmpY, style);
			tbSetFirst.hToolbarIcon			= (HICON)
				::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_SETFIRST_RTL_FL), IMAGE_ICON, icoX, icoY, style);
			tbSetFirst.hToolbarIconDarkMode	= (HICON)
				::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_SETFIRST_RTL_FL_DM), IMAGE_ICON, icoX, icoY, styleDark);
		}
		else
		{
			tbSetFirst.hToolbarBmp			= (HBITMAP)
				::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_SETFIRST), IMAGE_BITMAP, bmpX, bmpY, style);
			tbSetFirst.hToolbarIcon			= (HICON)
				::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_SETFIRST_FL), IMAGE_ICON, icoX, icoY, style);
			tbSetFirst.hToolbarIconDarkMode	= (HICON)
				::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_SETFIRST_FL_DM), IMAGE_ICON, icoX, icoY, styleDark);
		}

		::SendMessageW(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_SET_FIRST]._cmdID, (LPARAM)&tbSetFirst);
	}

	if (Settings.CompareTB)
	{
		tbCompare.hToolbarBmp				= (HBITMAP)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_COMPARE), IMAGE_BITMAP, bmpX, bmpY, style);
		tbCompare.hToolbarIcon				= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_COMPARE_FL), IMAGE_ICON, icoX, icoY, style);
		tbCompare.hToolbarIconDarkMode		= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_COMPARE_FL_DM), IMAGE_ICON, icoX, icoY, styleDark);

		::SendMessageW(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_COMPARE]._cmdID, (LPARAM)&tbCompare);
	}

	if (Settings.CompareSelTB)
	{
		tbCompareSel.hToolbarBmp			= (HBITMAP)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_COMPARE_LINES), IMAGE_BITMAP, bmpX, bmpY, style);
		tbCompareSel.hToolbarIcon			= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_COMPARE_LINES_FL), IMAGE_ICON, icoX, icoY, style);
		tbCompareSel.hToolbarIconDarkMode	= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_COMPARE_LINES_FL_DM), IMAGE_ICON, icoX, icoY, styleDark);

		::SendMessageW(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_COMPARE_SEL]._cmdID, (LPARAM)&tbCompareSel);
	}

	if (Settings.ClearCompareTB)
	{
		tbClearCompare.hToolbarBmp			= (HBITMAP)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_CLEARCOMPARE), IMAGE_BITMAP, bmpX, bmpY, style);
		tbClearCompare.hToolbarIcon			= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_CLEARCOMPARE_FL), IMAGE_ICON, icoX, icoY, style);
		tbClearCompare.hToolbarIconDarkMode	= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_CLEARCOMPARE_FL_DM), IMAGE_ICON, icoX, icoY, styleDark);

		::SendMessageW(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_CLEAR_ACTIVE]._cmdID, (LPARAM)&tbClearCompare);
	}

	if (Settings.NavigationTB)
	{
		tbFirst.hToolbarBmp					= (HBITMAP)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_FIRST), IMAGE_BITMAP, bmpX, bmpY, style);
		tbFirst.hToolbarIcon				= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_FIRST_FL), IMAGE_ICON, icoX, icoY, style);
		tbFirst.hToolbarIconDarkMode		= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_FIRST_FL_DM), IMAGE_ICON, icoX, icoY, styleDark);

		tbPrev.hToolbarBmp					= (HBITMAP)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_PREV), IMAGE_BITMAP, bmpX, bmpY, style);
		tbPrev.hToolbarIcon					= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_PREV_FL), IMAGE_ICON, icoX, icoY, style);
		tbPrev.hToolbarIconDarkMode			= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_PREV_FL_DM), IMAGE_ICON, icoX, icoY, styleDark);

		tbNext.hToolbarBmp					= (HBITMAP)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_NEXT), IMAGE_BITMAP, bmpX, bmpY, style);
		tbNext.hToolbarIcon					= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_NEXT_FL), IMAGE_ICON, icoX, icoY, style);
		tbNext.hToolbarIconDarkMode			= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_NEXT_FL_DM), IMAGE_ICON, icoX, icoY, styleDark);

		tbLast.hToolbarBmp					= (HBITMAP)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_LAST), IMAGE_BITMAP, bmpX, bmpY, style);
		tbLast.hToolbarIcon					= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_LAST_FL), IMAGE_ICON, icoX, icoY, style);
		tbLast.hToolbarIconDarkMode			= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_LAST_FL_DM), IMAGE_ICON, icoX, icoY, styleDark);

		::SendMessageW(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_FIRST]._cmdID, (LPARAM)&tbFirst);
		::SendMessageW(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_PREV]._cmdID, (LPARAM)&tbPrev);
		::SendMessageW(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_NEXT]._cmdID, (LPARAM)&tbNext);
		::SendMessageW(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_LAST]._cmdID, (LPARAM)&tbLast);
	}

	if (Settings.DiffsFilterTB)
	{
		tbDiffsFilters.hToolbarBmp			= (HBITMAP)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_DIFFS_FILTERS), IMAGE_BITMAP, bmpX, bmpY, style);
		tbDiffsFilters.hToolbarIcon			= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_DIFFS_FILTERS_FL), IMAGE_ICON, icoX, icoY, style);
		tbDiffsFilters.hToolbarIconDarkMode	= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_DIFFS_FILTERS_FL_DM), IMAGE_ICON, icoX, icoY, styleDark);

		::SendMessageW(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_DIFFS_VISUAL_FILTERS]._cmdID, (LPARAM)&tbDiffsFilters);
	}

	if (Settings.NavBarTB)
	{
		tbNavBar.hToolbarBmp				= (HBITMAP)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_NAVBAR), IMAGE_BITMAP, bmpX, bmpY, style);
		tbNavBar.hToolbarIcon				= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_NAVBAR_FL), IMAGE_ICON, icoX, icoY, style);
		tbNavBar.hToolbarIconDarkMode		= (HICON)
			::LoadImageW(hInstance, MAKEINTRESOURCEW(IDB_NAVBAR_FL_DM), IMAGE_ICON, icoX, icoY, styleDark);

		::SendMessageW(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE,
				(WPARAM)funcItem[CMD_NAV_BAR]._cmdID, (LPARAM)&tbNavBar);
	}
}


void onNppReady()
{
	NppState::get().updateLocalization();

	// It's N++'s job actually to disable its scroll menu commands but since it's not the case provide this as a patch
	if (isSingleView())
		NppState::get().enableNppScrollCommands(false);

	::SendMessageW(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_COMPARE_OPTIONS]._cmdID, (LPARAM)
		(Settings.IgnoreChangedSpaces || Settings.IgnoreAllSpaces || Settings.IgnoreEOL || Settings.IgnoreCase ||
		Settings.IgnoreRegex || Settings.IgnoreEmptyLines || Settings.IgnoreFoldedLines || Settings.IgnoreHiddenLines));

	::SendMessageW(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_DIFFS_VISUAL_FILTERS]._cmdID, (LPARAM)
		(Settings.HideMatches || Settings.HideNewLines || Settings.HideChangedLines || Settings.HideMovedLines));

	::SendMessageW(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_BOOKMARKS_SYNC]._cmdID,
			(LPARAM)Settings.BookmarksAsSync);

	::SendMessageW(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_NAV_BAR]._cmdID,
			(LPARAM)Settings.ShowNavBar);

	::SendMessageW(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_AUTO_RECOMPARE]._cmdID,
			(LPARAM)Settings.RecompareOnChange);

	if (getNotepadVersion() < MIN_NOTEPADPP_VERSION)
	{
		wchar_t msg[256];

		_snwprintf_s(msg, _countof(msg), _TRUNCATE, Strings::get()["MSG_NOT_COMPATIBLE"].c_str(), PLUGIN_NAME);

		MessageBoxW(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK | MB_ICONWARNING);

		notificationsLock = 1;

		HMENU hMenu = (HMENU)::SendMessageW(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);

		for (size_t i = 0; i < NB_MENU_COMMANDS; ++i)
		{
			if (funcItem[i]._pFunc != nullptr)
				::EnableMenuItem(hMenu, funcItem[i]._cmdID, MF_BYCOMMAND | MF_GRAYED);
		}

		::DrawMenuBar(nppData._nppHandle);

		HWND hNppToolbar = NppToolbarHandleGetter::get();
		if (hNppToolbar)
		{
			::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_CLEAR_ACTIVE]._cmdID, false);
			::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_FIRST]._cmdID, false);
			::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_PREV]._cmdID, false);
			::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_NEXT]._cmdID, false);
			::SendMessageW(hNppToolbar, TB_ENABLEBUTTON, funcItem[CMD_LAST]._cmdID, false);
		}
	}
	else
	{
		if (!allocateIndicator())
			::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_MARKER_ALLOC_FAIL"].c_str(),
					PLUGIN_NAME, MB_OK | MB_ICONWARNING);

		if (!allocateMarginNum())
			::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_MARGIN_ALLOC_FAIL"].c_str(),
					PLUGIN_NAME, MB_OK | MB_ICONWARNING);

		if (isDarkMode())
			Settings.useDarkColors();
		else
			Settings.useLightColors();

		if (!isSQLlibFound())
		{
			HMENU hMenu = (HMENU)::SendMessageW(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);

			::EnableMenuItem(hMenu, funcItem[CMD_SVN_DIFF]._cmdID, MF_BYCOMMAND | MF_GRAYED);
		}

		if (!isGITlibFound())
		{
			HMENU hMenu = (HMENU)::SendMessageW(nppData._nppHandle, NPPM_GETMENUHANDLE, NPPPLUGINMENU, 0);

			::EnableMenuItem(hMenu, funcItem[CMD_GIT_DIFF]._cmdID, MF_BYCOMMAND | MF_GRAYED);
		}

		readNppBookmarkID();

		if (getNotepadVersion() >= ((8 << 16) | 760)) // Necessary for Notepad++ versions above 8.7.6 (included)
			::SendMessageW(nppData._nppHandle, NPPM_ADDSCNMODIFIEDFLAGS, 0, (LPARAM)SCN_MODIFIED_NOTIF_FLAGS_USED);

		// // This is a Scintilla performance tweak that is added here for testing but needs to be handled properly
		// // in Notepad++ itself (in Notepad++ versions <= 8.8.7 it is disabled and not used)
		// if (CallScintilla(MAIN_VIEW, SCI_SUPPORTSFEATURE, SC_SUPPORTS_THREAD_SAFE_MEASURE_WIDTHS, 0) &&
			// CallScintilla(MAIN_VIEW, SCI_GETLAYOUTTHREADS, 0, 0) == 1)
		// {
			// const auto threadsCount = std::thread::hardware_concurrency();

			// if (threadsCount > 1)
			// {
				// CallScintilla(MAIN_VIEW, SCI_SETLAYOUTTHREADS, threadsCount, 0);
				// CallScintilla(SUB_VIEW, SCI_SETLAYOUTTHREADS, threadsCount, 0);
			// }

			// // ::MessageBoxW(nppData._nppHandle, L"Enabled multithreading for Scintilla layout calculations",
					// // PLUGIN_NAME, MB_OK);
		// }

		// if (CallScintilla(MAIN_VIEW, SCI_GETLAYOUTCACHE, 0, 0) < SC_CACHE_PAGE)
		// {
			// CallScintilla(MAIN_VIEW, SCI_SETLAYOUTCACHE, SC_CACHE_PAGE, 0);
			// CallScintilla(SUB_VIEW, SCI_SETLAYOUTCACHE, SC_CACHE_PAGE, 0);

			// // ::MessageBoxW(nppData._nppHandle, L"Set Scintilla layout cache to page mode", PLUGIN_NAME, MB_OK);
		// }

		// {
			// int positionCache = (int)CallScintilla(MAIN_VIEW, SCI_GETPOSITIONCACHE, 0, 0);

			// // std::wstring msg = L"Scintilla position cache: ";
			// // msg += std::to_wstring(positionCache);

			// // ::MessageBoxW(nppData._nppHandle, msg.c_str(), PLUGIN_NAME, MB_OK);

			// positionCache *= 2;

			// CallScintilla(MAIN_VIEW, SCI_SETPOSITIONCACHE, (WPARAM)positionCache, 0);
			// CallScintilla(SUB_VIEW, SCI_SETPOSITIONCACHE, (WPARAM)positionCache, 0);
		// }

		// if (CallScintilla(MAIN_VIEW, SCI_GETBIDIRECTIONAL, 0, 0) != SC_BIDIRECTIONAL_DISABLED)
		// {
			// std::wstring msg = L"Bidirectional support enabled, value: ";
			// msg += std::to_wstring((int)CallScintilla(MAIN_VIEW, SCI_GETBIDIRECTIONAL, 0, 0));

			// ::MessageBoxW(nppData._nppHandle, msg.c_str(), PLUGIN_NAME, MB_OK);
		// }

		// if (!CallScintilla(MAIN_VIEW, SCI_GETBUFFEREDDRAW, 0, 0))
		// {
			// ::MessageBoxW(nppData._nppHandle, L"Buffered drawing is OFF (no GDI mode)", PLUGIN_NAME, MB_OK);
		// }

		checkCmdLine();
	}
}


// void onPluginMsg(const wchar_t* msg)
// {
	// ::MessageBoxW(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK);
// }


inline void onSciUpdateUI(HWND view)
{
	LOGD(LOG_NOTIF, "onSciUpdateUI()\n");

	const int viewId = getViewIdSafe(view);

	if (viewId >= 0)
		storedLocation = std::make_unique<ViewLocation>(viewId);
}


inline void onSciPaint()
{
	doAlignment();
}


void DelayedAlign::operator()()
{
	doAlignment(_force);
}


void DelayedRecompare::operator()()
{
	compare(false, false, true);
}


inline std::shared_ptr<DeletedSection::UndoData> saveUndoData(int view, SCNotification* notifyCode,
	CompareList_t::iterator& cmpPair, bool* notReverting)
{
	const intptr_t startLine	= CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);
	const intptr_t endLine		= CallScintilla(view, SCI_LINEFROMPOSITION,
			notifyCode->position + notifyCode->length, 0);

	// Change is on single line?
	if (endLine <= startLine)
		return nullptr;

	std::shared_ptr<DeletedSection::UndoData> undo;

	LOGD(LOG_NOTIF, "SC_MOD_BEFOREDELETE: " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") +
			" view, lines range: " + std::to_string(startLine + 1) + "-" + std::to_string(endLine) + "\n");

	const int action = notifyCode->modificationType & (SC_PERFORMED_USER | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);

	ScopedIncrementerInt incr(notificationsLock);

	if (cmpPair->options.selectionCompare)
	{
		undo = std::make_shared<DeletedSection::UndoData>();

		undo->selection = cmpPair->options.selections[view];
	}

	if (!cmpPair->options.recompareOnChange)
	{
		if (!undo)
			undo = std::make_shared<DeletedSection::UndoData>();

		undo->alignment = cmpPair->summary.alignmentInfo;

		if (cmpPair->inEqualizeMode && !copiedSectionMarks.empty())
			undo->otherViewMarks = std::move(copiedSectionMarks);
	}

	*notReverting = cmpPair->getFileByViewId(view).pushDeletedSection(action, startLine, endLine - startLine, undo,
			cmpPair->options.recompareOnChange);

#ifdef DLOG
	if (*notReverting)
	{
		if (undo)
		{
			if (undo->selection.first >= 0)
			{
				LOGD(LOG_NOTIF, "Selection stored.\n");
			}

			if (!undo->alignment.empty())
			{
				LOGD(LOG_NOTIF, "Alignment stored.\n");
			}

			if (!undo->otherViewMarks.empty())
			{
				LOGD(LOG_NOTIF, "Other view markers stored.\n");
			}
		}
	}
#endif

	return undo;
}


inline std::shared_ptr<DeletedSection::UndoData> getUndoData(int view, SCNotification* notifyCode,
	CompareList_t::iterator& cmpPair, bool* selectionsAdjusted)
{
	const intptr_t startLine = CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);

	const int action = notifyCode->modificationType & (SC_PERFORMED_USER | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);

	LOGD(LOG_NOTIF, "SC_MOD_INSERTTEXT: " + std::string(view == MAIN_VIEW ? "MAIN" : "SUB") +
			" view, lines range: " + std::to_string(startLine + 1) + "-" +
			std::to_string(startLine + notifyCode->linesAdded) + "\n");

	ScopedIncrementerInt incr(notificationsLock);

	std::shared_ptr<DeletedSection::UndoData> undo =
			cmpPair->getFileByViewId(view).popDeletedSection(action, startLine);

	if (undo)
	{
		if ((undo->selection.first < undo->selection.second) &&
			(cmpPair->options.selections[view] != undo->selection))
		{
			cmpPair->options.selections[view] = undo->selection;

			*selectionsAdjusted = true;

			LOGD(LOG_NOTIF, "Selection restored.\n");
		}

		if (!cmpPair->options.recompareOnChange)
		{
			cmpPair->summary.alignmentInfo = std::move(undo->alignment);

			LOGD(LOG_NOTIF, "Alignment restored.\n");

			if (!undo->otherViewMarks.empty())
			{
				const intptr_t alignLine = getAlignmentLine(cmpPair->summary.alignmentInfo, view, startLine);

				if (alignLine >= 0)
				{
					setMarkers(getOtherViewId(view), alignLine, undo->otherViewMarks);

					LOGD(LOG_NOTIF, "Other view markers restored.\n");
				}
			}
		}
	}

	return undo;
}


void onSciModified(SCNotification* notifyCode)
{
	static bool notReverting = true;

	const int view = getViewIdSafe((HWND)notifyCode->nmhdr.hwndFrom);
	if (view < 0)
		return;

	CompareList_t::iterator cmpPair = getCompareBySciDoc(getDocId(view));
	if (cmpPair == compareList.end())
		return;

	std::shared_ptr<DeletedSection::UndoData> undo = nullptr;

	bool selectionsAdjusted = false;

	if (notifyCode->modificationType & SC_MOD_BEFOREDELETE)
	{
		undo = saveUndoData(view, notifyCode, cmpPair, &notReverting);
	}
	else if ((notifyCode->modificationType & SC_MOD_INSERTTEXT) && notifyCode->linesAdded)
	{
		notReverting = true;
		undo = getUndoData(view, notifyCode, cmpPair, &selectionsAdjusted);
	}

	if ((notifyCode->modificationType & SC_MOD_DELETETEXT) || (notifyCode->modificationType & SC_MOD_INSERTTEXT))
	{
		delayedAlign.cancel();
		delayedRecompare.cancel();

		if (notifyCode->linesAdded == 0)
			notReverting = true;

		bool updateStatus = false;

		// Set compare dirty flag if needed
		if (!cmpPair->options.recompareOnChange && notReverting && !undo)
		{
			if (!cmpPair->compareDirty || (!cmpPair->inEqualizeMode && !cmpPair->manuallyChanged))
			{
				if (!cmpPair->options.selectionCompare)
				{
					cmpPair->setCompareDirty();
					updateStatus = true;
				}
				else
				{
					intptr_t startLine = CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);

					if ((startLine >= cmpPair->options.selections[view].first) &&
						(startLine <= cmpPair->options.selections[view].second))
					{
						cmpPair->setCompareDirty();
						updateStatus = true;
					}

					if (!updateStatus && notifyCode->linesAdded && (notifyCode->modificationType & SC_MOD_DELETETEXT))
					{
						startLine += notifyCode->linesAdded + 1;

						if ((startLine >= cmpPair->options.selections[view].first) &&
							(startLine <= cmpPair->options.selections[view].second))
						{
							cmpPair->setCompareDirty();
							updateStatus = true;
						}
					}
				}
			}
		}

		// Adjust selections if in selection compare
		if (cmpPair->options.selectionCompare && notifyCode->linesAdded && !undo && !selectionsAdjusted)
		{
			intptr_t startLine		= CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);
			const intptr_t endLine	= startLine + std::abs(notifyCode->linesAdded) - 1;

			if (cmpPair->options.selections[view].first > startLine)
			{
				if (notifyCode->linesAdded > 0)
				{
					cmpPair->options.selections[view].first += notifyCode->linesAdded;
				}
				else
				{
					if (cmpPair->options.selections[view].first > endLine)
						cmpPair->options.selections[view].first += notifyCode->linesAdded;
					else
						cmpPair->options.selections[view].first -=
								(cmpPair->options.selections[view].first - startLine);
				}

				selectionsAdjusted = true;
			}

			// Handle the special case when the end of the selection compare is diff-equalized -
			// the insert part of the replace notification then needs to increase the selection accordingly to
			// include the equalized diff
			if (cmpPair->inEqualizeMode && (cmpPair->options.selections[view].second == startLine - 1) &&
					(notifyCode->linesAdded > 0))
				--startLine;

			if (cmpPair->options.selections[view].second >= startLine)
			{
				if (notifyCode->linesAdded > 0)
				{
					cmpPair->options.selections[view].second += notifyCode->linesAdded;
				}
				else
				{
					if (cmpPair->options.selections[view].second >= endLine)
						cmpPair->options.selections[view].second += notifyCode->linesAdded;
					else
						cmpPair->options.selections[view].second -=
								(cmpPair->options.selections[view].second - startLine + 1);
				}

				selectionsAdjusted = true;
			}

			if (!cmpPair->inEqualizeMode &&
				cmpPair->options.selections[view].second < cmpPair->options.selections[view].first)
			{
				clearComparePair(getCurrentBuffId());
				return;
			}

			LOGDIF(LOG_NOTIF, selectionsAdjusted, "Selection adjusted.\n");
		}

		if (cmpPair->options.recompareOnChange)
		{
			if (notifyCode->linesAdded)
				cmpPair->autoRecompareDelay = 500;
			else
				// Leave bigger delay before re-comparing if change is on single line because the user might be typing
				// and we should try to avoid interrupting / interfering
				cmpPair->autoRecompareDelay = 1500;

			if (!storedLocation)
				storedLocation = std::make_unique<ViewLocation>(view);

			return;
		}

		if (notifyCode->linesAdded)
		{
			intptr_t startLine = CallScintilla(view, SCI_LINEFROMPOSITION, notifyCode->position, 0);

			if (!undo)
			{
				if (!cmpPair->options.selectionCompare || selectionsAdjusted)
				{
					cmpPair->adjustAlignment(view, startLine, notifyCode->linesAdded);

					LOGD(LOG_NOTIF, "Alignment adjusted.\n");
				}
			}

			delayedAlign.post(300, true);

			if (Settings.ShowNavBar && !cmpPair->inEqualizeMode)
				NavDlg.Show();
		}

		if (updateStatus)
			cmpPair->setStatus();
	}
}


void onMarginClick(HWND view, intptr_t pos, int keyMods)
{
	if (keyMods & SCMOD_ALT)
		return;

	if (Settings.HideNewLines || Settings.HideChangedLines || Settings.HideMovedLines)
	{
		::MessageBoxW(nppData._nppHandle, Strings::get()["MSG_HIDDEN_NOT_POSSIBLE"].c_str(), PLUGIN_NAME, MB_OK);
		return;
	}

	const int viewId = getViewIdSafe(view);

	if (viewId < 0)
		return;

	if ((keyMods & SCMOD_CTRL) && CallScintilla(viewId, SCI_GETREADONLY, 0, 0))
		return;

	CompareList_t::iterator cmpPair = getCompareBySciDoc(getDocId(viewId));
	if (cmpPair == compareList.end())
		return;

	const intptr_t line = CallScintilla(viewId, SCI_LINEFROMPOSITION, pos, 0);

	if (!isLineMarked(viewId, line, MARKER_MASK_LINE) && !isLineAnnotated(viewId, line))
		return;

	int mark = MARKER_MASK_LINE;

	if (keyMods & SCMOD_SHIFT)
	{
		const int markerMask = static_cast<int>(CallScintilla(viewId, SCI_MARKERGET, line, 0));

		if (markerMask & (1 << MARKER_CHANGED_LINE))
			mark = (1 << MARKER_CHANGED_LINE);
		else if (markerMask & MARKER_MASK_LINE)
			mark = (1 << MARKER_ADDED_LINE) | (1 << MARKER_REMOVED_LINE) | (1 << MARKER_MOVED_LINE);
	}

	const int otherViewId = getOtherViewId(viewId);

	if ((keyMods & SCMOD_SHIFT) && (mark == (1 << MARKER_CHANGED_LINE)))
	{
		const intptr_t startPos		= getLineStart(viewId, line);
		const intptr_t endPos		= getLineStart(viewId, line + 1);
		const intptr_t otherLine	= otherViewMatchingLine(viewId, line, 0, true);

		if ((otherLine < 0) ||
			!(CallScintilla(otherViewId, SCI_MARKERGET, otherLine, 0) & (1 << MARKER_CHANGED_LINE)))
		{
			if (!(keyMods & SCMOD_CTRL))
				setSelection(viewId, startPos, endPos);

			temporaryRangeSelect(-1);

			return;
		}

		if (!(keyMods & SCMOD_CTRL))
		{
			setSelection(viewId, startPos, endPos);

			temporaryRangeSelect(otherViewId,
					getLineStart(otherViewId, otherLine), getLineStart(otherViewId, otherLine + 1));

			return;
		}

		const auto text =
				getText(otherViewId, getLineStart(otherViewId, otherLine), getLineStart(otherViewId, otherLine + 1));

		const bool lineFolded = isLineFoldedFoldPoint(otherViewId, otherLine);
		const bool lastMarked = (endPos == CallScintilla(viewId, SCI_GETLENGTH, 0, 0));

		ScopedIncrementerInt		inEqualize(cmpPair->inEqualizeMode);
		ScopedViewUndoAction		scopedUndo(viewId);
		ScopedFirstVisibleLineStore	firstVisLine(viewId);

		clearSelection(viewId);
		temporaryRangeSelect(-1);

		if (!cmpPair->options.recompareOnChange)
		{
			copiedSectionMarks = getMarkers(otherViewId, otherLine, 1, MARKER_MASK_ALL);
			clearAnnotation(otherViewId, otherLine);
		}
		else
		{
			clearMarks(otherViewId, otherLine, 1);
		}

		deleteRange(viewId, startPos, endPos);

		if (lastMarked)
			clearMarks(viewId, line);

		CallScintilla(viewId, SCI_INSERTTEXT, startPos, (LPARAM)text.data());

		if (lineFolded)
			CallScintilla(viewId, SCI_FOLDLINE, line, SC_FOLDACTION_CONTRACT);

		if (Settings.FollowingCaret)
			CallScintilla(viewId, SCI_SETEMPTYSELECTION, startPos, 0);

		if (!isLineVisible(viewId, line))
			firstVisLine.set(getVisibleFromDocLine(viewId, line));

		if (!cmpPair->options.recompareOnChange)
		{
			if (Settings.HideMatches || Settings.HideNewLines || Settings.HideChangedLines || Settings.HideMovedLines)
				alignDiffs(cmpPair);

			if (Settings.ShowNavBar)
				NavDlg.Show();
		}

		return;
	}

	std::pair<intptr_t, intptr_t> markedRange = getMarkedSection(viewId, line, line, mark);

	if ((markedRange.first < 0) && !(keyMods & SCMOD_SHIFT))
		markedRange = getMarkedSection(viewId, line + 1, line + 1, mark);

	if (cmpPair->options.findUniqueMode)
	{
		if (!(keyMods & SCMOD_CTRL))
		{
			if (markedRange.first >= 0)
				setSelection(viewId, markedRange.first, markedRange.second);
			else
				clearSelection(viewId);

			temporaryRangeSelect(-1);
		}

		return;
	}

	intptr_t lastLine = getEndNotEmptyLine(viewId);

	std::pair<intptr_t, intptr_t> otherMarkedRange;

	if (markedRange.first < 0)
	{
		otherMarkedRange.first = otherViewMatchingLine(viewId, line, getWrapCount(viewId, line));

		if (line < lastLine)
			otherMarkedRange.second	= otherViewMatchingLine(viewId, line + 1, -1);
		else
			otherMarkedRange.second = getEndLine(otherViewId);
	}
	else
	{
		intptr_t startLine		= CallScintilla(viewId, SCI_LINEFROMPOSITION, markedRange.first, 0);
		intptr_t startOffset	= 0;

		if (!Settings.HideMatches && (startLine > 1) && isLineAnnotated(viewId, startLine - 1))
		{
			--startLine;
			startOffset = getWrapCount(viewId, startLine);
		}

		intptr_t endLine = CallScintilla(viewId, SCI_LINEFROMPOSITION, markedRange.second, 0);

		if (getLineStart(viewId, endLine) == markedRange.second)
			--endLine;

		intptr_t endOffset = getWrapCount(viewId, endLine) - 1;

		if (!Settings.HideMatches)
			endOffset += getLineAnnotation(viewId, endLine);

		otherMarkedRange.first = otherViewMatchingLine(viewId, startLine, startOffset, true);

		if (otherMarkedRange.first < 0)
			otherMarkedRange.first = otherViewMatchingLine(viewId, startLine, startOffset) + 1;

		otherMarkedRange.second	=
				(line < lastLine) ? otherViewMatchingLine(viewId, endLine, endOffset) : getEndLine(otherViewId);
	}

	if (!Settings.HideMatches)
	{
		for (; (otherMarkedRange.first <= otherMarkedRange.second) &&
				!isLineMarked(otherViewId, otherMarkedRange.first, mark); ++otherMarkedRange.first);
	}
	else
	{
		intptr_t otherLine = otherMarkedRange.second;

		for (; (otherLine > otherMarkedRange.first) && isLineMarked(otherViewId, otherLine, mark); --otherLine);

		if (otherLine > otherMarkedRange.first)
			otherMarkedRange.first = otherLine + 1;
	}

	if (otherMarkedRange.first > otherMarkedRange.second)
	{
		otherMarkedRange.first = -1;
	}
	else
	{
		for (; (otherMarkedRange.second >= otherMarkedRange.first) &&
				!isLineMarked(otherViewId, otherMarkedRange.second, mark); --otherMarkedRange.second);

		if (otherMarkedRange.second < otherMarkedRange.first)
		{
			otherMarkedRange.first = -1;
		}
		else
		{
			otherMarkedRange.first	= getLineStart(otherViewId, otherMarkedRange.first);
			otherMarkedRange.second	= getLineStart(otherViewId, otherMarkedRange.second + 1);
		}
	}

	if (!(keyMods & SCMOD_CTRL))
	{
		if (markedRange.first >= 0)
			setSelection(viewId, markedRange.first, markedRange.second);
		else
			clearSelection(viewId);

		if (otherMarkedRange.first >= 0)
			temporaryRangeSelect(otherViewId, otherMarkedRange.first, otherMarkedRange.second);
		else
			temporaryRangeSelect(-1);

		return;
	}

	ScopedIncrementerInt		inEqualize(cmpPair->inEqualizeMode);
	ScopedViewUndoAction		scopedUndo(viewId);
	ScopedFirstVisibleLineStore	firstVisLine(viewId);

	clearSelection(viewId);
	temporaryRangeSelect(-1);

	if (otherMarkedRange.first >= 0)
	{
		const intptr_t otherStartLine	= CallScintilla(otherViewId, SCI_LINEFROMPOSITION, otherMarkedRange.first, 0);
		intptr_t otherEndLine			= CallScintilla(otherViewId, SCI_LINEFROMPOSITION, otherMarkedRange.second, 0);

		if (otherMarkedRange.second == CallScintilla(otherViewId, SCI_GETLENGTH, 0, 0))
			++otherEndLine;

		if (!cmpPair->options.recompareOnChange)
		{
			copiedSectionMarks =
					getMarkers(otherViewId, otherStartLine, otherEndLine - otherStartLine, MARKER_MASK_ALL);
			clearAnnotations(otherViewId, otherStartLine, otherEndLine - otherStartLine);
		}
		else
		{
			clearMarks(otherViewId, otherStartLine, otherEndLine - otherStartLine);
		}
	}

	lastLine = getEndLine(viewId);

	if (markedRange.first >= 0)
	{
		const intptr_t startLine = CallScintilla(viewId, SCI_LINEFROMPOSITION, markedRange.first, 0);

		if ((otherMarkedRange.first >= 0) && (startLine > 0))
			clearAnnotation(viewId, startLine - 1);

		const bool lastMarked = (markedRange.second == CallScintilla(viewId, SCI_GETLENGTH, 0, 0));

		if (Settings.FollowingCaret)
			CallScintilla(viewId, SCI_SETEMPTYSELECTION, markedRange.first, 0);

		if (!isLineVisible(viewId, startLine))
			firstVisLine.set(getVisibleFromDocLine(viewId, startLine));

		deleteRange(viewId, markedRange.first, markedRange.second);

		if (lastMarked)
			clearMarks(viewId, CallScintilla(viewId, SCI_LINEFROMPOSITION, markedRange.first, 0));
	}

	if (otherMarkedRange.first >= 0)
	{
		const intptr_t otherStartLine = CallScintilla(otherViewId, SCI_LINEFROMPOSITION, otherMarkedRange.first, 0);

		bool copyOtherTillEnd = false;

		intptr_t startPos = markedRange.first;

		if (startPos < 0)
		{
			if (line < lastLine)
			{
				if (!Settings.HideMatches)
				{
					startPos = line + 1;
				}
				else
				{
					if (otherStartLine < 0)
						return;

					startPos = getAlignmentLine(cmpPair->summary.alignmentInfo, otherViewId, otherStartLine);

					if (startPos < 0)
						return;
				}

				startPos = getLineStart(viewId, startPos);
			}
			else
			{
				startPos = getLineEnd(viewId, line);

				if (otherStartLine > 0)
					otherMarkedRange.first = getLineEnd(otherViewId, otherStartLine - 1);

				copyOtherTillEnd = true;
			}

			clearAnnotation(viewId, line);
		}
		else if (CallScintilla(viewId, SCI_LINEFROMPOSITION, markedRange.first, 0) == lastLine)
		{
			copyOtherTillEnd = true;
		}

		const bool endLineFolded = (!copyOtherTillEnd && isLineFoldedFoldPoint(otherViewId,
				CallScintilla(otherViewId, SCI_LINEFROMPOSITION, otherMarkedRange.second, 0) - 1));

		if (copyOtherTillEnd)
			otherMarkedRange.second = getLineEnd(otherViewId, getEndLine(otherViewId));

		const auto text = getText(otherViewId, otherMarkedRange.first, otherMarkedRange.second);

		if (otherStartLine > 0)
			clearAnnotation(otherViewId, otherStartLine - 1);

		CallScintilla(viewId, SCI_INSERTTEXT, startPos, (LPARAM)text.data());

		if (endLineFolded)
			CallScintilla(viewId, SCI_FOLDLINE, CallScintilla(viewId, SCI_LINEFROMPOSITION,
					startPos + otherMarkedRange.second - otherMarkedRange.first, 0) - 1, SC_FOLDACTION_CONTRACT);

		if (Settings.FollowingCaret)
			CallScintilla(viewId, SCI_SETEMPTYSELECTION, startPos, 0);

		const intptr_t firstLine = CallScintilla(viewId, SCI_LINEFROMPOSITION, startPos, 0);

		if (!isLineVisible(viewId, firstLine))
			firstVisLine.set(getVisibleFromDocLine(viewId, firstLine));
	}

	if (!cmpPair->options.recompareOnChange)
	{
		if (Settings.HideMatches || Settings.HideNewLines || Settings.HideChangedLines || Settings.HideMovedLines)
			alignDiffs(cmpPair);

		if (Settings.ShowNavBar)
			NavDlg.Show();
	}
}


void onSciZoom()
{
	CompareList_t::iterator cmpPair = getCompare(getCurrentBuffId());
	if (cmpPair == compareList.end())
		return;

	ScopedIncrementerInt incr(notificationsLock);

	// sync both views zoom
	const int zoom = static_cast<int>(CallScintilla(getCurrentViewId(), SCI_GETZOOM, 0, 0));
	CallScintilla(getOtherViewId(), SCI_SETZOOM, zoom, 0);

	NppState::get().setCompareZoom(zoom);
}


void DelayedActivate::operator()()
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	LOGDB(LOG_NOTIF, buffId, "Activate\n");

	if (buffId != currentlyActiveBuffID)
	{
		const ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);

		ScopedIncrementerInt incr(notificationsLock);

		// When compared file is activated make sure its corresponding pair file is also active in the other view
		if (getDocId(getOtherViewId()) != otherFile.sciDoc)
		{
			activateBufferID(otherFile.buffId);
			activateBufferID(buffId);
		}

		currentlyActiveBuffID = buffId;

		comparedFileActivated();

		onSciUpdateUI(getView(viewIdFromBuffId(buffId)));
	}
	else
	{
		// NPPN_BUFFERACTIVATED for the same buffer is received if file is reloaded or if we try to close an
		// unsaved file. We try to distinguish here the reason for the notification - if file is saved then it
		// seems reloaded and we want to update the compare.
		if (isCurrentFileSaved())
			delayedRecompare.post(30);
	}
}


void onBufferActivated(LRESULT buffId)
{
	delayedAlign.cancel();
	delayedRecompare.cancel();
	delayedActivation.cancel();

	// If compared pair was not active explicitly release mouse key as it might have been pressed and make a
	// false selection when files are activated and compare mode is set
	if (!NppState::get().compareMode)
	{
		INPUT inputs[1] = {};
		::ZeroMemory(inputs, sizeof(inputs));

		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dwFlags = ::GetSystemMetrics(SM_SWAPBUTTON) ? MOUSEEVENTF_RIGHTUP : MOUSEEVENTF_LEFTUP;

		::SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
	}

	ScopedIncrementerInt incr(notificationsLock);

	LOGDB(LOG_NOTIF, buffId, "onBufferActivated()\n");

	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
	{
		NppState::get().setNormalMode();
		setNormalView(getCurrentViewId());
		resetCompareView(getOtherViewId());

		currentlyActiveBuffID = buffId;
	}
	else
	{
		delayedActivation.buffId = buffId;
		delayedActivation.post(30);
	}
}


void DelayedClose::operator()()
{
	const LRESULT currentBuffId = getCurrentBuffId();

	ScopedIncrementerInt incr(notificationsLock);

	for (int i = static_cast<int>(closedBuffs.size()) - 1; i >= 0; --i)
	{
		CompareList_t::iterator cmpPair = getCompare(closedBuffs[i]);
		if (cmpPair == compareList.end())
			continue;

		ComparedFile& closedFile = cmpPair->getFileByBuffId(closedBuffs[i]);
		ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(closedBuffs[i]);

		if (closedFile.isTemp && closedFile.isOpen())
			closedFile.close();

		if (otherFile.isTemp)
		{
			if (otherFile.isOpen())
			{
				LOGDB(LOG_NOTIF, otherFile.buffId, "Close\n");

				otherFile.close();
			}
		}
		else
		{
			if (otherFile.isOpen())
				otherFile.restore();
		}

		compareList.erase(cmpPair);
	}

	closedBuffs.clear();

	activateBufferID(currentBuffId);
	onBufferActivated(currentBuffId);

	// If it is the last file and it is not in the main view - move it there
	if (getNumberOfFiles() == 1 && getCurrentViewId() == SUB_VIEW)
	{
		::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

		const LRESULT newBuffId = getCurrentBuffId();

		activateBufferID(currentBuffId);
		moveFileToOtherView();
		activateBufferID(newBuffId);
		::SendMessageW(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_CLOSE);
	}
}


void onFileBeforeClose(LRESULT buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	delayedAlign.cancel();
	delayedRecompare.cancel();
	delayedActivation.cancel();

	delayedClosure.cancel();
	delayedClosure.closedBuffs.push_back(buffId);

	const LRESULT currentBuffId = getCurrentBuffId();

	ScopedIncrementerInt incr(notificationsLock);

	ComparedFile& closedFile = cmpPair->getFileByBuffId(buffId);
	closedFile.onBeforeClose();

	if (cmpPair->relativePos && (closedFile.originalViewId == viewIdFromBuffId(buffId)))
	{
		ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);

		otherFile.originalPos = posFromBuffId(buffId) + cmpPair->relativePos;

		if (cmpPair->relativePos > 0)
			--otherFile.originalPos;
		else
			++otherFile.originalPos;

		if (otherFile.originalPos < 0)
			otherFile.originalPos = 0;
	}

	if (currentBuffId != buffId)
		activateBufferID(currentBuffId);

	delayedClosure.post(30);
}


void onFileSaved(LRESULT buffId)
{
	CompareList_t::iterator cmpPair = getCompare(buffId);
	if (cmpPair == compareList.end())
		return;

	const ComparedFile& otherFile = cmpPair->getOtherFileByBuffId(buffId);

	const LRESULT currentBuffId = getCurrentBuffId();
	const bool pairIsActive = (currentBuffId == buffId || currentBuffId == otherFile.buffId);

	ScopedIncrementerInt incr(notificationsLock);

	if (!pairIsActive)
		activateBufferID(buffId);
	else if (cmpPair->options.recompareOnChange && cmpPair->autoRecompareDelay)
		delayedRecompare.post(30);

	if (otherFile.isTemp == LAST_SAVED_TEMP)
	{
		HWND hNppTabBar = NppTabHandleGetter::get(otherFile.compareViewId);

		if (hNppTabBar)
		{
			wchar_t tabText[MAX_PATH];

			TCITEMW tab;
			tab.mask = TCIF_TEXT;
			tab.pszText = tabText;
			tab.cchTextMax = _countof(tabText);

			const int tabPos = posFromBuffId(otherFile.buffId);
			TabCtrl_GetItem(hNppTabBar, tabPos, &tab);

			wcscat_s(tabText, _countof(tabText), Strings::get()["MARK_OUTDATED"].c_str());

			::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, TRUE);

			TabCtrl_SetItem(hNppTabBar, tabPos, &tab);

			::SendMessageW(nppData._nppHandle, NPPM_HIDETABBAR, 0, FALSE);
		}
	}

	if (!pairIsActive)
	{
		activateBufferID(currentBuffId);
		onBufferActivated(currentBuffId);
	}
}

} // anonymous namespace


void ToggleNavigationBar()
{
	Settings.ShowNavBar = !Settings.ShowNavBar;
	::SendMessageW(nppData._nppHandle, NPPM_SETMENUITEMCHECK, funcItem[CMD_NAV_BAR]._cmdID,
			(LPARAM)Settings.ShowNavBar);
	Settings.markAsDirty();

	if (NppState::get().compareMode)
	{
		if (Settings.ShowNavBar)
			showNavBar();
		else
			NavDlg.Hide();
	}
}


// Main plugin DLL function
BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD  reasonForCall, LPVOID)
 {
	hInstance = hinstDLL;

	switch (reasonForCall)
	{
		case DLL_PROCESS_ATTACH:
			createMenu();
		break;

		case DLL_PROCESS_DETACH:
			deinitPlugin();
		break;

		case DLL_THREAD_ATTACH:
		break;

		case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}


//
// Notepad++ API functions below
//

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;

	sciFunc		= (SciFnDirect)::SendMessageW(notpadPlusData._scintillaMainHandle, SCI_GETDIRECTFUNCTION, 0, 0);
	sciPtr[0]	= (sptr_t)::SendMessageW(notpadPlusData._scintillaMainHandle, SCI_GETDIRECTPOINTER, 0, 0);
	sciPtr[1]	= (sptr_t)::SendMessageW(notpadPlusData._scintillaSecondHandle, SCI_GETDIRECTPOINTER, 0, 0);

	if (!sciFunc || !sciPtr[0] || !sciPtr[1])
	{
		exit(EXIT_FAILURE);
	}

	Settings.load();

	NavDlg.init(hInstance);
}


extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return PLUGIN_NAME;
}


extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int* nbF)
{
	*nbF = NB_MENU_COMMANDS;
	return funcItem;
}


extern "C" __declspec(dllexport) void beNotified(SCNotification* notifyCode)
{
	switch (notifyCode->nmhdr.code)
	{
		// Vertical scroll sync
		case SCN_UPDATEUI:
			if (NppState::get().compareMode && !notificationsLock && !storedLocation &&
					(notifyCode->updated & (SC_UPDATE_SELECTION | SC_UPDATE_V_SCROLL)) &&
					!delayedRecompare && !delayedActivation && !delayedClosure)
				onSciUpdateUI((HWND)notifyCode->nmhdr.hwndFrom);
		break;

		// Handle word-wrap refresh
		case SCN_PAINTED:
			if (NppState::get().compareMode && !notificationsLock && storedLocation &&
					!delayedRecompare && !delayedActivation && !delayedClosure)
				onSciPaint();
		break;

		// This is used to monitor fold state and deletion of lines to properly clear their compare markings
		case SCN_MODIFIED:
			if (NppState::get().compareMode && !notificationsLock)
				onSciModified(notifyCode);
		break;

		case NPPN_BUFFERACTIVATED:
			if (!compareList.empty() && !notificationsLock && !delayedClosure)
				onBufferActivated(notifyCode->nmhdr.idFrom);
		break;

		// Copy/equalize diffs
		case SCN_MARGINCLICK:
			if (NppState::get().compareMode && !notificationsLock && (notifyCode->margin == marginNum) &&
					!delayedRecompare && !delayedAlign && !delayedActivation && !delayedClosure)
				onMarginClick((HWND)notifyCode->nmhdr.hwndFrom, notifyCode->position, notifyCode->modifiers);
		break;

		case NPPN_FILEBEFORECLOSE:
			if (newCompare && (newCompare->pair.file[0].buffId == static_cast<LRESULT>(notifyCode->nmhdr.idFrom)))
				newCompare = nullptr;
#ifdef DLOG
			else if (dLogBuf == static_cast<LRESULT>(notifyCode->nmhdr.idFrom))
				dLogBuf = -1;
#endif
			else if (!compareList.empty() && !notificationsLock)
				onFileBeforeClose(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_FILESAVED:
			if (!compareList.empty() && !notificationsLock)
				onFileSaved(notifyCode->nmhdr.idFrom);
		break;

		case NPPN_GLOBALMODIFIED:
			if (!compareList.empty() && !notificationsLock)
			{
				const LRESULT changedBuffId = reinterpret_cast<LRESULT>(notifyCode->nmhdr.hwndFrom);

				CompareList_t::iterator cmpPair = getCompare(changedBuffId);

				if (cmpPair != compareList.end())
				{
					if (cmpPair->options.recompareOnChange)
					{
						cmpPair->autoRecompareDelay = 200;
					}
					else
					{
						cmpPair->setCompareDirty(true);

						if (changedBuffId == getCurrentBuffId())
							cmpPair->setStatus();
					}
				}
			}
		break;

		case SCN_ZOOM:
			if (!notificationsLock)
			{
				if (NppState::get().compareMode)
				{
					onSciZoom();
				}
				else
				{
					NppState::get().setMainZoom(static_cast<int>(CallScintilla(MAIN_VIEW, SCI_GETZOOM, 0, 0)));
					NppState::get().setSubZoom(static_cast<int>(CallScintilla(SUB_VIEW, SCI_GETZOOM, 0, 0)));
				}
			}
		break;

		case NPPN_LANGCHANGED:
			if (NppState::get().compareMode)
			{
				CompareList_t::iterator	cmpPair = getCompare(notifyCode->nmhdr.idFrom);
				if (cmpPair != compareList.end())
					cmpPair->setStatus();
			}
		break;

		case NPPN_WORDSTYLESUPDATED:
		case NPPN_DARKMODECHANGED:
		{
			const bool darkMode = isDarkMode();

			if (darkMode)
				Settings.useDarkColors();
			else
				Settings.useLightColors();

			if (!compareList.empty())
			{
				setStyles(Settings);
				NavDlg.SetColors(Settings.colors(), darkMode);

				if (NppState::get().compareMode)
				{
					setCompareView(MAIN_VIEW, !Settings.HideMargin,
							Settings.colors().blank, Settings.colors().caret_line_transparency);
					setCompareView(SUB_VIEW, !Settings.HideMargin,
							Settings.colors().blank, Settings.colors().caret_line_transparency);
				}
			}
		}
		// Intentional fall-through

		case NPPN_TOOLBARICONSETCHANGED:
			NppState::get().updateMenuAndToolbar();
		break;

		case NPPN_TBMODIFICATION:
			onToolBarReady();
		break;

		case NPPN_READY:
			onNppReady();
		break;

		// case NPPN_CMDLINEPLUGINMSG:
			// onPluginMsg(reinterpret_cast<wchar_t*>(notifyCode->nmhdr.idFrom));
		// break;

		case NPPN_NATIVELANGCHANGED:
			NppState::get().updateLocalization();
		break;

		case NPPN_BEFORESHUTDOWN:
			ClearAllCompares();
		break;

		case NPPN_SHUTDOWN:
			Settings.save();
			deinitPlugin();
		break;
	}
}


extern "C" __declspec(dllexport) LRESULT messageProc(UINT, WPARAM, LPARAM)
{
	return TRUE;
}


extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
