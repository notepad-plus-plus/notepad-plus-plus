// This file is part of Notepad++ project
// Copyright (C) 2021 The Notepad++ Contributors.

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


#include <shobjidl.h>
#include <shlwapi.h>	// PathIsDirectory
#ifdef __MINGW32__
#include <cwchar>
#endif
#include <comdef.h>		// _com_error
#include <comip.h>		// _com_ptr_t
#include <unordered_map>
#include "CustomFileDialog.h"
#include "Parameters.h"

// Workaround for MinGW because its implementation of __uuidof is different.
template<class T>
struct ComTraits
{
	static const GUID uid;
};
template<class T>
const GUID ComTraits<T>::uid = __uuidof(T);

// Smart pointer alias for COM objects that makes reference counting easier.
template<class T, class InterfaceT = T>
using com_ptr = _com_ptr_t<_com_IIID<T, &ComTraits<InterfaceT>::uid>>;

namespace // anonymous
{
	// Note: these common functions could be moved to some header.

	struct Filter
	{
		generic_string name;
		generic_string ext;
	};

	static const int IDC_FILE_CUSTOM_CHECKBOX = 4;
	static const int IDC_FILE_TYPE_CHECKBOX = IDC_FILE_CUSTOM_CHECKBOX + 1;

	// Returns a first extension from the extension specification string.
	// Multiple extensions are separated with ';'.
	// Example: input - ".c;.cpp;.h", output - ".c"
	generic_string get1stExt(const generic_string& extSpec)
	{
		size_t pos = extSpec.find('.');
		if (pos != generic_string::npos)
		{
			size_t posEnd = extSpec.find(';', pos + 1);
			if (posEnd != generic_string::npos)
			{
				size_t extLen = posEnd - pos;
				return extSpec.substr(pos, extLen);
			}
			return extSpec.substr(pos);
		}
		return {};
	}

	bool replaceExt(generic_string& name, const generic_string& ext)
	{
		if (!name.empty() && !ext.empty())
		{
			// Remove an existing extension from the name.
			size_t posNameExt = name.find_last_of('.');
			if (posNameExt != generic_string::npos)
				name.erase(posNameExt);
			// Append a new extension.
			name += ext;
			return true;
		}
		return false;
	}

	bool hasExt(const generic_string& name)
	{
		return name.find_last_of('.') != generic_string::npos;
	}

	void expandEnv(generic_string& s)
	{
		TCHAR buffer[MAX_PATH] = { '\0' };
		// This returns the resulting string length or 0 in case of error.
		DWORD ret = ExpandEnvironmentStrings(s.c_str(), buffer, static_cast<DWORD>(std::size(buffer)));
		if (ret != 0)
		{
			if (ret == static_cast<DWORD>(lstrlen(buffer) + 1))
			{
				s = buffer;
			}
			else
			{
				// Buffer was too small, try with a bigger buffer of the required size.
				std::vector<TCHAR> buffer2(ret, 0);
				ret = ExpandEnvironmentStrings(s.c_str(), buffer2.data(), static_cast<DWORD>(buffer2.size()));
				assert(ret == static_cast<DWORD>(lstrlen(buffer2.data()) + 1));
				s = buffer2.data();
			}
		}
	}

	generic_string getFilename(IShellItem* psi)
	{
		generic_string result;
		if (psi)
		{
			PWSTR pszFilePath = nullptr;
			HRESULT hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
			if (SUCCEEDED(hr) && pszFilePath)
			{
				result = pszFilePath;
				CoTaskMemFree(pszFilePath);
			}
		}
		return result;
	}

	bool setDialogFolder(IFileDialog* dialog, const TCHAR* path)
	{
		com_ptr<IShellItem> shellItem;
		HRESULT hr = SHCreateItemFromParsingName(path,
			nullptr,
			IID_PPV_ARGS(&shellItem));
		if (SUCCEEDED(hr) && shellItem && !::PathIsDirectory(path))
		{
			com_ptr<IShellItem> parentItem;
			hr = shellItem->GetParent(&parentItem);
			if (SUCCEEDED(hr))
				shellItem = parentItem;
		}
		if (SUCCEEDED(hr))
			hr = dialog->SetFolder(shellItem);
		return SUCCEEDED(hr);
	}

	generic_string getDialogFileName(IFileDialog* dialog)
	{
		generic_string fileName;
		if (dialog)
		{
			PWSTR pszFilePath = nullptr;
			HRESULT hr = dialog->GetFileName(&pszFilePath);
			if (SUCCEEDED(hr) && pszFilePath)
			{
				fileName = pszFilePath;
				CoTaskMemFree(pszFilePath);
			}
		}
		return fileName;
	}

	generic_string getDialogFolder(IFileDialog* dialog)
	{
		com_ptr<IShellItem> psi;
		HRESULT hr = dialog->GetFolder(&psi);
		if (SUCCEEDED(hr))
			return getFilename(psi);
		return {};
	}

	HWND getDialogHandle(IFileDialog* dialog)
	{
		com_ptr<IOleWindow> pOleWnd = dialog;
		if (pOleWnd)
		{
			HWND hwnd = nullptr;
			if (SUCCEEDED(pOleWnd->GetWindow(&hwnd)))
				return hwnd;
		}
		return nullptr;
	}

	// Backs up the current directory in constructor and restores it in destructor.
	// This is needed in case dialog changes the current directory.
	class CurrentDirBackup
	{
	public:
		CurrentDirBackup()
		{
			::GetCurrentDirectory(MAX_PATH, _dir);
		}
		~CurrentDirBackup()
		{
			::SetCurrentDirectory(_dir);
		}
	private:
		TCHAR _dir[MAX_PATH];
	};

} // anonymous namespace

///////////////////////////////////////////////////////////////////////////////

class FileDialogEventHandler : public IFileDialogEvents, public IFileDialogControlEvents
{
public:
	// IUnknown methods

	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override
	{
		// Always set out parameter to NULL, validating it first.
		if (!ppv)
			return E_INVALIDARG;
		*ppv = nullptr;
		if (riid == __uuidof(IUnknown) || riid == __uuidof(IFileDialogEvents))
		{
			// Increment the reference count and return the pointer.
			*ppv = static_cast<IFileDialogEvents*>(this);
			AddRef();
			return NOERROR;
		}
		else if (riid == __uuidof(IFileDialogControlEvents))
		{
			// Increment the reference count and return the pointer.
			*ppv = static_cast<IFileDialogControlEvents*>(this);
			AddRef();
			return NOERROR;
		}
		return E_NOINTERFACE;
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

	IFACEMETHODIMP OnFileOk(IFileDialog* dlg) override
	{
		_lastUsedFolder = getDialogFolder(dlg);
		return S_OK;
	}
	IFACEMETHODIMP OnFolderChange(IFileDialog*) override
	{
		// Dialog startup calling order: 3. Custom controls are added but inactive.
		if (!foundControls())
			findControls();
		return S_OK;
	}
	IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem* psi) override
	{
		// Called when the current dialog folder is about to change.
		// Dialog startup calling order: 2. Buttons are added, correct window title.
		_lastUsedFolder = getFilename(psi);
		return S_OK;
	}
	IFACEMETHODIMP OnSelectionChange(IFileDialog*) override
	{
		// This event may not be triggered in an empty folder (Windows 10+).
		// Dialog startup calling order: 4. Main window is shown.
		if (!foundControls())
			findControls();
		return S_OK;
	}
	IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) override
	{
		return S_OK;
	}
	IFACEMETHODIMP OnTypeChange(IFileDialog*) override
	{
		// Dialog startup calling order: 1. Inactive, window title might be wrong.
		UINT dialogIndex = 0;
		if (SUCCEEDED(_dialog->GetFileTypeIndex(&dialogIndex)))
		{
			// Enable checkbox if type was changed.
			if (OnTypeChange(dialogIndex))
				_customize->SetCheckButtonState(IDC_FILE_TYPE_CHECKBOX, TRUE);
		}
		return S_OK;
	}

	bool OnTypeChange(UINT dialogIndex)
	{
		if (dialogIndex == 0)
			return false;
		// Remember the current file type index.
		// Since GetFileTypeIndex() might return the old value in some cases.
		// Specifically, when called after SetFileTypeIndex().
		_currentType = dialogIndex;
		generic_string name = getDialogFileName(_dialog);
		if (changeExt(name, dialogIndex - 1))
		{
			// Set the file name and clear the selection in the edit box.
			// The selection is implicitly modified by SetFileName().
			DWORD selStart = 0;
			DWORD selEnd = 0;
			bool ok = SUCCEEDED(_dialog->SetFileName(name.c_str()));
			if (ok)
				SendMessage(_hwndNameEdit, EM_SETSEL, selStart, selEnd);
			return ok;
		}
		return false;
	}

	IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) override
	{
		// Called when a file with a given name already exists.
		// Focus the file name edit box to make it easier to pick a new name.
		if (_hwndNameEdit)
			SetFocus(_hwndNameEdit);
		return S_OK;
	}

	// IFileDialogControlEvents methods

	IFACEMETHODIMP OnItemSelected(IFileDialogCustomize*, DWORD, DWORD) override
	{
		return E_NOTIMPL;
	}

	IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize*, DWORD) override
	{
		return E_NOTIMPL;
	}

	IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize*, DWORD id, BOOL bChecked) override
	{
		if (id == IDC_FILE_TYPE_CHECKBOX)
		{
			UINT newFileType = 0;
			if (bChecked)
			{
				newFileType = _lastSelectedType;
			}
			else
			{
				if (_currentType != 0 && _currentType != _wildcardType)
					_lastSelectedType = _currentType;
				newFileType = _wildcardType;
			}
			_dialog->SetFileTypeIndex(newFileType);
			OnTypeChange(newFileType);
			return S_OK;
		}
		return E_NOTIMPL;
	}

	IFACEMETHODIMP OnControlActivating(IFileDialogCustomize*, DWORD) override
	{
		return E_NOTIMPL;
	}

	FileDialogEventHandler(IFileDialog* dlg, const std::vector<Filter>& filterSpec, int fileIndex, int wildcardIndex)
		: _cRef(1), _dialog(dlg), _customize(dlg), _filterSpec(filterSpec), _currentType(fileIndex + 1),
		_lastSelectedType(fileIndex + 1), _wildcardType(wildcardIndex >= 0 ? wildcardIndex + 1 : 0)
	{
		installHooks();
	}

	~FileDialogEventHandler()
	{
		eraseHandles();
		removeHooks();
	}

	const generic_string& getLastUsedFolder() const { return _lastUsedFolder; }

private:
	FileDialogEventHandler(const FileDialogEventHandler&) = delete;
	FileDialogEventHandler& operator=(const FileDialogEventHandler&) = delete;
	FileDialogEventHandler(FileDialogEventHandler&&) = delete;
	FileDialogEventHandler& operator=(FileDialogEventHandler&&) = delete;

	// Find window handles for file name edit and ok button.
	// Call this as late as possible to ensure all the controls of the dialog are created.
	bool findControls()
	{
		assert(_dialog);
		HWND hwndDlg = getDialogHandle(_dialog);
		if (hwndDlg)
		{
			EnumChildWindows(hwndDlg, &EnumChildProc, reinterpret_cast<LPARAM>(this));
			if (_hwndButton)
				s_handleMap[_hwndButton] = this;
			if (_hwndNameEdit)
				s_handleMap[_hwndNameEdit] = this;
		}
		return foundControls();
	}

	bool foundControls() const
	{
		return _hwndButton && _hwndNameEdit;
	}

	void installHooks()
	{
		_prevKbdHook = ::SetWindowsHookEx(WH_KEYBOARD,
			reinterpret_cast<HOOKPROC>(&FileDialogEventHandler::KbdProcHook),
			nullptr,
			::GetCurrentThreadId()
		);
		_prevCallHook = ::SetWindowsHookEx(WH_CALLWNDPROC,
			reinterpret_cast<HOOKPROC>(&FileDialogEventHandler::CallProcHook),
			nullptr,
			::GetCurrentThreadId()
		);
	}

	void removeHooks()
	{
		if (_prevKbdHook)
			::UnhookWindowsHookEx(_prevKbdHook);
		if (_prevCallHook)
			::UnhookWindowsHookEx(_prevCallHook);
		_prevKbdHook = nullptr;
		_prevCallHook = nullptr;
	}

	void eraseHandles()
	{
		if (_hwndButton && _hwndNameEdit)
		{
			s_handleMap.erase(_hwndButton);
			s_handleMap.erase(_hwndNameEdit);
		}
		else
		{
			std::vector<HWND> handlesToErase;
			for (auto&& x : s_handleMap)
			{
				if (x.second == this)
					handlesToErase.push_back(x.first);
			}
			for (auto&& h : handlesToErase)
			{
				s_handleMap.erase(h);
			}
		}
	}

	bool changeExt(generic_string& name, int extIndex)
	{
		if (extIndex >= 0 && extIndex < static_cast<int>(_filterSpec.size()))
		{
			const generic_string ext = get1stExt(_filterSpec[extIndex].ext);
			if (!endsWith(ext, _T(".*")))
				return replaceExt(name, ext);
		}
		return false;
	}

	generic_string getAbsPath(const generic_string& fileName)
	{
		if (::PathIsRelative(fileName.c_str()))
		{
			TCHAR buffer[MAX_PATH] = { '\0' };
			const generic_string folder = getDialogFolder(_dialog);
			LPTSTR ret = ::PathCombine(buffer, folder.c_str(), fileName.c_str());
			if (ret)
				return buffer;
		}
		return fileName;
	}

	// Modifies the file name if necesary after user confirmed input.
	// Called after the user input but before OnFileOk() and before any name validation.
	void onPreFileOk()
	{
		if (!_dialog)
			return;
		// Get the entered name.
		generic_string fileName = getDialogFileName(_dialog);
		expandEnv(fileName);
		bool nameChanged = transformPath(fileName);
		// Update the controls.
		if (!::PathIsDirectory(getAbsPath(fileName).c_str()))
		{
			// Name is a file path.
			// Add file extension if missing.
			if (!hasExt(fileName))
				nameChanged |= changeExt(fileName, _currentType - 1);
		}
		// Update the edit box text.
		// It will update the address if the path is a directory.
		if (nameChanged)
		{
			// Clear the name first to ensure it's updated properly.
			_dialog->SetFileName(_T(""));
			_dialog->SetFileName(fileName.c_str());
		}
	}

	// Transforms a forward-slash path to a canonical Windows path.
	static bool transformPath(generic_string& fileName)
	{
		if (fileName.empty())
			return false;
		bool transformed = false;
		// Replace a forward-slash with a backslash.
		std::replace_if(fileName.begin(), fileName.end(),
			[&transformed](generic_string::value_type c)
			{
				const bool eq = (c == '/');
				transformed |= eq;
				return eq;
			},
			'\\');
		return transformed;
	}

	// Enumerates the child windows of a dialog.
	// Remember handles of "OK" button and file name edit box.
	static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM param)
	{
		const int bufferLen = MAX_PATH;
		static TCHAR buffer[bufferLen];
		static bool isRTL = false;

		auto* inst = reinterpret_cast<FileDialogEventHandler*>(param);
		if (!inst)
			return FALSE;

		if (IsWindowEnabled(hwnd) && GetClassName(hwnd, buffer, bufferLen) != 0)
		{
			if (lstrcmpi(buffer, _T("ComboBox")) == 0)
			{
				// The edit box of interest is a child of the combo box and has empty window text.
				// We use the first combo box, but there might be the others (file type dropdown, address bar, etc).
				HWND hwndChild = FindWindowEx(hwnd, nullptr, _T("Edit"), _T(""));
				if (hwndChild && !inst->_hwndNameEdit)
				{
					inst->_hwndNameEdit = hwndChild;
				}
			}
			else if (lstrcmpi(buffer, _T("Button")) == 0)
			{
				// Find the OK button.
				// Preconditions:
				// Label could be "Open" or "Save".
				// Label could be localized (that's why can't search by window text).
				// Dialog could have other buttons ("Cancel", "Help", etc).
				// The order of the EnumChildWindows() traversal may change.
				// Solutions:
				// 1. Find the leftmost (or the rightmost if RTL) button. The current solution.
				// 2. Find by text. Get the localized text from windows resource DLL.
				//    Problem: Resource ID might be changed or relocated to a different DLL.
				// 3. Find by text. Set the localized text for the OK button beforehand (IFileDialog::SetOkButtonLabel). 
				//    Problem: Localization might be not installed; need to use the OS language not the app language.
				// 4. Just get the first button found. Save/Open button has control ID value lower than Cancel button. 
				//    Problem: It may work or may not, depending on the initialization order or other environment factors.
				LONG style = GetWindowLong(hwnd, GWL_STYLE);
				if (style & (WS_CHILDWINDOW | WS_GROUP))
				{
					DWORD type = style & 0xF;
					DWORD appearance = style & 0xF0;
					if ((type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) && (appearance == BS_TEXT))
					{
						// Get the leftmost button.
						if (inst->_hwndButton)
						{
							RECT rc1 = {};
							RECT rc2 = {};
							if (GetWindowRect(hwnd, &rc1) && GetWindowRect(inst->_hwndButton, &rc2))
							{
								const bool isLess = isRTL ? (rc1.right > rc2.right) : (rc1.left < rc2.left);
								if (isLess)
									inst->_hwndButton = hwnd;
							}
						}
						else
						{
							isRTL = GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL;
							inst->_hwndButton = hwnd;
						}
					}
				}
			}
		}
		return TRUE;
	}

	static LRESULT CALLBACK CallProcHook(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION)
		{
			auto* msg = reinterpret_cast<CWPSTRUCT*>(lParam);
			if (msg && msg->message == WM_COMMAND && HIWORD(msg->wParam) == BN_CLICKED)
			{
				// Handle Save/Open button click.
				// Different ways to press a button:
				// 1. space/enter is pressed when the button has focus (WM_KEYDOWN)
				// 2. left mouse click on a button (WM_LBUTTONDOWN)
				// 3. Alt + S
				auto ctrlId = LOWORD(msg->wParam);
				HWND hwnd = GetDlgItem(msg->hwnd, ctrlId);
				auto it = s_handleMap.find(hwnd);
				if (it != s_handleMap.end() && it->second && hwnd == it->second->_hwndButton)
					it->second->onPreFileOk();
			}
		}
		return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
	}

	static LRESULT CALLBACK KbdProcHook(int nCode, WPARAM wParam, LPARAM lParam)
	{
		if (nCode == HC_ACTION)
		{
			if (wParam == VK_RETURN)
			{
				// Handle return key passed to the file name edit box.
				HWND hwnd = GetFocus();
				auto it = s_handleMap.find(hwnd);
				if (it != s_handleMap.end() && it->second && hwnd == it->second->_hwndNameEdit)
					it->second->onPreFileOk();
			}
		}
		return ::CallNextHookEx(nullptr, nCode, wParam, lParam);
	}

	static std::unordered_map<HWND, FileDialogEventHandler*> s_handleMap;

	long _cRef;
	com_ptr<IFileDialog> _dialog;
	com_ptr<IFileDialogCustomize> _customize;
	const std::vector<Filter> _filterSpec;
	generic_string _lastUsedFolder;
	HHOOK _prevKbdHook = nullptr;
	HHOOK _prevCallHook = nullptr;
	HWND _hwndNameEdit = nullptr;
	HWND _hwndButton = nullptr;
	UINT _currentType = 0;  // File type currenly selected in dialog.
	UINT _lastSelectedType = 0;  // Last selected non-wildcard file type.
	UINT _wildcardType = 0;  // Wildcard *.* file type index (usually 1).
};
std::unordered_map<HWND, FileDialogEventHandler*> FileDialogEventHandler::s_handleMap;

///////////////////////////////////////////////////////////////////////////////

// Private implementation to avoid pollution with includes and defines in header.
class CustomFileDialog::Impl
{
public:
	Impl() = default;

	~Impl() = default;

	bool init(CLSID id)
	{
		if (_dialog)
			return false; // Avoid double initialization

		// Sanitize data.
		if (_fileTypeIndex >= static_cast<int>(_filterSpec.size()))
			_fileTypeIndex = 0;

		HRESULT hr = CoCreateInstance(id,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&_dialog));
		_customize = _dialog;

		// Init the event handler.
		// Pass the initially selected file type.
		if (SUCCEEDED(hr))
			_events.Attach(new FileDialogEventHandler(_dialog, _filterSpec, _fileTypeIndex, _wildcardIndex));

		// If "assign type" is OFF, then change the file type to *.*
		if (_enableFileTypeCheckbox && !_fileTypeCheckboxValue && _wildcardIndex >= 0)
			_fileTypeIndex = _wildcardIndex;

		if (SUCCEEDED(hr) && _title)
			hr = _dialog->SetTitle(_title);

		if (SUCCEEDED(hr))
		{
			// Do not fail the initialization if failed to set a folder.
			bool isFolderSet = false;
			if (!_initialFolder.empty())
				isFolderSet = setDialogFolder(_dialog, _initialFolder.c_str());

			if (!isFolderSet && !_fallbackFolder.empty())
				isFolderSet = setDialogFolder(_dialog, _fallbackFolder.c_str());
		}

		if (SUCCEEDED(hr) && _defExt && _defExt[0] != '\0')
			hr = _dialog->SetDefaultExtension(_defExt);

		if (SUCCEEDED(hr) && _initialFileName)
		{
			generic_string newFileName = _initialFileName;
			if (_fileTypeIndex >= 0 && _fileTypeIndex < static_cast<int>(_filterSpec.size()))
			{
				if (!hasExt(newFileName))
				{
					const generic_string ext = get1stExt(_filterSpec[_fileTypeIndex].ext);
					if (!endsWith(ext, _T(".*")))
						newFileName += ext;
				}
			}
			hr = _dialog->SetFileName(newFileName.c_str());
		}

		if (SUCCEEDED(hr) && !_filterSpec.empty())
		{
			std::vector<COMDLG_FILTERSPEC> fileTypes;
			fileTypes.reserve(_filterSpec.size());
			for (auto&& filter : _filterSpec)
				fileTypes.push_back({ filter.name.data(), filter.ext.data() });
			hr = _dialog->SetFileTypes(static_cast<UINT>(fileTypes.size()), fileTypes.data());
		}

		// The selected index should be set after the file types are set.
		if (SUCCEEDED(hr) && _fileTypeIndex >= 0)
			hr = _dialog->SetFileTypeIndex(_fileTypeIndex + 1); // This index is 1-based.

		if (_enableFileTypeCheckbox)
			addCheckbox(IDC_FILE_TYPE_CHECKBOX, _fileTypeCheckboxLabel.c_str(), _fileTypeCheckboxValue);

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
		if (!_customize)
			return false;
		if (_checkboxLabel && _checkboxLabel[0] != '\0')
		{
			return addCheckbox(IDC_FILE_CUSTOM_CHECKBOX, _checkboxLabel, false, _isCheckboxActive);
		}
		return true;
	}

	bool addCheckbox(int id, const TCHAR* label, bool value, bool enabled = true)
	{
		if (!_customize)
			return false;
		HRESULT hr = _customize->AddCheckButton(id, label, value ? TRUE : FALSE);
		if (SUCCEEDED(hr) && !enabled)
		{
			hr = _customize->SetControlState(id, CDCS_INACTIVE | CDCS_VISIBLE);
			return SUCCEEDED(hr);
		}
		return SUCCEEDED(hr);
	}

	bool show()
	{
		// Allow only one instance of the dialog to be active at a time.
		static bool isActive = false;
		if (isActive)
			return false;

		assert(_dialog);
		if (!_dialog)
			return false;

		isActive = true;
		HRESULT hr = S_OK;
		DWORD dwCookie = 0;
		com_ptr<IFileDialogEvents> dialogEvents = _events;
		if (dialogEvents)
		{
			hr = _dialog->Advise(dialogEvents, &dwCookie);
			if (FAILED(hr))
				dialogEvents.Release();
		}

		bool okPressed = false;
		if (SUCCEEDED(hr))
		{
			hr = _dialog->Show(_hwndOwner);
			okPressed = SUCCEEDED(hr);

			NppParameters& params = NppParameters::getInstance();
			if (okPressed && params.getNppGUI()._openSaveDir == dir_last)
			{
				// Note: IFileDialog doesn't modify the current directory.
				// At least, after it is hidden, the current directory is the same as before it was shown.
				params.setWorkingDir(_events->getLastUsedFolder().c_str());
			}
		}

		if (dialogEvents)
			_dialog->Unadvise(dwCookie);

		isActive = false;

		return okPressed;
	}

	BOOL getCheckboxState(int id) const
	{
		if (_customize)
		{
			BOOL bChecked = FALSE;
			HRESULT hr = _customize->GetCheckButtonState(id, &bChecked);
			if (SUCCEEDED(hr))
				return bChecked;
		}
		return FALSE;
	}

	generic_string getResultFilename()
	{
		generic_string fileName;
		com_ptr<IShellItem> psiResult;
		HRESULT hr = _dialog->GetResult(&psiResult);
		if (SUCCEEDED(hr))
		{
			fileName = getFilename(psiResult);
			_hasReadonly = hasReadonlyAttr(psiResult);
		}
		return fileName;
	}

	static bool hasReadonlyAttr(IShellItem* psi)
	{
		SFGAOF attrs = 0;
		HRESULT hr = psi->GetAttributes(SFGAO_READONLY, &attrs);
		if (SUCCEEDED(hr))
			return attrs & SFGAO_READONLY;
		return false;
	}

	std::vector<generic_string> getFilenames()
	{
		std::vector<generic_string> result;
		// Only the open dialog can have multiple results.
		com_ptr<IFileOpenDialog> pfd = _dialog;
		if (pfd)
		{
			com_ptr<IShellItemArray> psiaResults;
			HRESULT hr = pfd->GetResults(&psiaResults);
			if (SUCCEEDED(hr))
			{
				DWORD count = 0;
				hr = psiaResults->GetCount(&count);
				if (SUCCEEDED(hr))
				{
					for (DWORD i = 0; i != count; ++i)
					{
						com_ptr<IShellItem> psi;
						hr = psiaResults->GetItemAt(i, &psi);
						if (SUCCEEDED(hr))
						{
							_hasReadonly |= hasReadonlyAttr(psi);
							result.push_back(getFilename(psi));
						}
					}
				}
			}
		}
		return result;
	}

	HWND _hwndOwner = nullptr;
	const TCHAR* _title = nullptr;
	const TCHAR* _defExt = nullptr;
	generic_string _initialFolder;
	generic_string _fallbackFolder;
	const TCHAR* _checkboxLabel = nullptr;
	const TCHAR* _initialFileName = nullptr;
	bool _isCheckboxActive = true;
	std::vector<Filter> _filterSpec;
	int _fileTypeIndex = -1;	// preferred file type index
	int _wildcardIndex = -1;	// *.* file type index
	bool _hasReadonly = false;	// set during the result handling
	bool _enableFileTypeCheckbox = false;
	bool _fileTypeCheckboxValue = false;	// initial value
	generic_string _fileTypeCheckboxLabel;

private:
	com_ptr<IFileDialog> _dialog;
	com_ptr<IFileDialogCustomize> _customize;
	com_ptr<FileDialogEventHandler, IFileDialogEvents> _events;
};

///////////////////////////////////////////////////////////////////////////////

CustomFileDialog::CustomFileDialog(HWND hwnd) : _impl{ std::make_unique<Impl>() }
{
	_impl->_hwndOwner = hwnd;

	NppParameters& params = NppParameters::getInstance();
	const TCHAR* workDir = params.getWorkingDir();
	if (workDir)
		_impl->_fallbackFolder = workDir;
}

CustomFileDialog::~CustomFileDialog() = default;

void CustomFileDialog::setTitle(const TCHAR* title)
{
	_impl->_title = title;
}

void CustomFileDialog::setExtFilter(const TCHAR *extText, const TCHAR *exts)
{
	// Add an asterisk before each dot in file patterns
	generic_string newExts{ exts ? exts : _T("") };
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

	if (newExts.find(_T("*.*")) == 0)
		_impl->_wildcardIndex = static_cast<int>(_impl->_filterSpec.size());

	_impl->_filterSpec.push_back({ extText, newExts });
}

void CustomFileDialog::setExtFilter(const TCHAR *extText, std::initializer_list<const TCHAR*> extList)
{
	generic_string exts;
	for (auto&& x : extList)
	{
		exts += x;
		exts += _T(';');
	}
	exts.pop_back();	// remove the last ';'
	setExtFilter(extText, exts.c_str());
}

void CustomFileDialog::setDefExt(const TCHAR* ext)
{
	_impl->_defExt = ext;
}

void CustomFileDialog::setDefFileName(const TCHAR* fn)
{
	_impl->_initialFileName = fn;
}

void CustomFileDialog::setFolder(const TCHAR* folder)
{
	_impl->_initialFolder = folder ? folder : _T("");
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
	return _impl->getCheckboxState(IDC_FILE_CUSTOM_CHECKBOX);
}

bool CustomFileDialog::isReadOnly() const
{
	return _impl->_hasReadonly;
}

void CustomFileDialog::enableFileTypeCheckbox(const generic_string& text, bool value)
{
	assert(!text.empty());
	if (!text.empty())
	{
		_impl->_fileTypeCheckboxLabel = text;
		_impl->_enableFileTypeCheckbox = true;
		_impl->_fileTypeCheckboxValue = value;
	}
}

bool CustomFileDialog::getFileTypeCheckboxValue() const
{
	return _impl->getCheckboxState(IDC_FILE_TYPE_CHECKBOX);
}

generic_string CustomFileDialog::doSaveDlg()
{
	if (!_impl->initSave())
		return {};

	CurrentDirBackup backup;

	_impl->addFlags(FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM);
	bool bOk = _impl->show();
	return bOk ? _impl->getResultFilename() : _T("");
}

generic_string CustomFileDialog::doOpenSingleFileDlg()
{
	if (!_impl->initOpen())
		return {};

	CurrentDirBackup backup;

	_impl->addFlags(FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM);
	bool bOk = _impl->show();
	return bOk ? _impl->getResultFilename() : _T("");
}

std::vector<generic_string> CustomFileDialog::doOpenMultiFilesDlg()
{
	if (!_impl->initOpen())
		return {};

	CurrentDirBackup backup;

	_impl->addFlags(FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT);
	bool bOk = _impl->show();
	if (bOk)
		return _impl->getFilenames();
	return {};
}

generic_string CustomFileDialog::pickFolder()
{
	if (!_impl->initOpen())
		return {};

	_impl->addFlags(FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_PICKFOLDERS);
	bool bOk = _impl->show();
	return bOk ? _impl->getResultFilename() : _T("");
}
