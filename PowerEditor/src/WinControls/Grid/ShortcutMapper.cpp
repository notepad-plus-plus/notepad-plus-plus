// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "ShortcutMapper.h"
#include "Notepad_plus.h"


using namespace std;

void ShortcutMapper::initTabs()
{
	_hTabCtrl = ::GetDlgItem(_hSelf, IDC_BABYGRID_TABBAR);
	TCITEM tie{};
	tie.mask = TCIF_TEXT;

	for (size_t i = 0; i < _nbTab; ++i)
	{
		_tabNames[i] = getTabString(i);

		tie.pszText = const_cast<LPWSTR>(_tabNames[i].c_str());
		::SendMessage(_hTabCtrl, TCM_INSERTITEM, i, reinterpret_cast<LPARAM>(&tie));
	}

	TabCtrl_SetCurSel(_hTabCtrl, static_cast<int>(_currentState));
}

void ShortcutMapper::getClientRect(RECT& rc) const
{
	Window::getClientRect(rc);

	RECT rcTab{};
	getMappedChildRect(IDC_BABYGRID_TABBAR, rcTab);

	RECT rcInfo{};
	HWND hInfo = ::GetDlgItem(_hSelf, IDC_BABYGRID_INFO);
	::GetWindowRect(hInfo, &rcInfo);
	::MapWindowPoints(nullptr, _hSelf, reinterpret_cast<LPPOINT>(&rcInfo), 2);

	RECT rcFilterEdit{};
	HWND hFilterEdit = ::GetDlgItem(_hSelf, IDC_BABYGRID_FILTER);
	::GetWindowRect(hFilterEdit, &rcFilterEdit);
	::MapWindowPoints(nullptr, _hSelf, reinterpret_cast<LPPOINT>(&rcFilterEdit), 2);

	const UINT dpi = _dpiManager.getDpiForWindow(_hSelf);
	const LONG padding = _dpiManager.scale(6, dpi);
	const LONG gap = rcFilterEdit.top - rcInfo.bottom;

	rc.top = rcTab.bottom;
	rc.bottom = rcInfo.top - gap;
	::InflateRect(&rc, -padding, 0);
}

wstring ShortcutMapper::getTabString(size_t i) const
{
	if (i >= _nbTab)
		return L"";

	NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
	switch (i)
	{
		case 1:
			return nativeLangSpeaker->getShortcutMapperLangStr("MacrosTab", L"Macros");

		case 2:
			return nativeLangSpeaker->getShortcutMapperLangStr("RunCommandsTab", L"Run commands");

		case 3:
			return nativeLangSpeaker->getShortcutMapperLangStr("PluginCommandsTab", L"Plugin commands");

		case 4:
			return nativeLangSpeaker->getShortcutMapperLangStr("ScintillaCommandsTab", L"Scintilla commands");

		default: //0
			return nativeLangSpeaker->getShortcutMapperLangStr("MainMenuTab", L"Main menu");
	}
}


void ShortcutMapper::initBabyGrid()
{
	RECT rect{};
	getClientRect(rect);

	_lastHomeRow.resize(5, 1);
	_lastCursorRow.resize(5, 1);

	_hGridFonts.resize(MAX_GRID_FONTS);
	LOGFONT lf{ _dpiManager.getDefaultGUIFontForDpi() };
	lf.lfHeight = _dpiManager.scaleFont(10);
	_hGridFonts.at(GFONT_ROWS) = ::CreateFontIndirect(&lf);
	lf.lfHeight = _dpiManager.scaleFont(12);
	lf.lfWeight = FW_BOLD;
	_hGridFonts.at(GFONT_HEADER) = ::CreateFontIndirect(&lf);
	
	_babygrid.init(_hInst, _hSelf, IDD_BABYGRID_ID1);

	NppDarkMode::setDarkScrollBar(_babygrid.getHSelf());

	_babygrid.setHeaderFont(_hGridFonts.at(GFONT_HEADER));
	_babygrid.setRowFont(_hGridFonts.at(GFONT_ROWS));
	
	_babygrid.reSizeToWH(rect);
	_babygrid.hideCursor();
	_babygrid.makeColAutoWidth(true);
	_babygrid.setAutoRow(true);
	_babygrid.setColsNumbered(false);
	_babygrid.setColWidth(0, _dpiManager.scale(30));  // Force the first col to be small, others col will be automatically sized
	_babygrid.setHeaderHeight(_dpiManager.scale(21));
	_babygrid.setRowHeight(_dpiManager.scale(21));

	if (NppDarkMode::isEnabled())
	{
		_babygrid.setTextColor(NppDarkMode::getDarkerTextColor());
		_babygrid.setHighlightTextColor(NppDarkMode::getTextColor());
		_babygrid.setTitleTextColor(NppDarkMode::getTextColor());

		_babygrid.setUnprotectColor(NppDarkMode::getBackgroundColor());
		_babygrid.setTitleColor(NppDarkMode::getBackgroundColor());

		_babygrid.setBackgroundColor(NppDarkMode::getDlgBackgroundColor());

		_babygrid.setHighlightColor(NppDarkMode::getHotBackgroundColor());
		_babygrid.setHighlightColorNoFocus(NppDarkMode::getCtrlBackgroundColor());
		_babygrid.setProtectColor(NppDarkMode::getErrorBackgroundColor());
		_babygrid.setHighlightColorProtect(RGB(244, 10, 20));
		_babygrid.setHighlightColorProtectNoFocus(RGB(230, 100, 110));

		_babygrid.setGridlinesColor(NppDarkMode::getEdgeColor());
		_babygrid.setTitleGridlinesColor(NppDarkMode::getHotEdgeColor());
	}
	else
	{
		_babygrid.setTextColor(RGB(0, 0, 0));
		_babygrid.setHighlightTextColor(RGB(255, 255, 255));
		_babygrid.setTitleTextColor(RGB(0, 0, 0));

		_babygrid.setUnprotectColor(RGB(255, 255, 255));
		_babygrid.setTitleColor(::GetSysColor(COLOR_BTNFACE));

		_babygrid.setBackgroundColor(::GetSysColor(COLOR_BTNFACE));

		_babygrid.setHighlightColor(RGB(0, 0, 128));
		_babygrid.setHighlightColorNoFocus(RGB(200, 200, 210));
		_babygrid.setProtectColor(RGB(255, 130, 120));
		_babygrid.setHighlightColorProtect(RGB(244, 10, 20));
		_babygrid.setHighlightColorProtectNoFocus(RGB(230, 194, 190));

		_babygrid.setGridlinesColor(RGB(220, 220, 220));
		_babygrid.setTitleGridlinesColor(RGB(120, 120, 120));
	}

	NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
	nativeLangSpeaker->changeDlgLang(_hSelf, "ShortcutMapper");
	_conflictInfoOk = nativeLangSpeaker->getShortcutMapperLangStr("ConflictInfoOk", L"No shortcut conflicts for this item.");
	_conflictInfoEditing = nativeLangSpeaker->getShortcutMapperLangStr("ConflictInfoEditing", L"No conflicts . . .");
}

wstring ShortcutMapper::getTextFromCombo(HWND hCombo)
{
	const int NB_MAX(128);
	wchar_t str[NB_MAX](L"\0");
	::SendMessage(hCombo, WM_GETTEXT, NB_MAX, reinterpret_cast<LPARAM>(str));
	wstring res(str);
	return stringToLower(res);
}

bool ShortcutMapper::isFilterValid(Shortcut sc)
{
	// all words in _shortcutFilter must be in the name or keycombo
	// For example, the shortcut with name "foo bar baz" and keycombo "Ctrl+A"
	// would be matched by the filter "foo ctrl" and the filter "bar +a"
	// but *not* by the filter "foo shift" or the filter "quz +a"
	size_t filterSize = _shortcutFilter.size();
	if (filterSize == 0)
		return true;

	wstring shortcut_name = stringToLower(string2wstring(sc.getName(), CP_UTF8));
	wstring shortcut_value = stringToLower(string2wstring(sc.toString(), CP_UTF8));

	for (size_t i = 0; i < filterSize; ++i)
	{
		wstring filterWord = _shortcutFilter.at(i);
		// every word must be matched by keycombo or name
		if (shortcut_name.find(filterWord) == std::string::npos &&
			shortcut_value.find(filterWord) == std::string::npos)
			return false;
	}
	return true;
}

bool ShortcutMapper::isFilterValid(PluginCmdShortcut sc)
{
	// all words in _shortcutFilter must be in the name or the keycombo or the plugin name
	// For example, the shortcut with name "foo bar baz" and keycombo "Ctrl+A" and plugin "BlahLint"
	// would be matched by the filter "foo ctrl" and the filter "bar +a blah"
	// but *not* by the filter "baz shift" or the filter "lint quz" 
	size_t filterSize = _shortcutFilter.size();
	if (filterSize == 0)
		return true;

	wstring shortcut_name = stringToLower(string2wstring(sc.getName(), CP_UTF8));
	wstring shortcut_value = stringToLower(string2wstring(sc.toString(), CP_UTF8));
	wstring module_name = stringToLower(string2wstring(sc.getModuleName(), CP_UTF8));
	
	for (size_t i = 0; i < filterSize; ++i)
	{
		wstring filterWord = _shortcutFilter.at(i);
		// every word must be matched by keycombo or name or plugin name
		if (shortcut_name.find(filterWord) == std::string::npos &&
			shortcut_value.find(filterWord) == std::string::npos &&
			module_name.find(filterWord) == std::string::npos)
			return false;
	}
	return true;
}

bool ShortcutMapper::isFilterValid(ScintillaKeyMap sc)
{
	// all words in _shortcutFilter must be in the name or the list of keycombos
	// For example, the shortcut with name "foo bar baz" and keycombo "Ctrl+A or Alt+G"
	// would be matched by the filter "foo ctrl" and the filter "bar alt or"
	// but *not* by the filter "foo shift" or the filter "quz +a"
	size_t filterSize = _shortcutFilter.size();
	if (filterSize == 0)
		return true;

	wstring shortcut_name = stringToLower(string2wstring(sc.getName(), CP_UTF8));
	wstring shortcut_value = stringToLower(string2wstring(sc.toString(), CP_UTF8));

	for (size_t i = 0; i < filterSize; ++i)
	{
		wstring filterWord = _shortcutFilter.at(i);
		// every word must be matched by keycombo or name
		if (shortcut_name.find(filterWord) == std::string::npos &&
			shortcut_value.find(filterWord) == std::string::npos)
			return false;
	}
	return true;
}

void ShortcutMapper::fillOutBabyGrid()
{
	NppParameters& nppParam = NppParameters::getInstance();
	_babygrid.clear();
	_babygrid.setInitialContent(true);
	_shortcutIndex.clear();

	size_t nbItems = 0;
	NativeLangSpeaker* nativeLangSpeaker = nppParam.getNativeLangSpeaker();
	wstring nameStr = nativeLangSpeaker->getShortcutMapperLangStr("ColumnName", L"Name");
	wstring shortcutStr = nativeLangSpeaker->getShortcutMapperLangStr("ColumnShortcut", L"Shortcut");

	_babygrid.setText(0, 1, nameStr.c_str());
	_babygrid.setText(0, 2, shortcutStr.c_str());
	
	switch(_currentState)
	{
		case STATE_MENU:
		{
			nbItems = nppParam.getUserShortcuts().size();
			_babygrid.setLineColNumber(nbItems, 3);
			wstring categoryStr = nativeLangSpeaker->getShortcutMapperLangStr("ColumnCategory", L"Category");
			_babygrid.setText(0, 3, categoryStr.c_str());
		}
		break;

		case STATE_MACRO:
		{
			nbItems = nppParam.getMacroList().size();
			_babygrid.setLineColNumber(nbItems, 2);
		}
		break;

		case STATE_USER:
		{
			nbItems = nppParam.getUserCommandList().size();
			_babygrid.setLineColNumber(nbItems, 2);
		}
		break;

		case STATE_PLUGIN:
		{
			nbItems = nppParam.getPluginCommandList().size();
			_babygrid.setLineColNumber(nbItems, 3);
			wstring pluginStr = nativeLangSpeaker->getShortcutMapperLangStr("ColumnPlugin", L"Plugin");
			_babygrid.setText(0, 3, pluginStr.c_str());
		}
		break;

		case STATE_SCINTILLA:
		{
			nbItems = nppParam.getScintillaKeyList().size();
			_babygrid.setLineColNumber(nbItems, 2);
		}
		break;
	}

	bool isMarker = false;
	size_t cs_index = 0;
	
	// make _shortcutFilter a list of the words in IDC_BABYGRID_FILTER
	wstring shortcutFilterStr = getTextFromCombo(::GetDlgItem(_hSelf, IDC_BABYGRID_FILTER));
	const wstring whitespace(L" ");
	std::vector<wstring> shortcutFilterWithEmpties;
	stringSplit(shortcutFilterStr, whitespace, shortcutFilterWithEmpties);
	// now add only the non-empty strings in the split list to _shortcutFilter
	_shortcutFilter = std::vector<wstring>();
	for (size_t i = 0; i < shortcutFilterWithEmpties.size(); ++i)
	{
		wstring filterWord = shortcutFilterWithEmpties.at(i);
		if (!filterWord.empty())
			_shortcutFilter.push_back(filterWord);
	}

	switch(_currentState) 
	{
		case STATE_MENU:
		{
			vector<CommandShortcut> & cshortcuts = nppParam.getUserShortcuts();
			cs_index = 1;
			for (size_t i = 0; i < nbItems; ++i)
			{
				if (isFilterValid(cshortcuts[i]))
				{
					if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
						isMarker = _babygrid.setMarker(true);

					_babygrid.setText(cs_index, 1, string2wstring(cshortcuts[i].getName(), CP_UTF8).c_str());
					if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
						_babygrid.setText(cs_index, 2, string2wstring(cshortcuts[i].toString(), CP_UTF8).c_str());

					_babygrid.setText(cs_index, 3, cshortcuts[i].getCategory());

					if (isMarker)
						isMarker = _babygrid.setMarker(false);
					_shortcutIndex.push_back(i);
					cs_index++;
				}
			}
			_babygrid.setLineColNumber(cs_index - 1 , 3);
			::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), true);
			::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), true);
			::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
		}
		break;

		case STATE_MACRO:
		{
			vector<MacroShortcut> & cshortcuts = nppParam.getMacroList();
			cs_index = 1;
			for (size_t i = 0; i < nbItems; ++i)
			{
				if (isFilterValid(cshortcuts[i]))
				{
					if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
						isMarker = _babygrid.setMarker(true);

					_babygrid.setText(cs_index, 1, string2wstring(cshortcuts[i].getName(), CP_UTF8).c_str());
					if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
						_babygrid.setText(cs_index, 2, string2wstring(cshortcuts[i].toString(), CP_UTF8).c_str());
	
					if (isMarker)
						isMarker = _babygrid.setMarker(false);
					_shortcutIndex.push_back(i);
					cs_index++;
				}
			}
			_babygrid.setLineColNumber(cs_index - 1 , 2);
            bool shouldBeEnabled = nbItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), shouldBeEnabled);
		}
		break;

		case STATE_USER:
		{
			vector<UserCommand> & cshortcuts = nppParam.getUserCommandList();
			cs_index = 1;
			for (size_t i = 0; i < nbItems; ++i)
			{
				if (isFilterValid(cshortcuts[i]))
				{
					if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
						isMarker = _babygrid.setMarker(true);

					_babygrid.setText(cs_index, 1, string2wstring(cshortcuts[i].getName(), CP_UTF8).c_str());
					if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
						_babygrid.setText(cs_index, 2, string2wstring(cshortcuts[i].toString(), CP_UTF8).c_str());
	
					if (isMarker)
						isMarker = _babygrid.setMarker(false);
					_shortcutIndex.push_back(i);
					cs_index++;
				}
			}
			_babygrid.setLineColNumber(cs_index - 1 , 2);

            bool shouldBeEnabled = nbItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), shouldBeEnabled);
		}
		break;

		case STATE_PLUGIN:
		{
			vector<PluginCmdShortcut> & cshortcuts = nppParam.getPluginCommandList();
			cs_index = 1;
			for (size_t i = 0; i < nbItems; ++i)
			{
				if (isFilterValid(cshortcuts[i]))
				{
					if (findKeyConflicts(nullptr, cshortcuts[i].getKeyCombo(), i))
						isMarker = _babygrid.setMarker(true);

					_babygrid.setText(cs_index, 1, string2wstring(cshortcuts[i].getName(), CP_UTF8).c_str());
					if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
						_babygrid.setText(cs_index, 2, string2wstring(cshortcuts[i].toString(), CP_UTF8).c_str());
					_babygrid.setText(cs_index, 3, string2wstring(cshortcuts[i].getModuleName(), CP_UTF8).c_str());
	
					if (isMarker)
						isMarker = _babygrid.setMarker(false);
					_shortcutIndex.push_back(i);
					cs_index++;
				}
			}
			_babygrid.setLineColNumber(cs_index - 1 , 3);
            bool shouldBeEnabled = nbItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
		}
		break;

		case STATE_SCINTILLA:
		{
			vector<ScintillaKeyMap> & cshortcuts = nppParam.getScintillaKeyList();
			cs_index=1;
			for (size_t i = 0; i < nbItems; ++i)
			{
				if (isFilterValid(cshortcuts[i]))
				{
					if (cshortcuts[i].isEnabled())
					{
						size_t sciCombos = cshortcuts[i].getSize();
						for (size_t sciIndex = 0; sciIndex < sciCombos; ++sciIndex)
						{
							if (findKeyConflicts(nullptr, cshortcuts[i].getKeyComboByIndex(sciIndex), i))
							{
								isMarker = _babygrid.setMarker(true);
								break;
							}
						}
					}

					_babygrid.setText(cs_index, 1, string2wstring(cshortcuts[i].getName(), CP_UTF8).c_str());
					if (cshortcuts[i].isEnabled()) //avoid empty strings for better performance
						_babygrid.setText(cs_index, 2, string2wstring(cshortcuts[i].toString(), CP_UTF8).c_str());
	
					if (isMarker)
						isMarker = _babygrid.setMarker(false);
					_shortcutIndex.push_back(i);
					cs_index++;
				}
			}
			_babygrid.setLineColNumber(cs_index - 1 , 2);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), true);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR), false);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
		}
		break;
	}
	
	if (nbItems > 0) //restore the last view
		_babygrid.setLastView(_lastHomeRow[_currentState], _lastCursorRow[_currentState]);
	else //clear the info area
		::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, 0);

	_babygrid.setInitialContent(false);
}

void ShortcutMapper::resizeDialogElements()
{
	constexpr auto getRcWidth = [](const RECT& rc) -> int {
		return rc.right - rc.left;
	};

	constexpr auto getRcHeight = [](const RECT& rc) -> int {
		return rc.bottom - rc.top;
	};

	auto setOrDeferWindowPos = [](HDWP hWinPosInfo, HWND hWnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags) -> HDWP {
		if (hWinPosInfo != nullptr)
		{
			return ::DeferWindowPos(hWinPosInfo, hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
		}
		::SetWindowPos(hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
		return nullptr;
	};

	constexpr UINT flags = SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;

	RECT rcClient{};
	Window::getClientRect(rcClient);

	RECT rcModBtn{};
	HWND hModBtn = ::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY);
	getMappedChildRect(hModBtn, rcModBtn);

	RECT rcClearBtn{};
	HWND hClearBtn = ::GetDlgItem(_hSelf, IDM_BABYGRID_CLEAR);
	getMappedChildRect(hClearBtn, rcClearBtn);

	RECT rcDelBtn{};
	HWND hDelBtn = ::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE);
	::GetClientRect(hDelBtn, &rcDelBtn);

	RECT rcOkBtn{};
	HWND hOkBtn = ::GetDlgItem(_hSelf, IDOK);
	::GetClientRect(hOkBtn, &rcOkBtn);

	RECT rcStatic{};
	HWND hStatic = ::GetDlgItem(_hSelf, IDC_BABYGRID_STATIC);
	getMappedChildRect(hStatic, rcStatic);

	RECT rcFilterEdit{};
	HWND hFilterEdit = ::GetDlgItem(_hSelf, IDC_BABYGRID_FILTER);
	::GetWindowRect(hFilterEdit, &rcFilterEdit);
	::MapWindowPoints(nullptr, _hSelf, reinterpret_cast<LPPOINT>(&rcFilterEdit), 2);

	RECT rcFilterClearBtn{};
	HWND hFilterClearBtn = ::GetDlgItem(_hSelf, IDC_BABYGRID_FILTER_CLEAR);
	::GetWindowRect(hFilterClearBtn, &rcFilterClearBtn);

	int clrBtnWidth = rcFilterClearBtn.right - rcFilterClearBtn.left;
	int clrBtnHeight = rcFilterClearBtn.bottom - rcFilterClearBtn.top;

	RECT rcInfo{};
	HWND hInfo = ::GetDlgItem(_hSelf, IDC_BABYGRID_INFO);
	::GetWindowRect(hInfo, &rcInfo);
	::MapWindowPoints(nullptr, _hSelf, reinterpret_cast<LPPOINT>(&rcInfo), 2);

	const int wClient = getRcWidth(rcClient);
	const int center = wClient / 2;

	const int wBtn = getRcWidth(rcOkBtn);
	const int gapBtn = rcClearBtn.left - rcModBtn.right;
	const int gapBtnHalf = gapBtn / 2;

	const int padding = _dpiManager.scale(6);
	::InflateRect(&rcClient, -padding, -(gapBtn + getRcHeight(rcOkBtn)));

	const int gapBtnEdit = rcClearBtn.top - rcFilterEdit.bottom;
	const int heightFilter = getRcHeight(rcFilterEdit);
	const int heightInfo = getRcHeight(rcInfo);

	constexpr int nCtrls = 8;
	auto hdwp = ::BeginDeferWindowPos(nCtrls);

	hdwp = setOrDeferWindowPos(hdwp, hModBtn, nullptr, center - gapBtnHalf - wBtn * 2 - gapBtn, rcClient.bottom, 0, 0, SWP_NOSIZE | flags);
	hdwp = setOrDeferWindowPos(hdwp, hClearBtn, nullptr, center - gapBtnHalf - wBtn, rcClient.bottom, 0, 0, SWP_NOSIZE | flags);
	hdwp = setOrDeferWindowPos(hdwp, hDelBtn, nullptr, center + gapBtnHalf, rcClient.bottom, 0, 0, SWP_NOSIZE | flags);
	hdwp = setOrDeferWindowPos(hdwp, hOkBtn, nullptr, center + gapBtnHalf + wBtn + gapBtn, rcClient.bottom, 0, 0, SWP_NOSIZE | flags);

	rcClient.bottom -= (gapBtnEdit + heightFilter);
	hdwp = setOrDeferWindowPos(hdwp, hStatic, nullptr, rcClient.left, rcClient.bottom + gapBtnEdit / 2, 0, 0, SWP_NOSIZE | flags);
	hdwp = setOrDeferWindowPos(hdwp, hFilterEdit, nullptr, rcFilterEdit.left, rcClient.bottom, rcClient.right - rcFilterEdit.left - clrBtnWidth, heightFilter, flags);
	hdwp = setOrDeferWindowPos(hdwp, hFilterClearBtn, nullptr, rcClient.right - clrBtnWidth + 2, rcClient.bottom, clrBtnWidth, clrBtnHeight, SWP_NOSIZE | flags);
	hdwp = setOrDeferWindowPos(hdwp, hInfo, nullptr, rcClient.left, rcClient.bottom - gapBtnEdit - heightInfo, getRcWidth(rcClient), heightInfo, flags);

	if (hdwp)
		::EndDeferWindowPos(hdwp);

	getClientRect(rcClient);
	_babygrid.reSizeToWH(rcClient);
}

intptr_t CALLBACK ShortcutMapper::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			setDpi();
			initBabyGrid();
			initTabs();
			fillOutBabyGrid();
			_babygrid.display();
			goToCenter();

			resizeDialogElements();

			NppDarkMode::autoSubclassAndThemeChildControls(_hSelf);

			const LONG padding = _dpiManager.getSystemMetricsForDpi(SM_CXPADDEDBORDER);
			_szBorder.cx = (_dpiManager.getSystemMetricsForDpi(SM_CXFRAME) + padding) * 2;
			_szBorder.cy = (_dpiManager.getSystemMetricsForDpi(SM_CYFRAME) + padding) * 2 + _dpiManager.getSystemMetricsForDpi(SM_CYCAPTION);

			RECT rect{};
			Window::getClientRect(rect);
			_szMinDialog.cx = rect.right - rect.left;
			_szMinDialog.cy = rect.bottom - rect.top;

			_dialogInitDone = true;

			return TRUE;
		}

		case WM_CTLCOLOREDIT:
		{
			return NppDarkMode::onCtlColorCtrl(reinterpret_cast<HDC>(wParam));
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			return NppDarkMode::onCtlColorDlg(reinterpret_cast<HDC>(wParam));
		}

		case WM_PRINTCLIENT:
		{
			if (NppDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case NPPM_INTERNAL_REFRESHDARKMODE:
		{
			NppDarkMode::autoThemeChildControls(_hSelf);
			return TRUE;
		}

		case WM_GETMINMAXINFO:
		{
			auto mmi = reinterpret_cast<MINMAXINFO*>(lParam);
			if (_dialogInitDone)
			{
				mmi->ptMinTrackSize.x = _szMinDialog.cx + _szBorder.cx;
				mmi->ptMinTrackSize.y = _szMinDialog.cy + _szBorder.cy;
			}
			return TRUE;
		}

		case WM_DESTROY:
		{
			for (const HFONT & hFont : _hGridFonts)
				::DeleteObject(hFont);

			_hGridFonts.clear();
			_hGridFonts.shrink_to_fit();
			break;
		}

		case WM_SIZE:
		{
			resizeDialogElements();
			return TRUE;
		}

		case WM_NOTIFY:
		{
			NMHDR nmh = *reinterpret_cast<NMHDR*>(lParam);
			if (nmh.hwndFrom == _hTabCtrl)
			{
				if (nmh.code == TCN_SELCHANGE)
				{
					//save the current view
					_lastHomeRow[_currentState] = _babygrid.getHomeRow();
					_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
					int index = TabCtrl_GetCurSel(_hTabCtrl);

					switch (index)
					{
						case 0:
							_currentState = STATE_MENU;
							break;

						case 1:
							_currentState = STATE_MACRO;
							break;

						case 2:
							_currentState = STATE_USER;
							break;

						case 3:
							_currentState = STATE_PLUGIN;
							break;

						case 4:
							_currentState = STATE_SCINTILLA;
							break;
					}
					fillOutBabyGrid();
				}
			}
			return TRUE;
		}

		case NPPM_INTERNAL_FINDKEYCONFLICTS:
		{
			if (!wParam || !lParam)
				break;

			wstring conflictInfo;

			// In case of using filter will make the filtered items change index, so here we get its real index
			size_t realIndexOfSelectedItem = _shortcutIndex[_babygrid.getSelectedRow() - 1];

			const bool isConflict = findKeyConflicts(&conflictInfo, *reinterpret_cast<KeyCombo*>(wParam), realIndexOfSelectedItem);
			*reinterpret_cast<bool*>(lParam) = isConflict;

			if (isConflict)
				::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(conflictInfo.c_str()));
			else
				::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_conflictInfoEditing.c_str()));

			return TRUE;
		}

		case WM_DPICHANGED:
		{
			const UINT prevDpi = _dpiManager.getDpi();
			_dpiManager.setDpiWP(wParam);

			for (auto& hFont : _hGridFonts)
			{
				if (hFont != nullptr)
				{
					::DeleteObject(hFont);
					hFont = nullptr;
				}
			}

			LOGFONT lf{ _dpiManager.getDefaultGUIFontForDpi() };
			lf.lfHeight = _dpiManager.scaleFont(10);
			_hGridFonts.at(GFONT_ROWS) = ::CreateFontIndirect(&lf);
			lf.lfHeight = _dpiManager.scaleFont(12);
			lf.lfWeight = FW_BOLD;
			_hGridFonts.at(GFONT_HEADER) = ::CreateFontIndirect(&lf);

			_babygrid.setHeaderFont(_hGridFonts.at(GFONT_HEADER));
			_babygrid.setRowFont(_hGridFonts.at(GFONT_ROWS));

			_babygrid.setColWidth(0, _dpiManager.scale(30));
			_babygrid.setHeaderHeight(_dpiManager.scale(21));
			_babygrid.setRowHeight(_dpiManager.scale(21));

			const LONG padding = _dpiManager.getSystemMetricsForDpi(SM_CXPADDEDBORDER);
			_szBorder.cx = (_dpiManager.getSystemMetricsForDpi(SM_CXFRAME) + padding) * 2;
			_szBorder.cy = (_dpiManager.getSystemMetricsForDpi(SM_CYFRAME) + padding) * 2 + _dpiManager.getSystemMetricsForDpi(SM_CYCAPTION);

			const UINT dpi = _dpiManager.getDpi();
			_szMinDialog.cx = _dpiManager.scale(_szMinDialog.cx, dpi, prevDpi);
			_szMinDialog.cy = _dpiManager.scale(_szMinDialog.cy, dpi, prevDpi);

			setPositionDpi(lParam, SWP_NOZORDER | SWP_NOACTIVATE);

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL :
				{
					::EndDialog(_hSelf, -1);
					return TRUE;
				}
				case IDOK :
				{
					::EndDialog(_hSelf, 0);
					return TRUE;
				}

				case IDM_BABYGRID_CLEAR :
				{
					if (_babygrid.getNumberRows() < 1)
						return TRUE;

					NppParameters& nppParam = NppParameters::getInstance();
					int row = _babygrid.getSelectedRow();
					size_t shortcutIndex = _shortcutIndex[row-1];
					bool isModified = false;

					switch(_currentState)
					{
						case STATE_MENU:
						{
							//Get CommandShortcut corresponding to row
							vector<CommandShortcut> & shortcuts = nppParam.getUserShortcuts();
							CommandShortcut csc = shortcuts[shortcutIndex];
							csc.clear();
							shortcuts[shortcutIndex] = csc;
							//shortcut was altered
							nppParam.addUserModifiedIndex(shortcutIndex);

							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam.getAccelerator()->updateShortcuts();
							nppParam.setShortcutDirty();
						}
						break;

						case STATE_MACRO: 
						{
							//Get MacroShortcut corresponding to row
							vector<MacroShortcut> & shortcuts = nppParam.getMacroList();
							MacroShortcut msc = shortcuts[shortcutIndex];
							msc.clear();
							shortcuts[shortcutIndex] = msc;
							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam.getAccelerator()->updateShortcuts();
							nppParam.setShortcutDirty();
						}
						break;

						case STATE_USER: 
						{
							//Get UserCommand corresponding to row
							vector<UserCommand> & shortcuts = nppParam.getUserCommandList();
							UserCommand ucmd = shortcuts[shortcutIndex];
							ucmd.clear();

							//shortcut was altered
							shortcuts[shortcutIndex] = ucmd;

							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam.getAccelerator()->updateShortcuts();
							nppParam.setShortcutDirty();
						}
						break;

						case STATE_PLUGIN: 
						{
							//Get PluginCmdShortcut corresponding to row
							vector<PluginCmdShortcut> & shortcuts = nppParam.getPluginCommandList();
							PluginCmdShortcut pcsc = shortcuts[shortcutIndex];
							pcsc.clear();
							//shortcut was altered
							nppParam.addPluginModifiedIndex(shortcutIndex);
							shortcuts[shortcutIndex] = pcsc;

							//save the current view
							_lastHomeRow[_currentState] = _babygrid.getHomeRow();
							_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
							fillOutBabyGrid();

							isModified = true;

							//Notify current Accelerator class to update everything
							nppParam.getAccelerator()->updateShortcuts();
							unsigned long cmdID = pcsc.getID();
							ShortcutKey shortcut;
							shortcut._isAlt = FALSE;
							shortcut._isCtrl = FALSE;
							shortcut._isShift = FALSE;
							shortcut._key = '\0';

							::SendMessage(_hParent, NPPM_INTERNAL_PLUGINSHORTCUTMODIFIED, cmdID, reinterpret_cast<LPARAM>(&shortcut));
							nppParam.setShortcutDirty();
						}
						break;

						case STATE_SCINTILLA: 
						{
							// Do nothing
						}
						break;

					}

					if (!isModified)
						::SendMessage(_hSelf, WM_COMMAND, MAKEWPARAM(IDD_BABYGRID_ID1, BGN_ROWCHANGED), row);
					
					return TRUE;
				}

				case IDM_BABYGRID_MODIFY :
				{
					if (_babygrid.getNumberRows() < 1)
						return TRUE;

					NppParameters& nppParam = NppParameters::getInstance();
					int row = _babygrid.getSelectedRow();
					size_t shortcutIndex = _shortcutIndex[row-1];
					bool isModified = false;

					switch(_currentState)
					{
						case STATE_MENU:
						{
							//Get CommandShortcut corresponding to row
							vector<CommandShortcut> & shortcuts = nppParam.getUserShortcuts();
							CommandShortcut csc = shortcuts[shortcutIndex], prevcsc = shortcuts[shortcutIndex];
							csc.init(_hInst, _hSelf);
							if (csc.doDialog() != -1 && prevcsc != csc)
							{
								//shortcut was altered
								nppParam.addUserModifiedIndex(shortcutIndex);
								shortcuts[shortcutIndex] = csc;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam.getAccelerator()->updateShortcuts();
								nppParam.setShortcutDirty();
							}
						}
						break;

						case STATE_MACRO: 
						{
							//Get MacroShortcut corresponding to row
							vector<MacroShortcut> & shortcuts = nppParam.getMacroList();
							MacroShortcut msc = shortcuts[shortcutIndex], prevmsc = shortcuts[shortcutIndex];
							msc.init(_hInst, _hSelf);
							if (msc.doDialog() != -1 && prevmsc != msc)
							{
								//shortcut was altered
								shortcuts[shortcutIndex] = msc;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam.getAccelerator()->updateShortcuts();
								nppParam.setShortcutDirty();
							}
						}
						break; 

						case STATE_USER: 
						{
							//Get UserCommand corresponding to row
							vector<UserCommand> & shortcuts = nppParam.getUserCommandList();
							UserCommand ucmd = shortcuts[shortcutIndex];
							ucmd.init(_hInst, _hSelf);
							UserCommand prevucmd = ucmd;
							if (ucmd.doDialog() != -1 && prevucmd != ucmd)
							{
								//shortcut was altered
								shortcuts[shortcutIndex] = ucmd;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam.getAccelerator()->updateShortcuts();
								nppParam.setShortcutDirty();
							}
						}
						break; 

						case STATE_PLUGIN:
						{
							//Get PluginCmdShortcut corresponding to row
							vector<PluginCmdShortcut> & shortcuts = nppParam.getPluginCommandList();
							PluginCmdShortcut pcsc = shortcuts[shortcutIndex];
							pcsc.init(_hInst, _hSelf);
							PluginCmdShortcut prevpcsc = pcsc;
							if (pcsc.doDialog() != -1 && prevpcsc != pcsc)
							{
								//shortcut was altered
								nppParam.addPluginModifiedIndex(shortcutIndex);
								shortcuts[shortcutIndex] = pcsc;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();

								isModified = true;

								//Notify current Accelerator class to update everything
								nppParam.getAccelerator()->updateShortcuts();
								unsigned long cmdID = pcsc.getID();
								ShortcutKey shortcut;
								shortcut._isAlt = pcsc.getKeyCombo()._isAlt;
								shortcut._isCtrl = pcsc.getKeyCombo()._isCtrl;
								shortcut._isShift = pcsc.getKeyCombo()._isShift;
								shortcut._key = pcsc.getKeyCombo()._key;

								::SendMessage(_hParent, NPPM_INTERNAL_PLUGINSHORTCUTMODIFIED, cmdID, reinterpret_cast<LPARAM>(&shortcut));
								nppParam.setShortcutDirty();
							}
						}
						break;

						case STATE_SCINTILLA:
						{
							//Get ScintillaKeyMap corresponding to row
							vector<ScintillaKeyMap> & shortcuts = nppParam.getScintillaKeyList();
							ScintillaKeyMap skm = shortcuts[shortcutIndex], prevskm = shortcuts[shortcutIndex];
							skm.init(_hInst, _hSelf);
							if (skm.doDialog() != -1 && prevskm != skm)
							{
								//shortcut was altered
								nppParam.addScintillaModifiedIndex((int)shortcutIndex);
								shortcuts[shortcutIndex] = skm;

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();
								fillOutBabyGrid();
								_babygrid.updateView();

								isModified = true;

								//Notify current Accelerator class to update key
								nppParam.getScintillaAccelerator()->updateKeys();
								nppParam.setShortcutDirty();
							}
						}
						break;  
					}

					if (!isModified)
						::SendMessage(_hSelf, WM_COMMAND, MAKEWPARAM(IDD_BABYGRID_ID1, BGN_ROWCHANGED), row);

					return TRUE;
				}

				case IDM_BABYGRID_DELETE :
				{
					if (_babygrid.getNumberRows() < 1)
						return TRUE;

					NppParameters& nppParam = NppParameters::getInstance();
					int res = nppParam.getNativeLangSpeaker()->messageBox("SCMapperDoDeleteOrNot",
						_hSelf,
						L"Are you sure you want to delete this shortcut?",
						L"Are you sure?",
						MB_OKCANCEL);

					if (res == IDOK)
					{
						const int row = _babygrid.getSelectedRow();
						size_t shortcutIndex = _shortcutIndex[row-1];
						
						switch(_currentState) 
						{
							case STATE_MENU:
							case STATE_PLUGIN:
							case STATE_SCINTILLA: 
							{
								return FALSE;
							}

							case STATE_MACRO: 
							{
								vector<MacroShortcut> & theMacros = nppParam.getMacroList();
								theMacros.erase(theMacros.begin() + shortcutIndex);

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();

								const size_t numberRows = _babygrid.getNumberRows();
								if (_lastHomeRow[_currentState] == numberRows)
									--_lastHomeRow[_currentState];
								if (_lastCursorRow[_currentState] == numberRows)
									--_lastCursorRow[_currentState];

								fillOutBabyGrid();

								// clear all menu
								DynamicMenu& macroMenu = nppParam.getMacroMenuItems();
								macroMenu.clearMenu();

								// Erase the menu item
								macroMenu.erase(shortcutIndex);

								size_t nbElem = theMacros.size();
								for (size_t i = shortcutIndex; i < nbElem; ++i)	//lower the IDs of the remaining items so there are no gaps
								{
									MacroShortcut ms = theMacros[i];
									ms.setID(ms.getID() - 1);	//shift all IDs
									theMacros[i] = ms;

									// Adjust menu items
									MenuItemUnit& miu = macroMenu.getItemFromIndex(i);
									miu._cmdID -= 1;	//shift all IDs
								}
								// create from scratch according the new menu items structure
								macroMenu.createMenu();

								HMENU m = reinterpret_cast<HMENU>(::SendMessage(_hParent, NPPM_INTERNAL_GETMENU, 0, 0));
								HMENU hMenu = ::GetSubMenu(m, MENUINDEX_MACRO);
								if (!hMenu) return FALSE;

								int32_t posBase = macroMenu.getPosBase();
								if (nbElem == 0)
								{
									::RemoveMenu(hMenu, IDM_SETTING_SHORTCUT_MAPPER_MACRO, MF_BYCOMMAND);

									//remove separator
									::RemoveMenu(hMenu, posBase - 1, MF_BYPOSITION);
									::RemoveMenu(hMenu, posBase - 1, MF_BYPOSITION);
								}
							}
							break; 

							case STATE_USER: 
							{
								vector<UserCommand> & theUserCmds = nppParam.getUserCommandList();
								theUserCmds.erase(theUserCmds.begin() + shortcutIndex);

								//save the current view
								_lastHomeRow[_currentState] = _babygrid.getHomeRow();
								_lastCursorRow[_currentState] = _babygrid.getSelectedRow();

								const size_t numberRows = _babygrid.getNumberRows();
								if (_lastHomeRow[_currentState] == numberRows)
									--_lastHomeRow[_currentState];
								if (_lastCursorRow[_currentState] == numberRows)
									--_lastCursorRow[_currentState];

								fillOutBabyGrid();

								// clear all menu
								DynamicMenu& runMenu = nppParam.getRunMenuItems();
								runMenu.clearMenu();

								// Erase the menu item
								runMenu.erase(shortcutIndex);

								// preparing to remove from menu
			
								size_t nbElem = theUserCmds.size();
								for (size_t i = shortcutIndex; i < nbElem; ++i)	//lower the IDs of the remaining items so there are no gaps
								{
									UserCommand uc = theUserCmds[i];
									uc.setID(uc.getID() - 1);	//shift all IDs
									theUserCmds[i] = uc;

									// Adjust menu items
									MenuItemUnit& miu = runMenu.getItemFromIndex(i);
									miu._cmdID -= 1;	//shift all IDs
								}
								// create from scratch according the new menu items structure
								runMenu.createMenu();

								HMENU m = reinterpret_cast<HMENU>(::SendMessage(_hParent, NPPM_INTERNAL_GETMENU, 0, 0));
								HMENU hMenu = ::GetSubMenu(m, MENUINDEX_RUN);
								if (!hMenu) return FALSE;

								int32_t posBase = runMenu.getPosBase();
								if (nbElem == 0)
								{
									::RemoveMenu(hMenu, IDM_SETTING_SHORTCUT_MAPPER_RUN, MF_BYCOMMAND);

									//remove separator
									::RemoveMenu(hMenu, posBase - 1, MF_BYPOSITION);
									::RemoveMenu(hMenu, posBase - 1, MF_BYPOSITION);
								}
							}
							break;
						}

						// updateShortcuts() will update all menu item - the menu items will be shifted
						nppParam.getAccelerator()->updateShortcuts();
						nppParam.setShortcutDirty();
					}
					return TRUE;
				}

				case IDD_BABYGRID_ID1: 
				{
					switch (HIWORD(wParam))
					{
						case BGN_CELLDBCLICKED: //a cell was clicked in the properties grid
						{
							return ::SendMessage(_hSelf, WM_COMMAND, IDM_BABYGRID_MODIFY, LOWORD(lParam));
						}

						case BGN_CELLRCLICKED: //a cell was clicked in the properties grid
						{
							POINT p{};
							::GetCursorPos(&p);
							if (!_rightClickMenu.isCreated())
							{
								vector<MenuItemUnit> itemUnitArray;
								NativeLangSpeaker* nativeLangSpeaker = NppParameters::getInstance().getNativeLangSpeaker();
								wstring modifyStr = nativeLangSpeaker->getShortcutMapperLangStr("ModifyContextMenu", L"Modify");
								wstring deleteStr = nativeLangSpeaker->getShortcutMapperLangStr("DeleteContextMenu", L"Delete");
								wstring clearStr = nativeLangSpeaker->getShortcutMapperLangStr("ClearContextMenu", L"Clear");
								itemUnitArray.push_back(MenuItemUnit(IDM_BABYGRID_MODIFY, modifyStr.c_str()));
								itemUnitArray.push_back(MenuItemUnit(IDM_BABYGRID_DELETE, deleteStr.c_str()));
								itemUnitArray.push_back(MenuItemUnit(IDM_BABYGRID_CLEAR, clearStr.c_str()));
								_rightClickMenu.create(_hSelf, itemUnitArray);
							}

							if (_babygrid.getNumberRows() < 1)
							{
								_rightClickMenu.enableItem(IDM_BABYGRID_MODIFY, false);
								_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, false);
								_rightClickMenu.enableItem(IDM_BABYGRID_CLEAR, false);
							}
							else
							{
								_rightClickMenu.enableItem(IDM_BABYGRID_MODIFY, true);
								_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, true);
								if (_currentState == STATE_SCINTILLA)
									_rightClickMenu.enableItem(IDM_BABYGRID_CLEAR, false);
								else
									_rightClickMenu.enableItem(IDM_BABYGRID_CLEAR, true);

								switch(_currentState)
								{
									case STATE_MACRO:
									case STATE_USER:
									{
										_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, true);
									}
									break;

									case STATE_MENU:
									case STATE_PLUGIN:
									case STATE_SCINTILLA:
									{
										_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, false);
									}
									break;
								}
							}

							_rightClickMenu.display(p);
							return TRUE;
						}

						case BGN_DELETECELL: //VK_DELETE
						{
							switch(_currentState) 
							{
								case STATE_MACRO:
								case STATE_USER:
									return ::SendMessage(_hSelf, WM_COMMAND, IDM_BABYGRID_DELETE, 0);

								case STATE_MENU:
								case STATE_PLUGIN:
								case STATE_SCINTILLA:
								default:
									break;
							}
							return TRUE;
						}

						case BGN_ROWCHANGED:
						{
							if (_babygrid.getNumberRows() < 1)
								return TRUE;

							NppParameters& nppParam = NppParameters::getInstance();
							const size_t currentIndex = LOWORD(lParam) - 1;

							// In case of using filter will make the filtered items change index, so here we get its real index
							size_t realIndexOfSelectedItem = _shortcutIndex[currentIndex];

							wstring conflictInfo;

							switch (_currentState)
							{
								case STATE_MENU:
								{
									vector<CommandShortcut> & vShortcuts = nppParam.getUserShortcuts();
									findKeyConflicts(&conflictInfo, vShortcuts[realIndexOfSelectedItem].getKeyCombo(), realIndexOfSelectedItem);
								}
								break;

								case STATE_MACRO:
								{
									vector<MacroShortcut> & vShortcuts = nppParam.getMacroList();
									findKeyConflicts(&conflictInfo, vShortcuts[realIndexOfSelectedItem].getKeyCombo(), realIndexOfSelectedItem);
								}
								break;

								case STATE_USER:
								{
									vector<UserCommand> & vShortcuts = nppParam.getUserCommandList();
									findKeyConflicts(&conflictInfo, vShortcuts[realIndexOfSelectedItem].getKeyCombo(), realIndexOfSelectedItem);
								}
								break;

								case STATE_PLUGIN:
								{
									vector<PluginCmdShortcut> & vShortcuts = nppParam.getPluginCommandList();
									findKeyConflicts(&conflictInfo, vShortcuts[realIndexOfSelectedItem].getKeyCombo(), realIndexOfSelectedItem);
								}
								break;

								case STATE_SCINTILLA:
								{
									vector<ScintillaKeyMap> & vShortcuts = nppParam.getScintillaKeyList();
									size_t sciCombos = vShortcuts[realIndexOfSelectedItem].getSize();
									for (size_t sciIndex = 0; sciIndex < sciCombos; ++sciIndex)
										findKeyConflicts(&conflictInfo, vShortcuts[realIndexOfSelectedItem].getKeyComboByIndex(sciIndex), realIndexOfSelectedItem);
								}
								break;
							}

							if (conflictInfo.empty())
								::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(_conflictInfoOk.c_str()));
							else
								::SendDlgItemMessage(_hSelf, IDC_BABYGRID_INFO, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(conflictInfo.c_str()));

							return TRUE;
						}
					}
					break;
				}
				case IDC_BABYGRID_FILTER:
				{
					if (HIWORD(wParam) == EN_CHANGE)
					{
						fillOutBabyGrid();
					}
					return TRUE;
				}
				case IDC_BABYGRID_FILTER_CLEAR:
				{
					HWND hFilterEdit = ::GetDlgItem(_hSelf, IDC_BABYGRID_FILTER);
					::SetWindowText(hFilterEdit, L"");
					::SetFocus(hFilterEdit);
					return TRUE;
				}
				default:
				{
					break;
				}
			}
			break;
		}

		default:
			return FALSE;

	}

	return FALSE;
}

bool ShortcutMapper::findKeyConflicts(__inout_opt wstring * const keyConflictLocation,
										const KeyCombo & itemKeyComboToTest, const size_t & itemIndexToTest) const
{
	if (itemKeyComboToTest._key == 0) //no key assignment
		return false;

	bool retIsConflict = false; //returns true when a conflict is found
	NppParameters& nppParam = NppParameters::getInstance();

	for (size_t gridState = STATE_MENU; gridState <= STATE_SCINTILLA; ++gridState)
	{
		switch (gridState)
		{
			case STATE_MENU:
			{
				vector<CommandShortcut> & vShortcuts = nppParam.getUserShortcuts();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (!vShortcuts[itemIndex].isEnabled()) //no key assignment
						continue;

					if ((itemIndex == itemIndexToTest) && (gridState == static_cast<size_t>(_currentState))) //don't catch oneself
						continue;

					if (isConflict(vShortcuts[itemIndex].getKeyCombo(), itemKeyComboToTest))
					{
						retIsConflict = true;
						if (keyConflictLocation == nullptr)
							return retIsConflict;
						else
						{
							if (!keyConflictLocation->empty())
								*keyConflictLocation += L"\r\n";
							*keyConflictLocation += _tabNames[gridState];
							*keyConflictLocation += L"  |  ";
							*keyConflictLocation += std::to_wstring(itemIndex + 1);
							*keyConflictLocation += L"   ";
							*keyConflictLocation += string2wstring(vShortcuts[itemIndex].getName(), CP_UTF8);
							*keyConflictLocation += L"  ( ";
							*keyConflictLocation += string2wstring(vShortcuts[itemIndex].toString(), CP_UTF8);
							*keyConflictLocation += L" )";
						}
					}
				}
				break;
			} //case STATE_MENU
			case STATE_MACRO:
			{
				vector<MacroShortcut> & vShortcuts = nppParam.getMacroList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (!vShortcuts[itemIndex].isEnabled()) //no key assignment
						continue;

					if ((itemIndex == itemIndexToTest) && (gridState == static_cast<size_t>(_currentState))) //don't catch oneself
						continue;

					if (isConflict(vShortcuts[itemIndex].getKeyCombo(), itemKeyComboToTest))
					{
						retIsConflict = true;
						if (keyConflictLocation == nullptr)
							return retIsConflict;
						else
						{
							if (!keyConflictLocation->empty())
								*keyConflictLocation += L"\r\n";
							*keyConflictLocation += _tabNames[gridState];
							*keyConflictLocation += L"  |  ";
							*keyConflictLocation += std::to_wstring(itemIndex + 1);
							*keyConflictLocation += L"   ";
							*keyConflictLocation += string2wstring(vShortcuts[itemIndex].getName(), CP_UTF8);
							*keyConflictLocation += L"  ( ";
							*keyConflictLocation += string2wstring(vShortcuts[itemIndex].toString(), CP_UTF8);
							*keyConflictLocation += L" )";
						}
					}
				}
				break;
			} //case STATE_MACRO
			case STATE_USER:
			{
				vector<UserCommand> & vShortcuts = nppParam.getUserCommandList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (!vShortcuts[itemIndex].isEnabled()) //no key assignment
						continue;

					if ((itemIndex == itemIndexToTest) && (gridState == static_cast<size_t>(_currentState))) //don't catch oneself
						continue;

					if (isConflict(vShortcuts[itemIndex].getKeyCombo(), itemKeyComboToTest))
					{
						retIsConflict = true;
						if (keyConflictLocation == nullptr)
							return retIsConflict;
						else
						{
							if (!keyConflictLocation->empty())
								*keyConflictLocation += L"\r\n";
							*keyConflictLocation += _tabNames[gridState];
							*keyConflictLocation += L"  |  ";
							*keyConflictLocation += std::to_wstring(itemIndex + 1);
							*keyConflictLocation += L"   ";
							*keyConflictLocation += string2wstring(vShortcuts[itemIndex].getName(), CP_UTF8);
							*keyConflictLocation += L"  ( ";
							*keyConflictLocation += string2wstring(vShortcuts[itemIndex].toString(), CP_UTF8);
							*keyConflictLocation += L" )";
						}
					}
				}
				break;
			} //case STATE_USER
			case STATE_PLUGIN:
			{
				vector<PluginCmdShortcut> & vShortcuts = nppParam.getPluginCommandList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (!vShortcuts[itemIndex].isEnabled()) //no key assignment
						continue;

					if ((itemIndex == itemIndexToTest) && (gridState == static_cast<size_t>(_currentState))) //don't catch oneself
						continue;

					if (isConflict(vShortcuts[itemIndex].getKeyCombo(), itemKeyComboToTest))
					{
						retIsConflict = true;
						if (keyConflictLocation == nullptr)
							return retIsConflict;
						else
						{
							if (!keyConflictLocation->empty())
								*keyConflictLocation += L"\r\n";
							*keyConflictLocation += _tabNames[gridState];
							*keyConflictLocation += L"  |  ";
							*keyConflictLocation += std::to_wstring(itemIndex + 1);
							*keyConflictLocation += L"   ";
							*keyConflictLocation += string2wstring(vShortcuts[itemIndex].getName(), CP_UTF8);
							*keyConflictLocation += L"  ( ";
							*keyConflictLocation += string2wstring(vShortcuts[itemIndex].toString(), CP_UTF8);
							*keyConflictLocation += L" )";
						}
					}
				}
				break;
			} //case STATE_PLUGIN
			case STATE_SCINTILLA:
			{
				vector<ScintillaKeyMap> & vShortcuts = nppParam.getScintillaKeyList();
				size_t nbItems = vShortcuts.size();
				for (size_t itemIndex = 0; itemIndex < nbItems; ++itemIndex)
				{
					if (!vShortcuts[itemIndex].isEnabled()) //no key assignment
						continue;

					if ((itemIndex == itemIndexToTest) && (gridState == static_cast<size_t>(_currentState))) //don't catch oneself
						continue;

					size_t sciCombos = vShortcuts[itemIndex].getSize();
					for (size_t sciIndex = 0; sciIndex < sciCombos; ++sciIndex)
					{
						if (isConflict(vShortcuts[itemIndex].getKeyComboByIndex(sciIndex), itemKeyComboToTest))
						{
							retIsConflict = true;
							if (keyConflictLocation == nullptr)
								return retIsConflict;
							else
							{
								if (!keyConflictLocation->empty())
									*keyConflictLocation += L"\r\n";
								*keyConflictLocation += _tabNames[gridState];
								*keyConflictLocation += L"  |  ";
								*keyConflictLocation += std::to_wstring(itemIndex + 1);
								if (sciIndex > 0)
									*keyConflictLocation += L"*   ";
								else
									*keyConflictLocation += L"   ";
								*keyConflictLocation += string2wstring(vShortcuts[itemIndex].getName(), CP_UTF8);
								*keyConflictLocation += L"  ( ";
								*keyConflictLocation += string2wstring(vShortcuts[itemIndex].toString(sciIndex), CP_UTF8);
								*keyConflictLocation += L" )";
							}
						}
					}
				}
				break;
			}
		}
	}
	return retIsConflict;
}
