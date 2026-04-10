/*
 * This file is part of ComparePlus plugin for Notepad++
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

#include "Compare.h"
#include "UserSettings.h"
#include "Strings.h"

#include <shlwapi.h>
#include <cstdlib>


const wchar_t UserSettings::cRegexEntriesDelimiter[]		= L";\\;";

const wchar_t UserSettings::mainSection[]					= L"main_settings";

const wchar_t UserSettings::firstIsNewSetting[]				= L"set_first_as_new";
const wchar_t UserSettings::newFileViewSetting[]			= L"new_in_sub_view";
const wchar_t UserSettings::compareToPrevSetting[]			= L"default_compare_to_prev";

const wchar_t UserSettings::encodingsCheckSetting[]			= L"check_encodings";
const wchar_t UserSettings::sizesCheckSetting[]				= L"check_sizes";
const wchar_t UserSettings::manualSyncCheckSetting[]		= L"check_manual_sync";
const wchar_t UserSettings::promptCloseOnMatchSetting[]		= L"prompt_to_close_on_match";
const wchar_t UserSettings::hideMarginSetting[]				= L"hide_margin";
const wchar_t UserSettings::markIgnoredLinesSetting[]		= L"never_colorize_ignored_lines";
const wchar_t UserSettings::followingCaretSetting[]			= L"following_caret";
const wchar_t UserSettings::wrapAroundSetting[]				= L"wrap_around";
const wchar_t UserSettings::gotoFirstDiffSetting[]			= L"go_to_first_on_recompare";

const wchar_t UserSettings::detectMovesSetting[]			= L"detect_moves";
const wchar_t UserSettings::detectSubBlockDiffsSetting[]	= L"detect_sub_block_diffs";
const wchar_t UserSettings::detectSubLineMovesSetting[]		= L"detect_sub_line_moves";
const wchar_t UserSettings::detectCharDiffsSetting[]		= L"detect_character_diffs";
const wchar_t UserSettings::ignoreEmptyLinesSetting[]		= L"ignore_empty_lines";
const wchar_t UserSettings::ignoreFoldedLinesSetting[]		= L"ignore_folded_lines";
const wchar_t UserSettings::ignoreHiddenLinesSetting[]		= L"ignore_hidden_lines";
const wchar_t UserSettings::ignoreChangedSpacesSetting[]	= L"ignore_changed_spaces";
const wchar_t UserSettings::ignoreAllSpacesSetting[]		= L"ignore_all_spaces";
const wchar_t UserSettings::ignoreEOLSetting[]				= L"ignore_eol";
const wchar_t UserSettings::ignoreCaseSetting[]				= L"ignore_case";
const wchar_t UserSettings::ignoreRegexSetting[]			= L"ignore_regex";
const wchar_t UserSettings::invertRegexSetting[]			= L"invert_regex";
const wchar_t UserSettings::inclRegexNomatchLinesSetting[]	= L"incl_regex_nomatch_lines";
const wchar_t UserSettings::highlightRegexIgnoresSetting[]	= L"highlight_regex_ignores";
const wchar_t UserSettings::ignoreRegexStrSetting[]			= L"ignore_regex_strings";
const wchar_t UserSettings::hideMatchesSetting[]			= L"hide_matches";
const wchar_t UserSettings::hideNewLinesSetting[]			= L"hide_added_removed_lines";
const wchar_t UserSettings::hideChangedLinesSetting[]		= L"hide_changed_lines";
const wchar_t UserSettings::hideMovedLinesSetting[]			= L"hide_moved_lines";
const wchar_t UserSettings::showOnlySelSetting[]			= L"show_only_selections";
const wchar_t UserSettings::navBarSetting[]					= L"navigation_bar";

const wchar_t UserSettings::bookmarksAsSyncSetting[]		= L"bookmarks_as_sync";
const wchar_t UserSettings::reCompareOnChangeSetting[]		= L"recompare_on_change";

const wchar_t UserSettings::statusInfoSetting[]				= L"status_info";

const wchar_t UserSettings::colorsSection[]					= L"color_settings";

const wchar_t UserSettings::addedColorSetting[]				= L"added";
const wchar_t UserSettings::removedColorSetting[]			= L"removed";
const wchar_t UserSettings::movedColorSetting[]				= L"moved";
const wchar_t UserSettings::changedColorSetting[]			= L"changed";
const wchar_t UserSettings::addedPartColorSetting[]			= L"added_part";
const wchar_t UserSettings::removedPartColorSetting[]		= L"removed_part";
const wchar_t UserSettings::movedPartColorSetting[]			= L"moved_part";
const wchar_t UserSettings::partTranspSetting[]				= L"part_transparency";
const wchar_t UserSettings::caretLineTranspSetting[]		= L"caret_line_transparency";

const wchar_t UserSettings::addedColorDarkSetting[]			= L"added_dark";
const wchar_t UserSettings::removedColorDarkSetting[]		= L"removed_dark";
const wchar_t UserSettings::movedColorDarkSetting[]			= L"moved_dark";
const wchar_t UserSettings::changedColorDarkSetting[]		= L"changed_dark";
const wchar_t UserSettings::addedPartColorDarkSetting[]		= L"added_part_dark";
const wchar_t UserSettings::removedPartColorDarkSetting[]	= L"removed_part_dark";
const wchar_t UserSettings::movedPartColorDarkSetting[]		= L"moved_part_dark";
const wchar_t UserSettings::highlightTranspDarkSetting[]	= L"part_transparency_dark";
const wchar_t UserSettings::caretLineTranspDarkSetting[]	= L"caret_line_transparency_dark";

const wchar_t UserSettings::changedResemblSetting[]			= L"changed_resemblance";

const wchar_t UserSettings::toolbarSection[]				= L"toolbar_settings";

const wchar_t UserSettings::enableToolbarSetting[]			= L"enable_toolbar";
const wchar_t UserSettings::setAsFirstTBSetting[]			= L"set_as_first_tb";
const wchar_t UserSettings::compareTBSetting[]				= L"compare_tb";
const wchar_t UserSettings::compareSelTBSetting[]			= L"compare_selection_tb";
const wchar_t UserSettings::clearCompareTBSetting[]			= L"clear_compare_tb";
const wchar_t UserSettings::navigationTBSetting[]			= L"navigation_tb";
const wchar_t UserSettings::diffsFilterTBSetting[]			= L"diffs_filter_tb";
const wchar_t UserSettings::navBarTBSetting[]				= L"nav_bar_tb";


constexpr int UserSettings::MaxRegexStrSize()
{
	return ((cMaxRegexLen * cMaxRegexHistory) +
			((_countof(cRegexEntriesDelimiter) - 1) * (cMaxRegexHistory - 1)) + 1);
}


void UserSettings::load()
{
	dirty = false;

	wchar_t ini[MAX_PATH];

	::SendMessageW(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(ini), (LPARAM)ini);

	::PathAppendW(ini, L"ComparePlus.ini");

	FirstFileIsNew			= ::GetPrivateProfileIntW(mainSection, firstIsNewSetting,
			DEFAULT_FIRST_IS_NEW, ini) != 0;
	NewFileViewId			= ::GetPrivateProfileIntW(mainSection, newFileViewSetting,
			DEFAULT_NEW_IN_SUB_VIEW, ini) == 0 ? MAIN_VIEW : SUB_VIEW;
	CompareToPrev			= ::GetPrivateProfileIntW(mainSection, compareToPrevSetting,
			DEFAULT_COMPARE_TO_PREV, ini) != 0;
	EncodingsCheck			= ::GetPrivateProfileIntW(mainSection, encodingsCheckSetting,
			DEFAULT_ENCODINGS_CHECK, ini) != 0;
	SizesCheck				= ::GetPrivateProfileIntW(mainSection, sizesCheckSetting,
			DEFAULT_SIZES_CHECK, ini) != 0;
	ManualSyncCheck			= ::GetPrivateProfileIntW(mainSection, manualSyncCheckSetting,
			DEFAULT_MANUAL_SYNC_CHECK, ini) != 0;
	PromptToCloseOnMatch	= ::GetPrivateProfileIntW(mainSection, promptCloseOnMatchSetting,
			DEFAULT_PROMPT_CLOSE_ON_MATCH, ini) != 0;
	HideMargin				= ::GetPrivateProfileIntW(mainSection, hideMarginSetting,
			DEFAULT_HIDE_MARGIN, ini) != 0;
	NeverMarkIgnored		= ::GetPrivateProfileIntW(mainSection, markIgnoredLinesSetting,
			DEFAULT_NEVER_MARK_IGNORED, ini) != 0;
	FollowingCaret			= ::GetPrivateProfileIntW(mainSection, followingCaretSetting,
			DEFAULT_FOLLOWING_CARET, ini) != 0;
	WrapAround				= ::GetPrivateProfileIntW(mainSection, wrapAroundSetting,
			DEFAULT_WRAP_AROUND, ini) != 0;
	GotoFirstDiff			= ::GetPrivateProfileIntW(mainSection, gotoFirstDiffSetting,
			DEFAULT_GOTO_FIRST_DIFF, ini) != 0;

	DetectMoves				= ::GetPrivateProfileIntW(mainSection, detectMovesSetting,			 1, ini) != 0;
	DetectSubBlockDiffs		= ::GetPrivateProfileIntW(mainSection, detectSubBlockDiffsSetting,	 1, ini) != 0;
	DetectSubLineMoves		= ::GetPrivateProfileIntW(mainSection, detectSubLineMovesSetting,	 1, ini) != 0;
	DetectCharDiffs			= ::GetPrivateProfileIntW(mainSection, detectCharDiffsSetting,		 0, ini) != 0;
	IgnoreEmptyLines		= ::GetPrivateProfileIntW(mainSection, ignoreEmptyLinesSetting,		 0, ini) != 0;
	IgnoreFoldedLines		= ::GetPrivateProfileIntW(mainSection, ignoreFoldedLinesSetting,	 0, ini) != 0;
	IgnoreHiddenLines		= ::GetPrivateProfileIntW(mainSection, ignoreHiddenLinesSetting,	 0, ini) != 0;
	IgnoreChangedSpaces		= ::GetPrivateProfileIntW(mainSection, ignoreChangedSpacesSetting,	 0, ini) != 0;
	IgnoreAllSpaces			= ::GetPrivateProfileIntW(mainSection, ignoreAllSpacesSetting,		 0, ini) != 0;
	IgnoreEOL				= ::GetPrivateProfileIntW(mainSection, ignoreEOLSetting,			 0, ini) != 0;
	IgnoreCase				= ::GetPrivateProfileIntW(mainSection, ignoreCaseSetting,			 0, ini) != 0;
	IgnoreRegex				= ::GetPrivateProfileIntW(mainSection, ignoreRegexSetting,			 0, ini) != 0;
	InvertRegex				= ::GetPrivateProfileIntW(mainSection, invertRegexSetting,			 0, ini) != 0;
	InclRegexNomatchLines	= ::GetPrivateProfileIntW(mainSection, inclRegexNomatchLinesSetting, 0, ini) != 0;
	HighlightRegexIgnores	= ::GetPrivateProfileIntW(mainSection, highlightRegexIgnoresSetting, 0, ini) != 0;

	std::wstring regexBuf;

	int maxRegexBufSize = (size_t)MaxRegexStrSize();

	regexBuf.resize(maxRegexBufSize);

	maxRegexBufSize =
		::GetPrivateProfileStringW(mainSection, ignoreRegexStrSetting, NULL, &regexBuf[0], maxRegexBufSize, ini);

	regexBuf.resize(maxRegexBufSize);

	for (size_t i = 0, pos1 = 0, pos2 = regexBuf.find(cRegexEntriesDelimiter);
		pos1 < (size_t)maxRegexBufSize && i < cMaxRegexHistory;
		++i, pos2 = regexBuf.find(cRegexEntriesDelimiter, pos1))
	{
		if (pos2 == std::wstring::npos)
			pos2 = (size_t)maxRegexBufSize;

		if (pos2 - pos1 > cMaxRegexLen)
		{
			--i;
			dirty = true;
		}
		else
		{
			IgnoreRegexStr[i] = std::move(regexBuf.substr(pos1, pos2 - pos1));
		}

		pos1 = pos2 + _countof(cRegexEntriesDelimiter) - 1;
	}

	HideMatches			= ::GetPrivateProfileIntW(mainSection, hideMatchesSetting,			0, ini) != 0;
	HideNewLines		= ::GetPrivateProfileIntW(mainSection, hideNewLinesSetting,			0, ini) != 0;
	HideChangedLines	= ::GetPrivateProfileIntW(mainSection, hideChangedLinesSetting,		0, ini) != 0;
	HideMovedLines		= ::GetPrivateProfileIntW(mainSection, hideMovedLinesSetting,		0, ini) != 0;
	ShowOnlySelections	= ::GetPrivateProfileIntW(mainSection, showOnlySelSetting,			1, ini) != 0;

	ShowNavBar			= ::GetPrivateProfileIntW(mainSection, navBarSetting,				1, ini) != 0;
	BookmarksAsSync		= ::GetPrivateProfileIntW(mainSection, bookmarksAsSyncSetting,		0, ini) != 0;
	RecompareOnChange	= ::GetPrivateProfileIntW(mainSection, reCompareOnChangeSetting,	1, ini) != 0;

	StatusInfo = static_cast<StatusType>(::GetPrivateProfileIntW(mainSection, statusInfoSetting,
			DEFAULT_STATUS_INFO, ini));

	if (StatusInfo >= STATUS_TYPE_END)
		StatusInfo = static_cast<StatusType>(DEFAULT_STATUS_INFO);

	colorsLight.added						= ::GetPrivateProfileIntW(colorsSection, addedColorSetting,
			DEFAULT_ADDED_COLOR, ini);
	colorsLight.removed						= ::GetPrivateProfileIntW(colorsSection, removedColorSetting,
			DEFAULT_REMOVED_COLOR, ini);
	colorsLight.moved						= ::GetPrivateProfileIntW(colorsSection, movedColorSetting,
			DEFAULT_MOVED_COLOR, ini);
	colorsLight.changed						= ::GetPrivateProfileIntW(colorsSection, changedColorSetting,
			DEFAULT_CHANGED_COLOR, ini);
	colorsLight.added_part					= ::GetPrivateProfileIntW(colorsSection, addedPartColorSetting,
			DEFAULT_PART_COLOR, ini);
	colorsLight.removed_part				= ::GetPrivateProfileIntW(colorsSection, removedPartColorSetting,
			DEFAULT_PART_COLOR, ini);
	colorsLight.moved_part					= ::GetPrivateProfileIntW(colorsSection, movedPartColorSetting,
			DEFAULT_MOVED_PART_COLOR, ini);
	colorsLight.part_transparency			= ::GetPrivateProfileIntW(colorsSection, partTranspSetting,
			DEFAULT_PART_TRANSP, ini);
	colorsLight.caret_line_transparency		= ::GetPrivateProfileIntW(colorsSection, caretLineTranspSetting,
			DEFAULT_CARET_LINE_TRANSP, ini);

	colorsDark.added						= ::GetPrivateProfileIntW(colorsSection, addedColorDarkSetting,
			DEFAULT_ADDED_COLOR_DARK, ini);
	colorsDark.removed						= ::GetPrivateProfileIntW(colorsSection, removedColorDarkSetting,
			DEFAULT_REMOVED_COLOR_DARK, ini);
	colorsDark.moved						= ::GetPrivateProfileIntW(colorsSection, movedColorDarkSetting,
			DEFAULT_MOVED_COLOR_DARK, ini);
	colorsDark.changed						= ::GetPrivateProfileIntW(colorsSection, changedColorDarkSetting,
			DEFAULT_CHANGED_COLOR_DARK, ini);
	colorsDark.added_part					= ::GetPrivateProfileIntW(colorsSection, addedPartColorDarkSetting,
			DEFAULT_PART_COLOR_DARK, ini);
	colorsDark.removed_part					= ::GetPrivateProfileIntW(colorsSection, removedPartColorDarkSetting,
			DEFAULT_PART_COLOR_DARK, ini);
	colorsDark.moved_part					= ::GetPrivateProfileIntW(colorsSection, movedPartColorDarkSetting,
			DEFAULT_MOVED_PART_COLOR_DARK, ini);
	colorsDark.part_transparency			= ::GetPrivateProfileIntW(colorsSection, highlightTranspDarkSetting,
			DEFAULT_PART_TRANSP_DARK, ini);
	colorsDark.caret_line_transparency		= ::GetPrivateProfileIntW(colorsSection, caretLineTranspDarkSetting,
			DEFAULT_CARET_LINE_TRANSP_DARK, ini);

	ChangedResemblPercent	= ::GetPrivateProfileIntW(colorsSection, changedResemblSetting,
			DEFAULT_CHANGED_RESEMBLANCE, ini);

	EnableToolbar		= ::GetPrivateProfileIntW(toolbarSection, enableToolbarSetting,
			DEFAULT_ENABLE_TOOLBAR_TB, ini) != 0;
	SetAsFirstTB		= ::GetPrivateProfileIntW(toolbarSection, setAsFirstTBSetting,
			DEFAULT_SET_AS_FIRST_TB, ini) != 0;
	CompareTB			= ::GetPrivateProfileIntW(toolbarSection, compareTBSetting,
			DEFAULT_COMPARE_TB, ini) != 0;
	CompareSelTB		= ::GetPrivateProfileIntW(toolbarSection, compareSelTBSetting,
			DEFAULT_COMPARE_SEL_TB, ini) != 0;
	ClearCompareTB		= ::GetPrivateProfileIntW(toolbarSection, clearCompareTBSetting,
			DEFAULT_CLEAR_COMPARE_TB, ini) != 0;
	NavigationTB		= ::GetPrivateProfileIntW(toolbarSection, navigationTBSetting,
			DEFAULT_NAVIGATION_TB, ini) != 0;
	DiffsFilterTB		= ::GetPrivateProfileIntW(toolbarSection, diffsFilterTBSetting,
			DEFAULT_DIFFS_FILTER_TB, ini) != 0;
	NavBarTB			= ::GetPrivateProfileIntW(toolbarSection, navBarTBSetting,
			DEFAULT_NAV_BAR_TB, ini) != 0;
}


void UserSettings::save()
{
	if (!dirty)
		return;

	wchar_t ini[MAX_PATH];

	::SendMessageW(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, (WPARAM)_countof(ini), (LPARAM)ini);

	if (::PathFileExistsW(ini) == FALSE)
	{
		if (::CreateDirectoryW(ini, NULL) == FALSE)
		{
			wchar_t msg[MAX_PATH + 128];

			_snwprintf_s(msg, _countof(msg), _TRUNCATE, Strings::get()["CONFIG_DIR_FAIL"].c_str(), ini);
			::MessageBoxW(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK | MB_ICONWARNING);

			return;
		}
	}

	::PathAppendW(ini, L"ComparePlus.ini");

	// Make sure the ini config file has UNICODE encoding to be able to store ignore regex Unicode string
	{
		FILE* fp;

		_wfopen_s(&fp, ini, L"w, ccs=UTF-8");

		if (fp)
			fclose(fp);
	}

	if (!::WritePrivateProfileStringW(mainSection, firstIsNewSetting, FirstFileIsNew ? L"1" : L"0", ini))
	{
		wchar_t msg[MAX_PATH + 64];

		_snwprintf_s(msg, _countof(msg), _TRUNCATE, Strings::get()["CONFIG_WRITE_FAIL"].c_str(), ini);
		::MessageBoxW(nppData._nppHandle, msg, PLUGIN_NAME, MB_OK | MB_ICONWARNING);

		return;
	}

	::WritePrivateProfileStringW(mainSection, newFileViewSetting,	NewFileViewId == SUB_VIEW	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, compareToPrevSetting,			CompareToPrev		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, encodingsCheckSetting,		EncodingsCheck		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, sizesCheckSetting,			SizesCheck			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, manualSyncCheckSetting,		ManualSyncCheck		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, promptCloseOnMatchSetting,	PromptToCloseOnMatch  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, hideMarginSetting,			HideMargin	  		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, markIgnoredLinesSetting,		NeverMarkIgnored	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, followingCaretSetting,		FollowingCaret		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, wrapAroundSetting,			WrapAround			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, gotoFirstDiffSetting,			GotoFirstDiff		  ? L"1" : L"0", ini);

	::WritePrivateProfileStringW(mainSection, detectMovesSetting,			DetectMoves			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, detectSubBlockDiffsSetting,	DetectSubBlockDiffs	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, detectSubLineMovesSetting,	DetectSubLineMoves	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, detectCharDiffsSetting,		DetectCharDiffs		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreEmptyLinesSetting,		IgnoreEmptyLines	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreFoldedLinesSetting,		IgnoreFoldedLines	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreHiddenLinesSetting,		IgnoreHiddenLines	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreChangedSpacesSetting,	IgnoreChangedSpaces	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreAllSpacesSetting,		IgnoreAllSpaces		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreEOLSetting,				IgnoreEOL			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreCaseSetting,			IgnoreCase			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, ignoreRegexSetting,			IgnoreRegex			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, invertRegexSetting,			InvertRegex			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, inclRegexNomatchLinesSetting,	InclRegexNomatchLines ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, highlightRegexIgnoresSetting,	HighlightRegexIgnores ? L"1" : L"0", ini);

	{
		std::wstring regexBuf = IgnoreRegexStr[0];

		for (size_t i = 1; i < cMaxRegexHistory && IgnoreRegexStr[i].size(); ++i)
		{
			regexBuf += cRegexEntriesDelimiter;
			regexBuf += IgnoreRegexStr[i];
		}

		::WritePrivateProfileStringW(mainSection, ignoreRegexStrSetting, regexBuf.c_str(), ini);
	}

	::WritePrivateProfileStringW(mainSection, hideMatchesSetting,			HideMatches			  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, hideNewLinesSetting,			HideNewLines		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, hideChangedLinesSetting,		HideChangedLines	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, hideMovedLinesSetting,		HideMovedLines		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, showOnlySelSetting,			ShowOnlySelections	  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, navBarSetting,				ShowNavBar			  ? L"1" : L"0", ini);

	::WritePrivateProfileStringW(mainSection, bookmarksAsSyncSetting,		BookmarksAsSync		  ? L"1" : L"0", ini);
	::WritePrivateProfileStringW(mainSection, reCompareOnChangeSetting,		RecompareOnChange	  ? L"1" : L"0", ini);

	wchar_t buffer[64];

	_itow_s(static_cast<int>(StatusInfo), buffer, 64, 10);
	::WritePrivateProfileStringW(mainSection, statusInfoSetting, buffer, ini);

	_itow_s(colorsLight.added, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, addedColorSetting, buffer, ini);

	_itow_s(colorsLight.removed, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, removedColorSetting, buffer, ini);

	_itow_s(colorsLight.moved, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, movedColorSetting, buffer, ini);

	_itow_s(colorsLight.changed, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, changedColorSetting, buffer, ini);

	_itow_s(colorsLight.added_part, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, addedPartColorSetting, buffer, ini);

	_itow_s(colorsLight.removed_part, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, removedPartColorSetting, buffer, ini);

	_itow_s(colorsLight.moved_part, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, movedPartColorSetting, buffer, ini);

	_itow_s(colorsLight.part_transparency, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, partTranspSetting, buffer, ini);

	_itow_s(colorsLight.caret_line_transparency, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, caretLineTranspSetting, buffer, ini);

	_itow_s(colorsDark.added, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, addedColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.removed, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, removedColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.moved, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, movedColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.changed, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, changedColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.added_part, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, addedPartColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.removed_part, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, removedPartColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.moved_part, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, movedPartColorDarkSetting, buffer, ini);

	_itow_s(colorsDark.part_transparency, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, highlightTranspDarkSetting, buffer, ini);

	_itow_s(colorsDark.caret_line_transparency, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, caretLineTranspDarkSetting, buffer, ini);

	_itow_s(ChangedResemblPercent, buffer, 64, 10);
	::WritePrivateProfileStringW(colorsSection, changedResemblSetting, buffer, ini);

	::WritePrivateProfileStringW(toolbarSection, enableToolbarSetting,	EnableToolbar	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, setAsFirstTBSetting,	SetAsFirstTB	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, compareTBSetting,		CompareTB		? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, compareSelTBSetting,	CompareSelTB	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, clearCompareTBSetting,	ClearCompareTB	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, navigationTBSetting,	NavigationTB	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, diffsFilterTBSetting,	DiffsFilterTB	? L"1" : L"0", ini);
	::WritePrivateProfileStringW(toolbarSection, navBarTBSetting,		NavBarTB		? L"1" : L"0", ini);

	dirty = false;
}


void UserSettings::moveRegexToHistory(std::wstring&& newRegex)
{
	int i = 0;

	for (; i < cMaxRegexHistory && IgnoreRegexStr[i].size() && !(IgnoreRegexStr[i] == newRegex); ++i);

	if (i < cMaxRegexHistory)
		--i;
	else
		i = cMaxRegexHistory - 2;

	for (; i >= 0; --i)
		IgnoreRegexStr[i + 1] = std::move(IgnoreRegexStr[i]);

	IgnoreRegexStr[0] = std::move(newRegex);
}
