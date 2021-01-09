// This file is part of Notepad++ project
// Copyright (C) 2021 The Notepad++ Contributors.
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


// Windows Vista is a minimum required version for Common File Dialogs.
// Define it only for current source file.
#if defined(_WIN32_WINNT) && (!defined(_WIN32_WINNT_VISTA) || (_WIN32_WINNT < _WIN32_WINNT_VISTA))
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#endif

#include <shobjidl.h>

#include "CustomFileDialog.h"
#include "Parameters.h"

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
			hr = setInitDir(_folder) ? S_OK : E_FAIL;

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

	bool setInitDir(const TCHAR* dir)
	{
		IShellItem* psi = nullptr;
		HRESULT hr = SHCreateItemFromParsingName(dir,
			0,
			IID_IShellItem,
			reinterpret_cast<void**>(&psi));
		if (SUCCEEDED(hr))
			hr = _dialog->SetFolder(psi);
		return SUCCEEDED(hr);
	}

	bool show()
	{
		HRESULT hr = _dialog->Show(_hwndOwner);
		return SUCCEEDED(hr);
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
	_impl->setInitDir(params.getWorkingDir());

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
