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

void FunctionListPanel::addEntry(const TCHAR *displayText, size_t pos)
{
	int index = ::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_GETCOUNT, 0, 0);
	::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_INSERTSTRING, index, (LPARAM)displayText);
	::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_SETITEMDATA, index, (LPARAM)pos);
}

void FunctionListPanel::removeAllEntries()
{
	while (::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_GETCOUNT, 0, 0))
		::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_DELETESTRING, 0, 0);
}

// bodyOpenSybe mbol & bodyCloseSymbol should be RE
size_t FunctionListPanel::getBodyClosePos(size_t begin, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol)
{
	size_t cntOpen = 1;

	int docLen = (*_ppEditView)->getCurrentDocLen();

	if (begin >= (size_t)docLen)
		return docLen;

	generic_string exprToSearch = TEXT("(");
	exprToSearch += bodyOpenSymbol;
	exprToSearch += TEXT("|");
	exprToSearch += bodyCloseSymbol;
	exprToSearch += TEXT(")");


	int flags = SCFIND_REGEXP | SCFIND_POSIX;

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	int targetStart = (*_ppEditView)->searchInTarget(exprToSearch.c_str(), exprToSearch.length(), begin, docLen);
	int targetEnd = 0;

	do
	{
		if (targetStart != -1 && targetStart != -2) // found open or close symbol
		{
			targetEnd = int((*_ppEditView)->execute(SCI_GETTARGETEND));

			// Now we determinate the symbol (open or close)
			int tmpStart = (*_ppEditView)->searchInTarget(bodyOpenSymbol, lstrlen(bodyOpenSymbol), targetStart, targetEnd);
			if (tmpStart != -1 && tmpStart != -2) // open symbol found 
			{
				cntOpen++;
			}
			else // if it's not open symbol, then it must be the close one
			{
				cntOpen--;
			}
		}
		else // nothing found
		{
			cntOpen = 0; // get me out of here
			targetEnd = begin;
		}

		targetStart = (*_ppEditView)->searchInTarget(exprToSearch.c_str(), exprToSearch.length(), targetEnd, docLen);

	} while (cntOpen);

	return targetEnd;
}

// This method will 
void FunctionListPanel::parse2(std::vector<foundInfo> & foundInfos, size_t begin, size_t end, const TCHAR *block, std::vector< generic_string > blockNameToSearch, const TCHAR *bodyOpenSymbol, const TCHAR *bodyCloseSymbol, const TCHAR *function, std::vector< generic_string > functionToSearch)
{
	if (begin >= end)
		return;

	int flags = SCFIND_REGEXP | SCFIND_POSIX;

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	int targetStart = (*_ppEditView)->searchInTarget(block, lstrlen(block), begin, end);
	int targetEnd = 0;
	
	//foundInfos.clear();
	while (targetStart != -1 && targetStart != -2)
	{
		targetEnd = int((*_ppEditView)->execute(SCI_GETTARGETEND));

		// Get class name
		int foundPos = 0;
		generic_string classStructName = parseSubLevel(targetStart, targetEnd, blockNameToSearch, foundPos);
		

		if (lstrcmp(bodyOpenSymbol, TEXT("")) != 0 && lstrcmp(bodyCloseSymbol, TEXT("")) != 0)
		{
			targetEnd = getBodyClosePos(targetEnd, bodyOpenSymbol, bodyCloseSymbol);
		}

		if (targetEnd > int(end)) //we found a result but outside our range, therefore do not process it
		{
			break;
		}
		int foundTextLen = targetEnd - targetStart;
		if (targetStart + foundTextLen == int(end))
            break;

		// Begin to search all method inside
		vector< generic_string > emptyArray;
		parse(foundInfos, targetStart, targetEnd, function, functionToSearch, emptyArray, classStructName);

		begin = targetStart + (targetEnd - targetStart);
		targetStart = (*_ppEditView)->searchInTarget(block, lstrlen(block), begin, end);
	}
}

void FunctionListPanel::parse(vector<foundInfo> & foundInfos, size_t begin, size_t end, const TCHAR *regExpr2search, vector< generic_string > dataToSearch, vector< generic_string > data2ToSearch, generic_string classStructName)
{
	if (begin >= end)
		return;

	int flags = SCFIND_REGEXP | SCFIND_POSIX;

	(*_ppEditView)->execute(SCI_SETSEARCHFLAGS, flags);
	int targetStart = (*_ppEditView)->searchInTarget(regExpr2search, lstrlen(regExpr2search), begin, end);
	int targetEnd = 0;
	
	//foundInfos.clear();
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
				fi._data = parseSubLevel(targetStart, targetEnd, dataToSearch, foundPos);
				fi._pos = foundPos;
			}

			if (data2ToSearch.size())
			{
				fi._data2 = parseSubLevel(targetStart, targetEnd, data2ToSearch, foundPos);
				fi._pos2 = foundPos;
			}
			else if (classStructName != TEXT(""))
			{
				fi._data2 = classStructName;
				fi._pos2 = 0; // change -1 valeur for validated data2
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


generic_string FunctionListPanel::parseSubLevel(size_t begin, size_t end, std::vector< generic_string > dataToSearch, int & foundPos)
{
	if (begin >= end)
	{
		foundPos = -1;
		return TEXT("");
	}

	if (!dataToSearch.size())
		return TEXT("");

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
		return parseSubLevel(targetStart, targetEnd, dataToSearch, foundPos);
	}
	else // only one processed element, so we conclude the result
	{
		TCHAR foundStr[1024];

		(*_ppEditView)->getGenericText(foundStr, 1024, targetStart, targetEnd);

		foundPos = targetStart;
		return foundStr;
	}
}

void FunctionListPanel::reload()
{
	// clean up
	removeAllEntries();

	generic_string funcBegin = TEXT("^[\\s]*");
	generic_string qualifier_maybe = TEXT("((static|const)[\\s]+)?");
	generic_string returnType = TEXT("[\\w]+");
	generic_string space_starMaybe = TEXT("([\\s]+|\\*[\\s]+|[\\s]+\\*|[\\s]+\\*[\\s]+)");
	//generic_string space_starMaybe = TEXT("([\\s]+|\\*[\\s]+|[\\s]+\\*)");
	generic_string classQualifier_maybe = TEXT("([\\w_]+[\\s]*::)?");
	generic_string funcName = TEXT("(?!(if|whil|for))[\\w_]+");
	generic_string const_maybe = TEXT("([\\s]*const[\\s]*)?");
	generic_string space_maybe = TEXT("[\\s]*");
	generic_string params = TEXT("\\([\\n\\w_,*&\\s]*\\)");
	generic_string funcBody = TEXT("\\{");
	generic_string space_eol_maybe = TEXT("[\\n\\s]*");

	generic_string function = funcBegin + qualifier_maybe + returnType + space_starMaybe + classQualifier_maybe + funcName + space_maybe + params + const_maybe + space_eol_maybe + funcBody;
	generic_string secondSearch = funcName + space_maybe;
	secondSearch += TEXT("\\(");


	int docLen = (*_ppEditView)->getCurrentDocLen();
	vector<foundInfo> fi;
	vector<generic_string> regExpr1;
	vector<generic_string> regExpr2;

	regExpr1.push_back(secondSearch);
	regExpr1.push_back(funcName);

	generic_string secondSearch_className = TEXT("[\\w_]+(?=[\\s]*::)");
	regExpr2.push_back(secondSearch_className);

	generic_string classRegExpr = TEXT("^[\\t ]*(class|struct)[\\t ]+[\\w]+[\\s]*(:[\\s]*(public|protected|private)[\\s]+[\\w]+[\\s]*)?\\{");
	vector<generic_string> classRegExprArray;
	generic_string str1 = TEXT("(class|struct)[\\t ]+[\\w]+");
	generic_string str2 = TEXT("[\\t ]+[\\w]+");
	generic_string str3 = TEXT("[\\w]+");
	classRegExprArray.push_back(str1.c_str());
	classRegExprArray.push_back(str2.c_str());
	classRegExprArray.push_back(str3.c_str());
	//parse(fi, 0, docLen, function.c_str(), regExpr1, regExpr2);
	const TCHAR bodyOpenSymbol[] = TEXT("\\{");
	const TCHAR bodyCloseSymbol[] = TEXT("\\}");
	parse2(fi, 0, docLen, classRegExpr.c_str(), classRegExprArray, bodyOpenSymbol, bodyCloseSymbol, function.c_str(), regExpr1);

	for (size_t i = 0; i < fi.size(); i++)
	{
		generic_string entryName = TEXT("");
		if (fi[i]._pos2 != -1)
		{
			entryName = fi[i]._data2;
			entryName += TEXT("=>");
		}
		entryName += fi[i]._data;
		addEntry(entryName.c_str(), fi[i]._pos);
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
						int i = ::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_GETCURSEL, 0, 0);
						if (i != LB_ERR)
						{
							int pos = ::SendDlgItemMessage(_hSelf, IDC_LIST_FUNCLIST, LB_GETITEMDATA, i, (LPARAM)0);
							//printInt(pos);
							int sci_line = (*_ppEditView)->execute(SCI_LINEFROMPOSITION, pos);
							(*_ppEditView)->execute(SCI_ENSUREVISIBLE, sci_line);
							(*_ppEditView)->execute(SCI_GOTOPOS, pos);
						}
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
