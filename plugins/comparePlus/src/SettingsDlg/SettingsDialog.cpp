/*
 * This file is part of ComparePlus plugin for Notepad++
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

#pragma comment (lib, "uxtheme")


#include "SettingsDialog.h"

#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <uxtheme.h>

#include "NppHelpers.h"
#include "Strings.h"
#include "Tools.h"


static const int c_Part_transp_min			= 0;
static const int c_Part_transp_max			= 100;
static const int c_Caret_line_transp_min	= 0;
static const int c_Caret_line_transp_max	= 100;
static const int c_Change_resembl_min		= 1;
static const int c_Change_resembl_max		= 99;


UINT SettingsDialog::doDialog(UserSettings* settings)
{
	_Settings = settings;

	return (UINT)::DialogBoxParamW(_hInst, MAKEINTRESOURCEW(IDD_SETTINGS_DIALOG), _hParent,
			(DLGPROC)dlgProc, (LPARAM)this);
}


INT_PTR CALLBACK SettingsDialog::run_dlgProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			registerDlgForDarkMode(_hSelf);

			goToCenter();

			::EnableThemeDialogTexture(_hSelf, ETDT_ENABLETAB);

			_ColorAdded.init(_hInst, _hSelf, ::GetDlgItem(_hSelf, IDC_COMBO_ADDED_COLOR));
			_ColorRemoved.init(_hInst, _hSelf, ::GetDlgItem(_hSelf, IDC_COMBO_REMOVED_COLOR));
			_ColorMoved.init(_hInst, _hSelf, ::GetDlgItem(_hSelf, IDC_COMBO_MOVED_COLOR));
			_ColorChanged.init(_hInst, _hSelf, ::GetDlgItem(_hSelf, IDC_COMBO_CHANGED_COLOR));
			_ColorAddedPart.init(_hInst, _hSelf, ::GetDlgItem(_hSelf, IDC_COMBO_ADDED_PART_COLOR));
			_ColorRemovedPart.init(_hInst, _hSelf, ::GetDlgItem(_hSelf, IDC_COMBO_REMOVED_PART_COLOR));
			_ColorMovedPart.init(_hInst, _hSelf, ::GetDlgItem(_hSelf, IDC_COMBO_MOVED_PART_COLOR));

			HWND hCtrl = ::GetDlgItem(_hSelf, IDC_PART_TRANSP_EDIT);
			::SendMessageW(hCtrl, EM_SETLIMITTEXT, 3L, 0);

			hCtrl = ::GetDlgItem(_hSelf, IDC_CARET_TRANSP_EDIT);
			::SendMessageW(hCtrl, EM_SETLIMITTEXT, 3L, 0);

			hCtrl = ::GetDlgItem(_hSelf, IDC_CHANGE_RES_EDIT);
			::SendMessageW(hCtrl, EM_SETLIMITTEXT, 2L, 0);

			hCtrl = ::GetDlgItem(_hSelf, IDC_PART_TRANSP_SPIN);
			::SendMessageW(hCtrl, UDM_SETRANGE, 0L, MAKELONG(c_Part_transp_max, c_Part_transp_min));

			hCtrl = ::GetDlgItem(_hSelf, IDC_CARET_TRANSP_SPIN);
			::SendMessageW(hCtrl, UDM_SETRANGE, 0L, MAKELONG(c_Caret_line_transp_max, c_Caret_line_transp_min));

			hCtrl = ::GetDlgItem(_hSelf, IDC_CHANGE_RES_SPIN);
			::SendMessageW(hCtrl, UDM_SETRANGE, 0L, MAKELONG(c_Change_resembl_max, c_Change_resembl_min));

			// Dialog opens by default in english
			if (Strings::get().currentLocale() != "english")
				updateLocalization();

			if (isRTLwindow(nppData._nppHandle))
			{
				const auto& str = Strings::get();

				updateDlgCtrlTxt(_hSelf, IDC_NEW_IN_SUB,	str["IDC_NEW_IN_SUB_RTL"].c_str(), true);
				updateDlgCtrlTxt(_hSelf, IDC_OLD_IN_SUB,	str["IDC_OLD_IN_SUB_RTL"].c_str(), true);
			}

			SetParams();

		}
		return TRUE;

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDOK:
					GetParams();
					_Settings->markAsDirty();
					::EndDialog(_hSelf, IDOK);
				return TRUE;

				case IDCANCEL:
					::EndDialog(_hSelf, IDCANCEL);
				return TRUE;

				case IDDEFAULT:
				{
					UserSettings settings;

					settings.FirstFileIsNew			= (bool) DEFAULT_FIRST_IS_NEW;
					settings.NewFileViewId			= DEFAULT_NEW_IN_SUB_VIEW ? SUB_VIEW : MAIN_VIEW;
					settings.CompareToPrev			= (bool) DEFAULT_COMPARE_TO_PREV;
					settings.EncodingsCheck			= (bool) DEFAULT_ENCODINGS_CHECK;
					settings.SizesCheck				= (bool) DEFAULT_SIZES_CHECK;
					settings.ManualSyncCheck		= (bool) DEFAULT_MANUAL_SYNC_CHECK;
					settings.PromptToCloseOnMatch	= (bool) DEFAULT_PROMPT_CLOSE_ON_MATCH;
					settings.HideMargin				= (bool) DEFAULT_HIDE_MARGIN;
					settings.NeverMarkIgnored		= (bool) DEFAULT_NEVER_MARK_IGNORED;
					settings.FollowingCaret			= (bool) DEFAULT_FOLLOWING_CARET;
					settings.WrapAround				= (bool) DEFAULT_WRAP_AROUND;
					settings.GotoFirstDiff			= (bool) DEFAULT_GOTO_FIRST_DIFF;

					settings.StatusInfo				= static_cast<StatusType>(DEFAULT_STATUS_INFO);

					if (isDarkMode())
					{
						settings.useDarkColors();

						settings.colors().added						= DEFAULT_ADDED_COLOR_DARK;
						settings.colors().removed					= DEFAULT_REMOVED_COLOR_DARK;
						settings.colors().moved						= DEFAULT_MOVED_COLOR_DARK;
						settings.colors().changed					= DEFAULT_CHANGED_COLOR_DARK;
						settings.colors().added_part				= DEFAULT_PART_COLOR_DARK;
						settings.colors().removed_part				= DEFAULT_PART_COLOR_DARK;
						settings.colors().moved_part				= DEFAULT_MOVED_PART_COLOR_DARK;
						settings.colors().part_transparency			= DEFAULT_PART_TRANSP_DARK;
						settings.colors().caret_line_transparency	= DEFAULT_CARET_LINE_TRANSP_DARK;
					}
					else
					{
						settings.useLightColors();

						settings.colors().added						= DEFAULT_ADDED_COLOR;
						settings.colors().removed					= DEFAULT_REMOVED_COLOR;
						settings.colors().moved						= DEFAULT_MOVED_COLOR;
						settings.colors().changed					= DEFAULT_CHANGED_COLOR;
						settings.colors().added_part				= DEFAULT_PART_COLOR;
						settings.colors().removed_part				= DEFAULT_PART_COLOR;
						settings.colors().moved_part				= DEFAULT_MOVED_PART_COLOR;
						settings.colors().part_transparency			= DEFAULT_PART_TRANSP;
						settings.colors().caret_line_transparency	= DEFAULT_CARET_LINE_TRANSP;
					}

					settings.ChangedResemblPercent = DEFAULT_CHANGED_RESEMBLANCE;

					settings.EnableToolbar	= (bool) DEFAULT_ENABLE_TOOLBAR_TB;
					settings.SetAsFirstTB	= (bool) DEFAULT_SET_AS_FIRST_TB;
					settings.CompareTB		= (bool) DEFAULT_COMPARE_TB;
					settings.CompareSelTB	= (bool) DEFAULT_COMPARE_SEL_TB;
					settings.ClearCompareTB	= (bool) DEFAULT_CLEAR_COMPARE_TB;
					settings.NavigationTB	= (bool) DEFAULT_NAVIGATION_TB;
					settings.DiffsFilterTB	= (bool) DEFAULT_DIFFS_FILTER_TB;
					settings.NavBarTB		= (bool) DEFAULT_NAV_BAR_TB;

					SetParams(&settings);
				}
				return TRUE;

				case IDC_COMBO_ADDED_COLOR:
					_ColorAdded.onSelect();
				return TRUE;

				case IDC_COMBO_REMOVED_COLOR:
					_ColorRemoved.onSelect();
				return TRUE;

				case IDC_COMBO_MOVED_COLOR:
					_ColorMoved.onSelect();
				return TRUE;

				case IDC_COMBO_CHANGED_COLOR:
					_ColorChanged.onSelect();
				return TRUE;

				case IDC_COMBO_ADDED_PART_COLOR:
					_ColorAddedPart.onSelect();
				return TRUE;

				case IDC_COMBO_REMOVED_PART_COLOR:
					_ColorRemovedPart.onSelect();
				return TRUE;

				case IDC_COMBO_MOVED_PART_COLOR:
					_ColorMovedPart.onSelect();
				return TRUE;

				case IDC_ENABLE_TOOLBAR:
				{
					bool enableToolbar = (Button_GetCheck(::GetDlgItem(_hSelf, IDC_ENABLE_TOOLBAR)) == BST_CHECKED);

					Button_Enable(::GetDlgItem(_hSelf, IDC_SET_AS_FIRST_TB),		enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_COMPARE_TB),				enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_COMPARE_SELECTIONS_TB),	enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_CLEAR_COMPARE_TB),		enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_NAVIGATION_TB),			enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_DIFFS_FILTERS_TB),		enableToolbar);
					Button_Enable(::GetDlgItem(_hSelf, IDC_NAV_BAR_TB),				enableToolbar);
				}
				return TRUE;

				default:
				return FALSE;
			}
		}
		break;

		case WM_NOTIFY:
		{
			if (((LPNMHDR)lParam)->code == UDN_DELTAPOS)
			{
				constexpr int cStep = 5;

				LPNMUPDOWN lpnmud = (LPNMUPDOWN) lParam;

				int newPos = lpnmud->iPos / cStep;

				if (lpnmud->iDelta < 0)
				{
					if ((lpnmud->iPos % cStep) == 0)
						--newPos;
				}
				else
				{
					++newPos;
				}

				lpnmud->iDelta = (newPos * cStep) - lpnmud->iPos;

				return TRUE;
			}
		}
		break;
	}

	return FALSE;
}


void SettingsDialog::updateLocalization()
{
	const auto& str = Strings::get();

	::SetWindowTextW(_hSelf, (std::wstring(PLUGIN_NAME) + std::wstring(L"   ") + str["SETTINGS"]).c_str());

	::SetDlgItemTextW(_hSelf, IDOK,					str["IDOK"].c_str());
	::SetDlgItemTextW(_hSelf, IDCANCEL,				str["IDCANCEL"].c_str());
	::SetDlgItemTextW(_hSelf, IDDEFAULT,			str["IDDEFAULT"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_MAIN,				str["IDC_MAIN"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_FIRST,			str["IDC_FIRST"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_FILES_POS,		str["IDC_FILES_POS"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_DEFAULT_COMPARE,	str["IDC_DEFAULT_COMPARE"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_STATUS_BAR,		str["IDC_STATUS_BAR"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_MISC,				str["IDC_MISC"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_COLORS,			str["IDC_COLORS"].c_str());
	::SetDlgItemTextW(_hSelf, IDC_TOOLBAR,			str["IDC_TOOLBAR"].c_str());

	updateDlgCtrlTxt(_hSelf, IDC_FIRST_NEW,				str["IDC_FIRST_NEW"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_FIRST_OLD,				str["IDC_FIRST_OLD"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_NEW_IN_SUB,			str["IDC_NEW_IN_SUB"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_OLD_IN_SUB,			str["IDC_OLD_IN_SUB"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_COMPARE_TO_PREV,		str["IDC_COMPARE_TO_PREV"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_COMPARE_TO_NEXT,		str["IDC_COMPARE_TO_NEXT"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_DIFFS_SUMMARY,			str["IDC_DIFFS_SUMMARY"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_COMPARE_OPTIONS,		str["IDC_COMPARE_OPTIONS"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_STATUS_DISABLED,		str["IDC_STATUS_DISABLED"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_ENCODINGS_CHECK,		str["IDC_ENCODINGS_CHECK"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_SIZES_CHECK,			str["IDC_SIZES_CHECK"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_MANUAL_SYNC_CHECK,		str["IDC_MANUAL_SYNC_CHECK"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_CLOSE_ON_MATCH,		str["IDC_CLOSE_ON_MATCH"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_HIDE_MARGIN,			str["IDC_HIDE_MARGIN"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_NEVER_MARK_IGNORED,	str["IDC_NEVER_MARK_IGNORED"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_FOLLOWING_CARET,		str["IDC_FOLLOWING_CARET"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_WRAP_AROUND,			str["IDC_WRAP_AROUND"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_GOTO_FIRST_DIFF,		str["IDC_GOTO_FIRST_DIFF"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_ENABLE_TOOLBAR,		str["IDC_ENABLE_TOOLBAR"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_SET_AS_FIRST_TB,		str["IDC_SET_AS_FIRST_TB"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_COMPARE_TB,			str["IDC_COMPARE_TB"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_COMPARE_SELECTIONS_TB,	str["IDC_COMPARE_SELECTIONS_TB"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_CLEAR_COMPARE_TB,		str["IDC_CLEAR_COMPARE_TB"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_NAVIGATION_TB,			str["IDC_NAVIGATION_TB"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_DIFFS_FILTERS_TB,		str["IDC_DIFFS_FILTERS_TB"].c_str(), true);
	updateDlgCtrlTxt(_hSelf, IDC_NAV_BAR_TB,			str["IDC_NAV_BAR_TB"].c_str(), true);

	updateDlgCtrlTxt(_hSelf, IDC_ADDED,				str["IDC_ADDED"].c_str());
	updateDlgCtrlTxt(_hSelf, IDC_REMOVED,			str["IDC_REMOVED"].c_str());
	updateDlgCtrlTxt(_hSelf, IDC_MOVED,				str["IDC_MOVED"].c_str());
	updateDlgCtrlTxt(_hSelf, IDC_CHANGED,			str["IDC_CHANGED"].c_str());
	updateDlgCtrlTxt(_hSelf, IDC_ADDED_PART,		str["IDC_ADDED_PART"].c_str());
	updateDlgCtrlTxt(_hSelf, IDC_REMOVED_PART,		str["IDC_REMOVED_PART"].c_str());
	updateDlgCtrlTxt(_hSelf, IDC_MOVED_PART,		str["IDC_MOVED_PART"].c_str());
	updateDlgCtrlTxt(_hSelf, IDC_PART_TRANSP,		str["IDC_PART_TRANSP"].c_str());
	updateDlgCtrlTxt(_hSelf, IDC_CARET_TRANSP,		str["IDC_CARET_TRANSP"].c_str());
	updateDlgCtrlTxt(_hSelf, IDC_CHANGE_RESEMBL,	str["IDC_CHANGE_RESEMBL"].c_str());
}


void SettingsDialog::SetParams(UserSettings* settings)
{
	if (settings == nullptr)
		settings = _Settings;

	Button_SetCheck(::GetDlgItem(_hSelf, settings->FirstFileIsNew ? IDC_FIRST_NEW : IDC_FIRST_OLD),
			BST_CHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->FirstFileIsNew ? IDC_FIRST_OLD : IDC_FIRST_NEW),
			BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->NewFileViewId == MAIN_VIEW ? IDC_OLD_IN_SUB : IDC_NEW_IN_SUB),
			BST_CHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->NewFileViewId == MAIN_VIEW ? IDC_NEW_IN_SUB : IDC_OLD_IN_SUB),
			BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->CompareToPrev ? IDC_COMPARE_TO_PREV : IDC_COMPARE_TO_NEXT),
			BST_CHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, settings->CompareToPrev ? IDC_COMPARE_TO_NEXT : IDC_COMPARE_TO_PREV),
			BST_UNCHECKED);

	if (settings->StatusInfo == StatusType::DIFFS_SUMMARY)
	{
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_SUMMARY),	BST_CHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_OPTIONS),	BST_UNCHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_STATUS_DISABLED),	BST_UNCHECKED);
	}
	else if (settings->StatusInfo == StatusType::COMPARE_OPTIONS)
	{
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_SUMMARY),	BST_UNCHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_OPTIONS),	BST_CHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_STATUS_DISABLED),	BST_UNCHECKED);
	}
	else
	{
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_SUMMARY),	BST_UNCHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_OPTIONS),	BST_UNCHECKED);
		Button_SetCheck(::GetDlgItem(_hSelf, IDC_STATUS_DISABLED),	BST_CHECKED);
	}

	Button_SetCheck(::GetDlgItem(_hSelf, IDC_ENCODINGS_CHECK),
			settings->EncodingsCheck ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_SIZES_CHECK),
			settings->SizesCheck ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_MANUAL_SYNC_CHECK),
			settings->ManualSyncCheck ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_CLOSE_ON_MATCH),
			settings->PromptToCloseOnMatch ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_HIDE_MARGIN),
			settings->HideMargin ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_NEVER_MARK_IGNORED),
			settings->NeverMarkIgnored ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_FOLLOWING_CARET),
			settings->FollowingCaret ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_WRAP_AROUND),
			settings->WrapAround ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_GOTO_FIRST_DIFF),
			settings->GotoFirstDiff ? BST_CHECKED : BST_UNCHECKED);

	// Set current colors configured in option dialog
	_ColorAdded.setColor(settings->colors().added);
	_ColorRemoved.setColor(settings->colors().removed);
	_ColorMoved.setColor(settings->colors().moved);
	_ColorChanged.setColor(settings->colors().changed);
	_ColorAddedPart.setColor(settings->colors().added_part);
	_ColorRemovedPart.setColor(settings->colors().removed_part);
	_ColorMovedPart.setColor(settings->colors().moved_part);

	if (settings->colors().part_transparency < c_Part_transp_min)
		settings->colors().part_transparency = c_Part_transp_min;
	else if (settings->colors().part_transparency > c_Part_transp_max)
		settings->colors().part_transparency = c_Part_transp_max;

	// Set highlight transparency
	::SetDlgItemInt(_hSelf, IDC_PART_TRANSP_EDIT, settings->colors().part_transparency, FALSE);

	if (settings->colors().caret_line_transparency < c_Caret_line_transp_min)
		settings->colors().caret_line_transparency = c_Caret_line_transp_min;
	else if (settings->colors().caret_line_transparency > c_Caret_line_transp_max)
		settings->colors().caret_line_transparency = c_Caret_line_transp_max;

	// Set caret line transparency
	::SetDlgItemInt(_hSelf, IDC_CARET_TRANSP_EDIT, settings->colors().caret_line_transparency, FALSE);

	if (settings->ChangedResemblPercent < c_Change_resembl_min)
		settings->ChangedResemblPercent = c_Change_resembl_min;
	else if (settings->ChangedResemblPercent > c_Change_resembl_max)
		settings->ChangedResemblPercent = c_Change_resembl_max;

	// Set changed threshold percentage
	::SetDlgItemInt(_hSelf, IDC_CHANGE_RES_EDIT, settings->ChangedResemblPercent, FALSE);

	Button_SetCheck(::GetDlgItem(_hSelf, IDC_ENABLE_TOOLBAR),
			settings->EnableToolbar ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_SET_AS_FIRST_TB),
			settings->SetAsFirstTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_TB),
			settings->CompareTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_SELECTIONS_TB),
			settings->CompareSelTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_CLEAR_COMPARE_TB),
			settings->ClearCompareTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_NAVIGATION_TB),
			settings->NavigationTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_FILTERS_TB),
			settings->DiffsFilterTB ? BST_CHECKED : BST_UNCHECKED);
	Button_SetCheck(::GetDlgItem(_hSelf, IDC_NAV_BAR_TB),
			settings->NavBarTB ? BST_CHECKED : BST_UNCHECKED);

	Button_Enable(::GetDlgItem(_hSelf, IDC_SET_AS_FIRST_TB),		settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_COMPARE_TB),				settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_COMPARE_SELECTIONS_TB),	settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_CLEAR_COMPARE_TB),		settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_NAVIGATION_TB),			settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_DIFFS_FILTERS_TB),		settings->EnableToolbar);
	Button_Enable(::GetDlgItem(_hSelf, IDC_NAV_BAR_TB),				settings->EnableToolbar);
}


void SettingsDialog::GetParams()
{
	_Settings->FirstFileIsNew		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_FIRST_NEW)) == BST_CHECKED);
	_Settings->NewFileViewId		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_NEW_IN_SUB)) == BST_CHECKED) ?
			SUB_VIEW : MAIN_VIEW;
	_Settings->CompareToPrev		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_TO_PREV)) == BST_CHECKED);

	if (Button_GetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_SUMMARY)) == BST_CHECKED)
		_Settings->StatusInfo = StatusType::DIFFS_SUMMARY;
	else if (Button_GetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_OPTIONS)) == BST_CHECKED)
		_Settings->StatusInfo = StatusType::COMPARE_OPTIONS;
	else
		_Settings->StatusInfo = StatusType::STATUS_DISABLED;

	_Settings->EncodingsCheck		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_ENCODINGS_CHECK)) == BST_CHECKED);
	_Settings->SizesCheck			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_SIZES_CHECK)) == BST_CHECKED);
	_Settings->ManualSyncCheck		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_MANUAL_SYNC_CHECK)) == BST_CHECKED);
	_Settings->PromptToCloseOnMatch	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_CLOSE_ON_MATCH)) == BST_CHECKED);
	_Settings->HideMargin			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_HIDE_MARGIN)) == BST_CHECKED);
	_Settings->NeverMarkIgnored		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_NEVER_MARK_IGNORED)) == BST_CHECKED);
	_Settings->FollowingCaret		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_FOLLOWING_CARET)) == BST_CHECKED);
	_Settings->WrapAround			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_WRAP_AROUND)) == BST_CHECKED);
	_Settings->GotoFirstDiff		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_GOTO_FIRST_DIFF)) == BST_CHECKED);

	// Get color chosen in dialog
	_Settings->colors().added			= _ColorAdded.getColor();
	_Settings->colors().removed			= _ColorRemoved.getColor();
	_Settings->colors().moved			= _ColorMoved.getColor();
	_Settings->colors().changed			= _ColorChanged.getColor();
	_Settings->colors().added_part 		= _ColorAddedPart.getColor();
	_Settings->colors().removed_part	= _ColorRemovedPart.getColor();
	_Settings->colors().moved_part		= _ColorMovedPart.getColor();

	// Get highlight transparency
	_Settings->colors().part_transparency = ::GetDlgItemInt(_hSelf, IDC_PART_TRANSP_EDIT, NULL, FALSE);
	int setting = _Settings->colors().part_transparency;

	if (_Settings->colors().part_transparency < c_Part_transp_min)
		_Settings->colors().part_transparency = c_Part_transp_min;
	else if (_Settings->colors().part_transparency > c_Part_transp_max)
		_Settings->colors().part_transparency = c_Part_transp_max;

	if (setting != _Settings->colors().part_transparency)
		::SetDlgItemInt(_hSelf, IDC_PART_TRANSP_EDIT, _Settings->colors().part_transparency, FALSE);

	// Get caret line transparency
	_Settings->colors().caret_line_transparency = ::GetDlgItemInt(_hSelf, IDC_CARET_TRANSP_EDIT, NULL, FALSE);
	setting = _Settings->colors().caret_line_transparency;

	if (_Settings->colors().caret_line_transparency < c_Caret_line_transp_min)
		_Settings->colors().caret_line_transparency = c_Caret_line_transp_min;
	else if (_Settings->colors().caret_line_transparency > c_Caret_line_transp_max)
		_Settings->colors().caret_line_transparency = c_Caret_line_transp_max;

	if (setting != _Settings->colors().caret_line_transparency)
		::SetDlgItemInt(_hSelf, IDC_CARET_TRANSP_EDIT, _Settings->colors().caret_line_transparency, FALSE);

	// Get changed threshold percentage
	_Settings->ChangedResemblPercent = ::GetDlgItemInt(_hSelf, IDC_CHANGE_RES_EDIT, NULL, FALSE);
	setting = _Settings->ChangedResemblPercent;

	if (_Settings->ChangedResemblPercent < c_Change_resembl_min)
		_Settings->ChangedResemblPercent = c_Change_resembl_min;
	else if (_Settings->ChangedResemblPercent > c_Change_resembl_max)
		_Settings->ChangedResemblPercent = c_Change_resembl_max;

	if (setting != _Settings->ChangedResemblPercent)
		::SetDlgItemInt(_hSelf, IDC_CHANGE_RES_EDIT, _Settings->ChangedResemblPercent, FALSE);

	_Settings->EnableToolbar	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_ENABLE_TOOLBAR)) == BST_CHECKED);
	_Settings->SetAsFirstTB		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_SET_AS_FIRST_TB)) == BST_CHECKED);
	_Settings->CompareTB		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_TB)) == BST_CHECKED);
	_Settings->CompareSelTB		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_COMPARE_SELECTIONS_TB)) == BST_CHECKED);
	_Settings->ClearCompareTB	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_CLEAR_COMPARE_TB)) == BST_CHECKED);
	_Settings->NavigationTB		= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_NAVIGATION_TB)) == BST_CHECKED);
	_Settings->DiffsFilterTB	= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_DIFFS_FILTERS_TB)) == BST_CHECKED);
	_Settings->NavBarTB			= (Button_GetCheck(::GetDlgItem(_hSelf, IDC_NAV_BAR_TB)) == BST_CHECKED);
}