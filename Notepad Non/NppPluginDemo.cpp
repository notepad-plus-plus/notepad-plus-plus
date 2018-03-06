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

#include "PluginDefinition.h"
#include <shlwapi.h>
#include "GoToLineDlg.h"
#include <windows.h>
#include <iostream>
#include "dragbase.h"
extern FuncItem funcItem[nbFunc];
extern NppData nppData;
extern bool doCloseTag;

extern DemoDlg _goToLine;

INT_PTR CALLBACK InputDialogProc(
	__in  HWND hwndDlg,
	__in  UINT uMsg,
	__in  WPARAM wParam,
	__in  LPARAM lParam
);

// Param struct passed to the callback procs
struct CallbackParam
{
	HWND hwnd;
	HINSTANCE hinst;
	bool * new_document;
};

const unsigned MAX_TEXT_SIZE = 1000000;

// *** Our save function which is also used by dragbase
BOOL __stdcall Save(const char * const filename, const char * const directory, void * param)
{
	HWND hNotepad_plus = FindWindow(TEXT("Notepad++"), NULL);
	int which = -1;
	::SendMessage(hNotepad_plus, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	if (which == -1)
		return false;
	HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
	int text_len = ::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0);
	char *text_buf = new char[text_len + 1];
	::SendMessage(curScintilla, SCI_GETTEXT, text_len + 1, (LPARAM)text_buf);

	DWORD num_of_bytes_written;
	TCHAR new_filename[MAX_PATH];
	TCHAR wFilename[MAX_PATH];
	TCHAR file_extension[MAX_PATH];
	char new_filename_in_chars[MAX_PATH];
	char final_save_path[MAX_PATH];
	::SendMessage(hNotepad_plus, NPPM_GETFILENAME, MAX_PATH, (LPARAM)new_filename);
	wcstombs(new_filename_in_chars, new_filename, MAX_PATH);
	SetFilename(new_filename_in_chars);
	strcpy(final_save_path, directory);
	strcat(final_save_path, new_filename_in_chars);
	mbstowcs(wFilename, final_save_path, strlen(final_save_path) + 1);
	HANDLE tmp_file = CreateFile(wFilename, GENERIC_WRITE, FILE_SHARE_WRITE,
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(tmp_file, text_buf, text_len, &num_of_bytes_written, NULL);
	CloseHandle(tmp_file);
	delete text_buf;
	return true;
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  reasonForCall, 
                       LPVOID lpReserved )
{
	HWND hNotepad_plus = FindWindow(TEXT("Notepad++"), NULL);
	Create(hNotepad_plus, Save,"Testing",NULL);
    switch (reasonForCall)
    {

      case DLL_PROCESS_ATTACH:
        pluginInit(hModule);
        break;

      case DLL_PROCESS_DETACH:
		commandMenuCleanUp();
        pluginCleanUp();
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}


extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;
	commandMenuInit();
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}


extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	switch (notifyCode->nmhdr.code) 
	{
		case SCN_CHARADDED:
		{
			LangType docType;
			::SendMessage(nppData._nppHandle, NPPM_GETCURRENTLANGTYPE, 0, (LPARAM)&docType);
			bool isDocTypeHTML = (docType == L_HTML || docType == L_XML || docType == L_PHP);
			if (doCloseTag && isDocTypeHTML)
			{
				if (notifyCode->ch == '>')
				{
					char buf[512];
					int currentEdit;
					::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
					HWND hCurrentEditView = (currentEdit == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
					int currentPos = int(::SendMessage(hCurrentEditView, SCI_GETCURRENTPOS, 0, 0));
					int beginPos = currentPos - (sizeof(buf) - 1);
					int startPos = (beginPos > 0)?beginPos:0;
					int size = currentPos - startPos;
					int insertStringSize = 2;
					char insertString[516] = "</";

					if (size >= 3) 
					{
						struct TextRange tr = {{startPos, currentPos}, buf};

						::SendMessage(hCurrentEditView, SCI_GETTEXTRANGE, 0, (LPARAM)&tr);

						if (buf[size-2] != '/') 
						{

							const char *pBegin = &buf[0];
							const char *pCur = &buf[size - 2];
							int  insertStringSize = 2;

							for (; pCur > pBegin && *pCur != '<' && *pCur != '>' ;)
								pCur--;
								

							if (*pCur == '<')
							{
								pCur++;
								
								while (StrChrA(":_-.", *pCur) || IsCharAlphaNumeric(*pCur))
								{
									insertString[insertStringSize++] = *pCur;
									pCur++;
								}
							}

							insertString[insertStringSize++] = '>';
							insertString[insertStringSize] = '\0';

							if (insertStringSize > 3)
							{				
								::SendMessage(hCurrentEditView, SCI_BEGINUNDOACTION, 0, 0);
								::SendMessage(hCurrentEditView, SCI_REPLACESEL, 0, (LPARAM)insertString);
								::SendMessage(hCurrentEditView, SCI_SETSEL, currentPos, currentPos);
								::SendMessage(hCurrentEditView, SCI_ENDUNDOACTION, 0, 0);
							}
						}
					}	
				}
			}
		}
		break;
	}
}


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{/*
	if (Message == WM_MOVE)
	{
		::MessageBox(NULL, "move", "", MB_OK);
	}
*/
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
