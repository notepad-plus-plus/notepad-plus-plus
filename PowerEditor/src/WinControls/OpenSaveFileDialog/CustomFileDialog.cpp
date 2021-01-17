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


// Windows Vista is a minimum required version for Common File Dialogs.
// Define it only for current source file.
#if defined(_WIN32_WINNT) && (!defined(_WIN32_WINNT_VISTA) || (_WIN32_WINNT < _WIN32_WINNT_VISTA))
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#endif

#include <shobjidl.h>
#include <Shlwapi.h>	// QISearch

#include "CustomFileDialog.h"
#include "Parameters.h"

namespace // anonymous
{
	bool setDialogFolder(IFileDialog* dialog, const TCHAR* folder)
	{
		IShellItem* psi = nullptr;
		HRESULT hr = SHCreateItemFromParsingName(folder,
			0,
			IID_IShellItem,
			reinterpret_cast<void**>(&psi));
		if (SUCCEEDED(hr))
			hr = dialog->SetFolder(psi);
		return SUCCEEDED(hr);
	}

} // anonymous namespace

class FileDialogEventHandler : public IFileDialogEvents
{
public:
	static HRESULT createInstance(REFIID riid, void **ppv)
	{
		*ppv = nullptr;
		FileDialogEventHandler *pDialogEventHandler = new (std::nothrow) FileDialogEventHandler();
		HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
		if (SUCCEEDED(hr))
		{
			hr = pDialogEventHandler->QueryInterface(riid, ppv);
			pDialogEventHandler->Release();
		}
		return hr;
	}

	// IUnknown methods

	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override
	{
		static const QITAB qit[] =
		{
			QITABENT(FileDialogEventHandler, IFileDialogEvents),
			{ 0 }
		};
		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef() override
	{
		return InterlockedIncrement(&_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release() override
	{
		long cRef = InterlockedDecrement(&_cRef);
		if (!cRef)
			delete this;
		return cRef;
	}

	// IFileDialogEvents methods

	IFACEMETHODIMP OnFileOk(IFileDialog*) override
	{
		return S_OK;
	}
	IFACEMETHODIMP OnFolderChange(IFileDialog* dlg) override
	{
		// First launch order: 3. Custom controls are added but inactive.
		if (!_dialog)
			initDialog(dlg);
		return S_OK;
	}
	IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) override
	{
		// First launch order: 2. Buttons are added, correct window title.
		return S_OK;
	}
	IFACEMETHODIMP OnSelectionChange(IFileDialog*) override
	{
		// First launch order: 4. Main window is shown.
		return S_OK;
	}
	IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) override
	{
		return S_OK;
	}
	IFACEMETHODIMP OnTypeChange(IFileDialog*) override
	{
		// First launch order: 1. Inactive, window title might be wrong.
		return S_OK;
	}
	IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) override
	{
		return S_OK;
	}

private:

	// Use createInstance() instead
	FileDialogEventHandler() : _cRef(1)
	{
		_staticThis = this;
	}
	~FileDialogEventHandler()
	{
		_staticThis = nullptr;
	}
	FileDialogEventHandler(const FileDialogEventHandler&) = delete;
	FileDialogEventHandler& operator=(const FileDialogEventHandler&) = delete;
	FileDialogEventHandler(FileDialogEventHandler&&) = delete;
	FileDialogEventHandler& operator=(FileDialogEventHandler&&) = delete;

	void initDialog(IFileDialog * d)
	{
		assert(!_dialog);
		_dialog = d;
		_okButtonProc = nullptr;
		IOleWindow* pOleWnd = nullptr;
		HRESULT hr = _dialog->QueryInterface(&pOleWnd);
		if (SUCCEEDED(hr) && pOleWnd)
		{
			HWND hwndDlg = nullptr;
			hr = pOleWnd->GetWindow(&hwndDlg);
			if (SUCCEEDED(hr) && hwndDlg)
			{
				EnumChildWindows(hwndDlg, &EnumChildProc, 0);
			}
		}
	}

	// Called after the user input but before OnFileOk() and before any name validation.
	void onPreFileOk()
	{
		if (!_dialog)	// Unlikely.
			return;
		// Get the entered name.
		PWSTR pszFilePath = nullptr;
		_dialog->GetFileName(&pszFilePath);
		generic_string fileName = pszFilePath;
		CoTaskMemFree(pszFilePath);
		// Transform to a Windows path.
		std::replace(fileName.begin(), fileName.end(), '/', '\\');
		// Update the controls.
		if (::PathIsDirectory(fileName.c_str()))
		{
			// Name is a directory, update the address bar text.
			setDialogFolder(_dialog, fileName.c_str());
			// Empty the edit box.
			_dialog->SetFileName(L"");
		}
		else
		{
			// Name is a file path, update the edit box text.
			_dialog->SetFileName(fileName.c_str());
		}
	}

	// Enumerates the child windows of a dialog.
	// Sets up window procedure overrides for "OK" button and file name edit box.
	static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM)
	{
		const int bufferLen = MAX_PATH;
		static TCHAR buffer[bufferLen];
		if (GetClassName(hwnd, buffer, bufferLen) != 0)
		{
			if (lstrcmpi(buffer, _T("ComboBox")) == 0)
			{
				// The edit box of interest is a child of the combo box and has empty window text.
				// Note that file type dropdown is a combo box also (but without an edit box).
				HWND hwndChild = FindWindowEx(hwnd, nullptr, _T("Edit"), _T(""));
				if (hwndChild)
					_fileNameProc = (WNDPROC)SetWindowLongPtr(hwndChild, GWLP_WNDPROC, (LPARAM)&FileNameWndProc);
			}
			else if (lstrcmpi(buffer, _T("Button")) == 0)
			{
				// The button of interest has a focus by default.
				LONG style = GetWindowLong(hwnd, GWL_STYLE);
				if (style & BS_DEFPUSHBUTTON)
					_okButtonProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LPARAM)&OkButtonWndProc);
			}
		}
		if (_okButtonProc && _fileNameProc)
			return FALSE;	// Found all children, stop enumeration.
		return TRUE;
	}

	static LRESULT CALLBACK OkButtonWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (msg == WM_LBUTTONDOWN)
			_staticThis->onPreFileOk();
		return CallWindowProc(_okButtonProc, hwnd, msg, wparam, lparam);
	}

	static LRESULT CALLBACK FileNameWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		// WM_KEYDOWN isn't delivered here. So watch the input while we have focus.
		static bool checkInput = false;
		switch (msg)
		{
		case WM_SETFOCUS:
			checkInput = true;
			break;
		case WM_KILLFOCUS:
			checkInput = false;
			break;
		}
		if (checkInput)
		{
			SHORT state = GetAsyncKeyState(VK_RETURN);
			if (state & 0x8000)
			{
				_staticThis->onPreFileOk();
				checkInput = false;
			}
		}
		return CallWindowProc(_fileNameProc, hwnd, msg, wparam, lparam);
	}

	static WNDPROC _okButtonProc;
	static WNDPROC _fileNameProc;
	static FileDialogEventHandler* _staticThis;

	long _cRef;
	IFileDialog* _dialog = nullptr;
};

WNDPROC FileDialogEventHandler::_okButtonProc;
WNDPROC FileDialogEventHandler::_fileNameProc;
FileDialogEventHandler* FileDialogEventHandler::_staticThis;

// Private impelemnation to avoid pollution with includes and defines in header.
class CustomFileDialog::Impl
{
public:
	Impl()
	{
		memset(_fileName, 0, std::size(_fileName));
	}

	~Impl()
	{
		if (_customize)
			_customize->Release();
		if (_dialog)
			_dialog->Release();
	}

	bool init(CLSID id)
	{
		if (_dialog)
			return false; // Avoid double initizliation

		HRESULT hr = CoCreateInstance(id,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&_dialog));

		if (SUCCEEDED(hr) && _title)
			hr = _dialog->SetTitle(_title);

		if (SUCCEEDED(hr) && _folder)
			hr = setFolder(_folder) ? S_OK : E_FAIL;

		if (SUCCEEDED(hr) && _defExt && _defExt[0] != '\0')
			hr = _dialog->SetDefaultExtension(_defExt);

		if (SUCCEEDED(hr) && !_filterSpec.empty())
		{
			std::vector<COMDLG_FILTERSPEC> fileTypes;
			for (auto&& filter : _filterSpec)
				fileTypes.push_back({ filter.first.data(), filter.second.data() });
			hr = _dialog->SetFileTypes(static_cast<UINT>(fileTypes.size()), fileTypes.data());
		}

		if (SUCCEEDED(hr) && _fileTypeIndex >= 0)
			hr = _dialog->SetFileTypeIndex(_fileTypeIndex + 1); // This index is 1-based

		if (SUCCEEDED(hr))
			return addControls();

		return false;
	}

	bool initSave()
	{
		return init(CLSID_FileSaveDialog);
	}

	bool initOpen()
	{
		return init(CLSID_FileOpenDialog);
	}

	bool addFlags(DWORD dwNewFlags)
	{
		// Before setting, always get the options first in order 
		// not to override existing options.
		DWORD dwOldFlags = 0;
		HRESULT hr = _dialog->GetOptions(&dwOldFlags);
		if (SUCCEEDED(hr))
			hr = _dialog->SetOptions(dwOldFlags | dwNewFlags);
		return SUCCEEDED(hr);
	}

	bool addControls()
	{
		HRESULT hr = _dialog->QueryInterface(IID_PPV_ARGS(&_customize));
		if (SUCCEEDED(hr))
		{
			if (_checkboxLabel && _checkboxLabel[0] != '\0')
			{
				const BOOL isChecked = FALSE;
				hr = _customize->AddCheckButton(IDC_FILE_CHECKBOX, _checkboxLabel, isChecked);
				if (SUCCEEDED(hr) && !_isCheckboxActive)
				{
					hr = _customize->SetControlState(IDC_FILE_CHECKBOX, CDCS_INACTIVE | CDCS_VISIBLE);
				}
			}
		}
		return SUCCEEDED(hr);
	}

	bool setFolder(const TCHAR* dir)
	{
		return setDialogFolder(_dialog, dir);
	}

	bool show()
	{
		bool okPressed = false;
		HRESULT hr = FileDialogEventHandler::createInstance(IID_PPV_ARGS(&_events));
		if (SUCCEEDED(hr))
		{
			DWORD dwCookie;
			hr = _dialog->Advise(_events, &dwCookie);
			if (SUCCEEDED(hr))
			{
				hr = _dialog->Show(_hwndOwner);
				okPressed = SUCCEEDED(hr);

				_dialog->Unadvise(dwCookie);
				_events->Release();
			}
		}
		return okPressed;
	}

	BOOL getCheckboxState() const
	{
		if (_customize)
		{
			BOOL bChecked = FALSE;
			HRESULT hr = _customize->GetCheckButtonState(IDC_FILE_CHECKBOX, &bChecked);
			if (SUCCEEDED(hr))
				return bChecked;
		}
		return FALSE;
	}

	const TCHAR* getResultFilename()
	{
		bool bOk = false;
		IShellItem* psiResult = nullptr;
		HRESULT hr = _dialog->GetResult(&psiResult);
		if (SUCCEEDED(hr))
		{
			PWSTR pszFilePath = NULL;
			hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
			if (SUCCEEDED(hr))
			{
				size_t len = pszFilePath ? wcslen(pszFilePath) : 0;
				if (len > 0 && len <= std::size(_fileName))
				{
					wcsncpy_s(_fileName, pszFilePath, len);
					bOk = true;
				}
				CoTaskMemFree(pszFilePath);
			}
			psiResult->Release();
		}
		return bOk ? _fileName : nullptr;
	}

	static const int IDC_FILE_CHECKBOX = 4;

	HWND _hwndOwner = nullptr;
	const TCHAR* _title = nullptr;
	const TCHAR* _defExt = nullptr;
	const TCHAR* _folder = nullptr;
	const TCHAR* _checkboxLabel = nullptr;
	bool _isCheckboxActive = true;
	std::vector<std::pair<generic_string, generic_string>> _filterSpec;	// text + extension
	int _fileTypeIndex = -1;

private:
	IFileDialog* _dialog = nullptr;
	IFileDialogCustomize* _customize = nullptr;
	IFileDialogEvents* _events = nullptr;
	TCHAR _fileName[MAX_PATH * 8];
};

CustomFileDialog::CustomFileDialog(HWND hwnd) : _impl{std::make_unique<Impl>()}
{
	_impl->_hwndOwner = hwnd;
}

CustomFileDialog::~CustomFileDialog() = default;

void CustomFileDialog::setTitle(const TCHAR* title)
{
	_impl->_title = title;
}

void CustomFileDialog::setExtFilter(const TCHAR *extText, const TCHAR *exts)
{
	// Add an asterisk before each dot in file patterns
	generic_string newExts{ exts };
	for (size_t pos = 0; pos < newExts.size(); ++pos)
	{
		pos = newExts.find(_T('.'), pos);
		if (pos == generic_string::npos)
			break;
		if (pos == 0 || newExts[pos - 1] != _T('*'))
		{
			newExts.insert(pos, 1, _T('*'));
			++pos;
		}
	}

	_impl->_filterSpec.push_back({ extText, newExts });
}

void CustomFileDialog::setDefExt(const TCHAR* ext)
{
	_impl->_defExt = ext;
}

void CustomFileDialog::setFolder(const TCHAR* folder)
{
	_impl->_folder = folder;
}

void CustomFileDialog::setCheckbox(const TCHAR* text, bool isActive)
{
	_impl->_checkboxLabel = text;
	_impl->_isCheckboxActive = isActive;
}

void CustomFileDialog::setExtIndex(int extTypeIndex)
{
	_impl->_fileTypeIndex = extTypeIndex;
}

bool CustomFileDialog::getCheckboxState() const
{
	return _impl->getCheckboxState();
}

const TCHAR* CustomFileDialog::doSaveDlg()
{
	if (!_impl->initSave())
		return nullptr;

	TCHAR dir[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, dir);

	NppParameters& params = NppParameters::getInstance();
	_impl->setFolder(params.getWorkingDir());

	_impl->addFlags(FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM);
	bool bOk = _impl->show();

	if (params.getNppGUI()._openSaveDir == dir_last)
	{
		::GetCurrentDirectory(MAX_PATH, dir);
		params.setWorkingDir(dir);
	}
	::SetCurrentDirectory(dir);

	return bOk ? _impl->getResultFilename() : nullptr;
}

const TCHAR* CustomFileDialog::pickFolder()
{
	if (!_impl->initOpen())
		return nullptr;

	_impl->addFlags(FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);
	bool bOk = _impl->show();
	return bOk ? _impl->getResultFilename() : nullptr;
}
