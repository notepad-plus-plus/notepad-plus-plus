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


#include "precompiledHeaders.h"
#include "functionListPanel.h"
#include "ScintillaEditView.h"

void FunctionListPanel::addEntry(const TCHAR *displayText)
{
	int index = ::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_GETCOUNT, 0, 0);
	::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_INSERTSTRING, index, (LPARAM)displayText);
}

void FunctionListPanel::removeAllEntries()
{
	while (::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_GETCOUNT, 0, 0))
		::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_DELETESTRING, 0, 0);
}


void FunctionListPanel::parse(vector<foundInfo> & foundInfos, size_t begin, size_t end, const TCHAR *wordToExclude, const TCHAR *regExpr2search, vector< generic_string > dataToSearch, vector< generic_string > data2ToSearch)
{
	if (begin >= end)
		return;

	int flags = SCFIND_REGEXP | SCFIND_POSIX;

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	int targetStart = (*_ppEditView)->searchInTarget(regExpr2search, lstrlen(regExpr2search), begin, end);
	int targetEnd = 0;
	
	foundInfos.clear();
	while (targetStart != -1 && targetStart != -2)
	{
		targetStart = int((*_ppEditView)->execute(SCI_GETTARGETSTART));
		targetEnd = int((*_ppEditView)->execute(SCI_GETTARGETEND));
		if (targetEnd > int(end)) //we found a result but outside our range, therefore do not process it
		{
			break;
		}
		int foundTextLen = targetEnd - targetStart;
		if (targetStart + foundTextLen == int(end))
            break;

		foundInfo fi;

		// dataToSearch & data2ToSearch are optional
		if (!dataToSearch.size() && !data2ToSearch.size())
		{
			TCHAR foundData[1024];
			(*_ppEditView)->getGenericText(foundData, 1024, targetStart, targetEnd);

			fi._data = foundData; // whole found data
			fi._pos = targetStart;

		}
		else
		{
			int foundPos;
			if (dataToSearch.size())
			{
				fi._data = parseSubLevel(targetStart, targetEnd, wordToExclude, dataToSearch, foundPos);
				fi._pos = foundPos;
			}

			if (data2ToSearch.size())
			{
				fi._data2 = parseSubLevel(targetStart, targetEnd, wordToExclude, data2ToSearch, foundPos);
				fi._pos2 = foundPos;
			}
		}

		if (fi._pos != -1 || fi._pos2 != -1) // at least one should be found
		{
			foundInfos.push_back(fi);
		}
		
		begin = targetStart + foundTextLen;
		targetStart = (*_ppEditView)->searchInTarget(regExpr2search, lstrlen(regExpr2search), begin, end);
	}
}


generic_string FunctionListPanel::parseSubLevel(size_t begin, size_t end, const TCHAR *wordToExclude, std::vector< generic_string > dataToSearch, int & foundPos)
{
	if (begin >= end)
	{
		foundPos = -1;
		return TEXT("");
	}

	int flags = SCFIND_REGEXP | SCFIND_POSIX;

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	const TCHAR *regExpr2search = dataToSearch[0].c_str();
	int targetStart = (*_ppEditView)->searchInTarget(regExpr2search, lstrlen(regExpr2search), begin, end);

	if (targetStart == -1 || targetStart == -2)
	{
		foundPos = -1;
		return TEXT("");
	}
	int targetEnd = int((*_ppEditView)->execute(SCI_GETTARGETEND));

	if (dataToSearch.size() >= 2)
	{
		dataToSearch.erase(dataToSearch.begin());
		return parseSubLevel(targetStart, targetEnd, wordToExclude, dataToSearch, foundPos);
	}
	else // only one processed element, so we conclude the result
	{
		TCHAR foundStr[1024];

		(*_ppEditView)->getGenericText(foundStr, 1024, targetStart, targetEnd);

		if (!isInList(foundStr, wordToExclude))
		{
			foundPos = targetStart;
			return foundStr;
		}
		else
		{
			foundPos = -1;
			return TEXT("");
		}
	}
}

void FunctionListPanel::reload()
{
	// clean up
	removeAllEntries();

	generic_string funcBegin = TEXT("^[\\s]*");
	generic_string qualifier_maybe = TEXT("((static|const)[\\s]+)?");
	generic_string returnType = TEXT("[\\w]+");
	generic_string space = TEXT("[\\s]+");
	generic_string classQualifier_maybe = TEXT("([\\w_]+[\\s]*::)?");
	generic_string funcName = TEXT("[\\w_]+");
	generic_string const_maybe = TEXT("([\\s]*const[\\s]*)?");
	generic_string space_maybe = TEXT("[\\s]*");
	generic_string params = TEXT("\\([\\n\\w_,*&\\s]*\\)");
	generic_string funcBody = TEXT("\\{");
	generic_string space_eol_maybe = TEXT("[\\n\\s]*");
	
	//const TCHAR TYPE[] = "";

	generic_string function = funcBegin + qualifier_maybe + returnType + space + classQualifier_maybe + funcName + space_maybe + params + const_maybe + space_eol_maybe + funcBody;
	generic_string secondSearch = funcName + space_maybe;
	secondSearch += TEXT("\\(");


	int docLen = (*_ppEditView)->getCurrentDocLen();
	vector<foundInfo> fi;
	vector<generic_string> regExpr1;
	vector<generic_string> regExpr2;

	regExpr1.push_back(secondSearch);
	regExpr1.push_back(funcName);

	parse(fi, 0, docLen, TEXT("if while for"),
		//TEXT("^[\\s]*[\\w]+[\\s]+[\\w]*[\\s]*([\\w_]+[\\s]*::)?[\\s]*[\\w_]+[\\s]*\\([\\n\\w_,*&\\s]*\\)[\\n\\s]*\\{"),
		function.c_str(),
		regExpr1,
		//TEXT("[\\w_]+[\\s]*\\("),
		regExpr2);

	for (size_t i = 0; i < fi.size(); i++)
	{
		addEntry(fi[i]._data.c_str());
	}
}

BOOL CALLBACK FunctionListPanel::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG :
        {		
            return TRUE;
        }

		
		case WM_DESTROY:
			break;

		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
            {
                case IDC_LIST_FUNCLIST:
				{
					if (HIWORD(wParam) == LBN_DBLCLK)
					{
						
						
					}
					return TRUE;
				}
			}
		}
		break;
		
        case WM_SIZE:
        {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
			::MoveWindow(::GetDlgItem(_hSelf, IDC_LIST_FUNCLIST), 0, 0, width, height, TRUE);
            break;
        }
/*
		case WM_VKEYTOITEM:
		{
			if (LOWORD(wParam) == VK_RETURN)
			{
				int i = ::SendDlgItemMessage(_hSelf, IDC_LIST_CLIPBOARD, LB_GETCURSEL, 0, 0);
				printInt(i);
				return TRUE;
			}//return TRUE;
			break;
		}
*/

        default :
            return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
    }
	return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
}
