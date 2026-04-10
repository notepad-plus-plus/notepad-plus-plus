/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C)2017-2025 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <windows.h>
#include <wchar.h>
#include <string>


// Those are interpreted as bool values
#define DEFAULT_FIRST_IS_NEW				1
#define DEFAULT_NEW_IN_SUB_VIEW				1
#define DEFAULT_COMPARE_TO_PREV				1

#define DEFAULT_ENCODINGS_CHECK				1
#define DEFAULT_SIZES_CHECK					1
#define DEFAULT_MANUAL_SYNC_CHECK			1
#define DEFAULT_PROMPT_CLOSE_ON_MATCH		0
#define DEFAULT_HIDE_MARGIN					0
#define DEFAULT_NEVER_MARK_IGNORED			0
#define DEFAULT_FOLLOWING_CARET				1
#define DEFAULT_WRAP_AROUND					0
#define DEFAULT_GOTO_FIRST_DIFF				1

#define DEFAULT_STATUS_INFO					0

#define DEFAULT_ADDED_COLOR					0xC6FFC6
#define DEFAULT_REMOVED_COLOR				0xC6C6FF
#define DEFAULT_MOVED_COLOR					0xFFE6CC
#define DEFAULT_CHANGED_COLOR				0x98E7E7
#define DEFAULT_PART_COLOR					0x0683FF
#define DEFAULT_MOVED_PART_COLOR			0xF58742
#define DEFAULT_PART_TRANSP					0
#define DEFAULT_CARET_LINE_TRANSP			60

#define DEFAULT_ADDED_COLOR_DARK			0x055A05
#define DEFAULT_REMOVED_COLOR_DARK			0x16164F
#define DEFAULT_MOVED_COLOR_DARK			0x4F361C
#define DEFAULT_CHANGED_COLOR_DARK			0x145050
#define DEFAULT_PART_COLOR_DARK				0x0683FF
#define DEFAULT_MOVED_PART_COLOR_DARK		0xF58742
#define DEFAULT_PART_TRANSP_DARK			0
#define DEFAULT_CARET_LINE_TRANSP_DARK		80

#define DEFAULT_CHANGED_RESEMBLANCE			20

#define DEFAULT_ENABLE_TOOLBAR_TB			1
#define DEFAULT_SET_AS_FIRST_TB				1
#define DEFAULT_COMPARE_TB					1
#define DEFAULT_COMPARE_SEL_TB				1
#define DEFAULT_CLEAR_COMPARE_TB			1
#define DEFAULT_NAVIGATION_TB				1
#define DEFAULT_DIFFS_FILTER_TB				1
#define DEFAULT_NAV_BAR_TB					1


enum StatusType
{
	DIFFS_SUMMARY = 0,
	COMPARE_OPTIONS,
	STATUS_DISABLED,
	STATUS_TYPE_END
};


struct ColorSettings
{
	int added;
	int removed;
	int changed;
	int moved;
	int blank;
    int _default;
	int added_part;
	int removed_part;
	int moved_part;
	int part_transparency;
	int caret_line_transparency;
};


struct UserSettings
{
public:
	UserSettings() : _colors(&colorsLight) {}

	void load();
	void save();

	inline void markAsDirty()
	{
		dirty = true;
	}

	inline void useLightColors()
	{
		_colors = &colorsLight;
	}

	inline void useDarkColors()
	{
		_colors = &colorsDark;
	}

	inline ColorSettings& colors()
	{
		return *_colors;
	}

	void moveRegexToHistory(std::wstring&& newRegex);

	static constexpr int cMaxRegexLen		= 2047;
	static constexpr int cMaxRegexHistory	= 5;

	bool			FirstFileIsNew;
	int				NewFileViewId;
	bool			CompareToPrev;

	bool			EncodingsCheck;
	bool			SizesCheck;
	bool			ManualSyncCheck;
	bool			PromptToCloseOnMatch;
	bool			HideMargin;
	bool			NeverMarkIgnored;
	bool			FollowingCaret;
	bool			WrapAround;
	bool			GotoFirstDiff;

	bool			DetectMoves;
	bool			DetectSubBlockDiffs;
	bool			DetectSubLineMoves;
	bool			DetectCharDiffs;
	bool			IgnoreEmptyLines;
	bool			IgnoreFoldedLines;
	bool			IgnoreHiddenLines;
	bool			IgnoreChangedSpaces;
	bool			IgnoreAllSpaces;
	bool			IgnoreEOL;
	bool			IgnoreCase;
	bool			IgnoreRegex;
	bool			InvertRegex;
	bool			InclRegexNomatchLines;
	bool			HighlightRegexIgnores;
	std::wstring	IgnoreRegexStr[cMaxRegexHistory];

	bool			HideMatches;
	bool			HideNewLines;
	bool			HideChangedLines;
	bool			HideMovedLines;
	bool			ShowOnlySelections;
	bool			ShowNavBar;

	bool			BookmarksAsSync;
	bool			RecompareOnChange;
	StatusType		StatusInfo;

	int				ChangedResemblPercent;

	bool			EnableToolbar;
	bool			SetAsFirstTB;
	bool			CompareTB;
	bool			CompareSelTB;
	bool			ClearCompareTB;
	bool			NavigationTB;
	bool			DiffsFilterTB;
	bool			NavBarTB;

private:
	static const wchar_t cRegexEntriesDelimiter[];

	static const wchar_t mainSection[];

	static const wchar_t firstIsNewSetting[];
	static const wchar_t newFileViewSetting[];
	static const wchar_t compareToPrevSetting[];

	static const wchar_t encodingsCheckSetting[];
	static const wchar_t sizesCheckSetting[];
	static const wchar_t manualSyncCheckSetting[];
	static const wchar_t promptCloseOnMatchSetting[];
	static const wchar_t hideMarginSetting[];
	static const wchar_t markIgnoredLinesSetting[];
	static const wchar_t followingCaretSetting[];
	static const wchar_t wrapAroundSetting[];
	static const wchar_t gotoFirstDiffSetting[];

	static const wchar_t detectMovesSetting[];
	static const wchar_t detectSubBlockDiffsSetting[];
	static const wchar_t detectSubLineMovesSetting[];
	static const wchar_t detectCharDiffsSetting[];
	static const wchar_t ignoreEmptyLinesSetting[];
	static const wchar_t ignoreFoldedLinesSetting[];
	static const wchar_t ignoreHiddenLinesSetting[];
	static const wchar_t ignoreChangedSpacesSetting[];
	static const wchar_t ignoreAllSpacesSetting[];
	static const wchar_t ignoreEOLSetting[];
	static const wchar_t ignoreCaseSetting[];
	static const wchar_t ignoreRegexSetting[];
	static const wchar_t invertRegexSetting[];
	static const wchar_t inclRegexNomatchLinesSetting[];
	static const wchar_t highlightRegexIgnoresSetting[];
	static const wchar_t ignoreRegexStrSetting[];

	static const wchar_t hideMatchesSetting[];
	static const wchar_t hideNewLinesSetting[];
	static const wchar_t hideChangedLinesSetting[];
	static const wchar_t hideMovedLinesSetting[];
	static const wchar_t showOnlySelSetting[];
	static const wchar_t navBarSetting[];

	static const wchar_t bookmarksAsSyncSetting[];
	static const wchar_t reCompareOnChangeSetting[];

	static const wchar_t statusInfoSetting[];

	static const wchar_t colorsSection[];

	static const wchar_t addedColorSetting[];
	static const wchar_t removedColorSetting[];
	static const wchar_t movedColorSetting[];
	static const wchar_t changedColorSetting[];
	static const wchar_t addedPartColorSetting[];
	static const wchar_t removedPartColorSetting[];
	static const wchar_t movedPartColorSetting[];
	static const wchar_t partTranspSetting[];
	static const wchar_t caretLineTranspSetting[];

	static const wchar_t addedColorDarkSetting[];
	static const wchar_t removedColorDarkSetting[];
	static const wchar_t movedColorDarkSetting[];
	static const wchar_t changedColorDarkSetting[];
	static const wchar_t addedPartColorDarkSetting[];
	static const wchar_t removedPartColorDarkSetting[];
	static const wchar_t movedPartColorDarkSetting[];
	static const wchar_t highlightTranspDarkSetting[];
	static const wchar_t caretLineTranspDarkSetting[];

	static const wchar_t changedResemblSetting[];

	static const wchar_t toolbarSection[];

	static const wchar_t enableToolbarSetting[];
	static const wchar_t setAsFirstTBSetting[];
	static const wchar_t compareTBSetting[];
	static const wchar_t compareSelTBSetting[];
	static const wchar_t clearCompareTBSetting[];
	static const wchar_t navigationTBSetting[];
	static const wchar_t diffsFilterTBSetting[];
	static const wchar_t navBarTBSetting[];

	static constexpr int MaxRegexStrSize();

	bool dirty {false};

	ColorSettings	colorsLight;
	ColorSettings	colorsDark;

	ColorSettings*	_colors;
};
