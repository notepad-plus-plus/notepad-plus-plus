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

#include <deque>
#include <algorithm>
#include <time.h>
#include <locale>
#include <sys/stat.h>
#include "Buffer.h"
#include "Scintilla.h"
#include "ILexer.h"
#include "Lexilla.h"
#include "Parameters.h"
#include "Notepad_plus.h"
#include "ScintillaEditView.h"
#include "EncodingMapper.h"
#include "uchardet.h"
#include "FileInterface.h"


static const int blockSize = 128 * 1024 + 4;
static const int CR = 0x0D;
static const int LF = 0x0A;

long Buffer::_recentTagCtr = 0;


namespace // anonymous
{
	static EolType getEOLFormatForm(const char* const data, size_t length, EolType defvalue = EolType::osdefault)
	{
		assert(length == 0 || (data != nullptr && "invalid buffer for getEOLFormatForm()"));

		for (size_t i = 0; i != length; ++i)
		{
			if (data[i] == CR)
			{
				if (i + 1 < length && data[i + 1] == LF)
					return EolType::windows;

				return EolType::macos;
			}

			if (data[i] == LF)
				return EolType::unix;
		}

		return defvalue; // fallback unknown
	}

} // anonymous namespace

using namespace std;

Buffer::Buffer(FileManager * pManager, BufferID id, Document doc, DocFileStatus type, const wchar_t *fileName, bool isLargeFile)
	// type must be either DOC_REGULAR or DOC_UNNAMED
	: _pManager(pManager) , _id(id), _doc(doc), _lang(L_TEXT), _isLargeFile(isLargeFile)
{
	NppParameters& nppParamInst = NppParameters::getInstance();
	const NewDocDefaultSettings& ndds = (nppParamInst.getNppGUI()).getNewDocDefaultSettings();

	_eolFormat = ndds._format;
	_unicodeMode = ndds._unicodeMode;
	_encoding = ndds._codepage;
	if (_encoding != -1)
		_unicodeMode = uniCookie;

	_currentStatus = type;

	setFileName(fileName);
	updateTimeStamp();
	checkFileState();

	// reset after initialization
	_canNotify = true;

	if (nppParamInst.getNativeLangSpeaker()->isRTL() && nppParamInst.getNativeLangSpeaker()->isEditZoneRTL())
		_isRTL = true;
}


void Buffer::doNotify(int mask)
{
	if (_canNotify)
	{
		assert(_pManager != nullptr);
		_pManager->beNotifiedOfBufferChange(this, mask);
	}
}


void Buffer::setDirty(bool dirty)
{
	_isDirty = dirty;
	doNotify(BufferChangeDirty);
}


void Buffer::setEncoding(int encoding)
{
	_encoding = encoding;
	doNotify(BufferChangeUnicode | BufferChangeDirty);
}


void Buffer::setUnicodeMode(UniMode mode)
{
	_unicodeMode = mode;
	doNotify(BufferChangeUnicode | BufferChangeDirty);
}


void Buffer::setLangType(LangType lang, const wchar_t* userLangName)
{
	if (lang == _lang && lang != L_USER)
		return;

	_lang = lang;
	if (_lang == L_USER)
		_userLangExt = userLangName;
	else if (_lang == L_ASCII)
		_encoding = NPP_CP_DOS_437;

	_needLexer = true;	//change of lang means lexern needs updating
	doNotify(BufferChangeLanguage | BufferChangeLexing);
}


void Buffer::updateTimeStamp()
{
	FILETIME timeStampLive {};

	WIN32_FILE_ATTRIBUTE_DATA attributes{};
	attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	bool bWorkerThreadTerminated = true;
	DWORD dwWin32ApiError = NO_ERROR;
	BOOL bGetFileAttributesExSucceeded = getFileAttributesExWithTimeout(_fullPathName.c_str(), &attributes, 0, &bWorkerThreadTerminated, &dwWin32ApiError);
	if (bGetFileAttributesExSucceeded && (attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES) && !(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		timeStampLive = attributes.ftLastWriteTime;
	}
	else
	{
		if (_currentStatus != DOC_UNNAMED)
		{
			NppParameters& nppParam = NppParameters::getInstance();
			if (nppParam.doNppLogNetworkDriveIssue())
			{
				wstring issueFn = nppLogNetworkDriveIssue;
				issueFn += L".log";
				wstring nppIssueLog = nppParam.getUserPath();
				pathAppend(nppIssueLog, issueFn);
				std::string msg = wstring2string(_fullPathName, CP_UTF8);
				msg += "  in Buffer::updateTimeStamp(), getFileAttributesExWithTimeout returned ";
				if (bGetFileAttributesExSucceeded)
					msg += "TRUE";
				else
					msg += "FALSE";

				if (bWorkerThreadTerminated)
				{
					msg += ", its worker thread had to be forcefully terminated due to timeout reached!";
				}
				else
				{
					msg += ", its worker thread finished successfully within the timeout given, ";
					if (attributes.dwFileAttributes == INVALID_FILE_ATTRIBUTES)
						msg += "dwFileAttributes == INVALID_FILE_ATTRIBUTES ! (WIN32API Error Code: " + std::to_string(dwWin32ApiError) + ")";
					else
						msg += "dwFileAttributes has the FILE_ATTRIBUTE_DIRECTORY flag set!";
				}
				writeLog(nppIssueLog.c_str(), msg.c_str());
			}
		}

		// if getting timestamp operation fails, no timestamp to compare then no need to continue
		return;
	}

	LONG res = CompareFileTime(&_timeStamp, &timeStampLive);

	if (res == -1 || res == 1)
	// (res == -1) => timeStampLive is later, it means the file has been modified outside of Notepad++ - usual case
	// 
	// (res == 1) => timeStampLive (get directly from the file on disk) is earlier than buffer's timestamp - unusual case
	//               It can happen when user copies a backup of editing file somewhere-else firstly, then modifies the editing file in Notepad++ and saves it.
	//               Now user copies the backup back to erase the modified editing file outside Notepad++ (via Explorer).
	{
		NppParameters& nppParam = NppParameters::getInstance();
		if (nppParam.doNppLogNetworkDriveIssue())
		{
			char buf[1024]{};
			if (res == 1)
			{
				sprintf_s(buf, _countof(buf) - 1, "  in updateTimeStamp(): timeStampLive (%lu/%lu) < _timeStamp (%lu/%lu)",
					timeStampLive.dwLowDateTime, timeStampLive.dwHighDateTime, _timeStamp.dwLowDateTime, _timeStamp.dwHighDateTime);
			}
			else
			{
				// usual case, uncomment if needed to log too
				//sprintf_s(buf, _countof(buf) - 1, "  in updateTimeStamp(): timeStampLive (%lu/%lu) > _timeStamp (%lu/%lu)",
				//	timeStampLive.dwLowDateTime, timeStampLive.dwHighDateTime, _timeStamp.dwLowDateTime, _timeStamp.dwHighDateTime);
			}

			if (buf[0] != '\0')
			{
				wstring issueFn = nppLogNetworkDriveIssue;
				issueFn += L".log";
				wstring nppIssueLog = nppParam.getUserPath();
				pathAppend(nppIssueLog, issueFn);
				std::string msg = wstring2string(_fullPathName, CP_UTF8);
				msg += buf;
				writeLog(nppIssueLog.c_str(), msg.c_str());
			}
		}

		_timeStamp = timeStampLive;
		doNotify(BufferChangeTimestamp);
	}
	// else (res == 0) => nothing to change
}


// Set full path file name in buffer object,
// and determinate its language by its extension.
void Buffer::setFileName(const wchar_t *fn)
{
	NppParameters& nppParamInst = NppParameters::getInstance();
	if (_fullPathName == fn)
	{
		updateTimeStamp();
		doNotify(BufferChangeTimestamp);
		return;
	}

	_fullPathName = fn;
	_fileName = PathFindFileName(_fullPathName.c_str());
	_isFromNetwork = PathIsNetworkPath(fn);

	// for _lang
	LangType determinatedLang = L_TEXT;
	wchar_t *ext = PathFindExtension(_fullPathName.c_str());
	if (*ext == '.') // extension found
	{
		ext += 1;

		// Define User Lang firstly
		const wchar_t* langName = nppParamInst.getUserDefinedLangNameFromExt(ext, _fileName);
		if (langName)
		{
			determinatedLang = L_USER;
			_userLangExt = langName;
		}
		else // if it's not user lang, then check if it's supported lang
		{
			_userLangExt.clear();
			determinatedLang = nppParamInst.getLangFromExt(ext);
		}
	}

	if (determinatedLang == L_TEXT)	// language can probably be refined
	{
		if ((wcsicmp(_fileName, L"makefile") == 0) || (wcsicmp(_fileName, L"GNUmakefile") == 0))
			determinatedLang = L_MAKEFILE;
		else if (wcsicmp(_fileName, L"CmakeLists.txt") == 0)
			determinatedLang = L_CMAKE;
		else if ((wcsicmp(_fileName, L"SConstruct") == 0) || (wcsicmp(_fileName, L"SConscript") == 0) || (wcsicmp(_fileName, L"wscript") == 0))
			determinatedLang = L_PYTHON;
		else if ((wcsicmp(_fileName, L"Rakefile") == 0) || (wcsicmp(_fileName, L"Vagrantfile") == 0))
			determinatedLang = L_RUBY;
		else if ((wcsicmp(_fileName, L"crontab") == 0) || (wcsicmp(_fileName, L"PKGBUILD") == 0) || (wcsicmp(_fileName, L"APKBUILD") == 0))
			determinatedLang = L_BASH;
	}

	updateTimeStamp();

	BufferStatusInfo lang2Change = BufferChangeNone;
	if (!_hasLangBeenSetFromMenu && (determinatedLang != _lang || _lang == L_USER))
	{
		if (_isLargeFile)
		{
			_lang = L_TEXT;
		}
		else
		{
			_lang = determinatedLang;
			lang2Change = BufferChangeLanguage;
		}
	}

	doNotify(BufferChangeFilename | BufferChangeTimestamp | lang2Change);
}


bool Buffer::checkFileState() // returns true if the status has been changed (it can change into DOC_REGULAR too). false otherwise
{
	// 1. Unsaved document cannot change by environment
	// 2. Monitoring is sent by NPPM_INTERNAL_RELOADSCROLLTOEND
	// So no need to check file status for both the above cases
	if (_currentStatus == DOC_UNNAMED || isMonitoringOn())
		return false;

	NppParameters& nppParam = NppParameters::getInstance();

	WIN32_FILE_ATTRIBUTE_DATA attributes{};
	attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	bool bWorkerThreadTerminated = true;
	DWORD dwWin32ApiError = NO_ERROR;
	BOOL bGetFileAttributesExSucceeded = getFileAttributesExWithTimeout(_fullPathName.c_str(), &attributes, 0, &bWorkerThreadTerminated, &dwWin32ApiError);
	bool fileExists = (bGetFileAttributesExSucceeded && (attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES) && !(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));

#ifndef	_WIN64
	bool isWow64Off = false;
	if (!fileExists)
	{
		nppParam.safeWow64EnableWow64FsRedirection(FALSE);
		isWow64Off = true;

		bGetFileAttributesExSucceeded = getFileAttributesExWithTimeout(_fullPathName.c_str(), &attributes, 0, &bWorkerThreadTerminated, &dwWin32ApiError);
		fileExists = (bGetFileAttributesExSucceeded && (attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES) && !(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
	}
#endif

	if (!fileExists && nppParam.doNppLogNetworkDriveIssue())
	{
		wstring issueFn = nppLogNetworkDriveIssue;
		issueFn += L".log";
		wstring nppIssueLog = nppParam.getUserPath();
		pathAppend(nppIssueLog, issueFn);
		std::string msg = wstring2string(_fullPathName, CP_UTF8);
		msg += "  in Buffer::checkFileState(), getFileAttributesExWithTimeout returned ";
		if (bGetFileAttributesExSucceeded)
		{
			msg += "TRUE";
		}
		else
		{
			msg += "FALSE";
		}

		if (bWorkerThreadTerminated)
		{
			msg += ", its worker thread had to be forcefully terminated due to timeout reached!";
		}
		else
		{
			msg += ", its worker thread finished successfully within the timeout given, ";
			if (attributes.dwFileAttributes == INVALID_FILE_ATTRIBUTES)
				msg += "dwFileAttributes == INVALID_FILE_ATTRIBUTES ! (WIN32API Error Code: " + std::to_string(dwWin32ApiError) + ")";
			else
				msg += "dwFileAttributes has the FILE_ATTRIBUTE_DIRECTORY flag set!";
		}
		writeLog(nppIssueLog.c_str(), msg.c_str());
	}

	bool isOK = false;
	if (_currentStatus == DOC_INACCESSIBLE && !fileExists)	//document is absent on its first load - we set readonly and not dirty, and make it be as document which has been deleted
	{
		_currentStatus = DOC_DELETED;//DOC_INACCESSIBLE;
		_isInaccessible = true;
		_isFileReadOnly = true;
		_isDirty = false;
		_timeStamp = {};
		doNotify(BufferChangeStatus | BufferChangeReadonly | BufferChangeTimestamp);
		isOK = true;
	}
	else if (_currentStatus != DOC_DELETED && !fileExists)	//document has been deleted
	{
		_currentStatus = DOC_DELETED;
		_isFileReadOnly = false;
		_isDirty = true;	//dirty since no match with filesystem
		_timeStamp = {};
		doNotify(BufferChangeStatus | BufferChangeReadonly | BufferChangeTimestamp);
		isOK = true;
	}
	else if (_currentStatus == DOC_DELETED && fileExists) //document has returned from its grave
	{
		// fileExists==true here means that we can use the previously obtained attributes safely
		_isFileReadOnly = attributes.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
		_currentStatus = DOC_MODIFIED;
		_timeStamp = attributes.ftLastWriteTime;
		if (_reloadFromDiskRequestGuard.try_lock())
		{
			doNotify(BufferChangeStatus | BufferChangeReadonly | BufferChangeTimestamp);
			_reloadFromDiskRequestGuard.unlock();
		}
		isOK = true;
	}
	else if (bGetFileAttributesExSucceeded)
	{
		int mask = 0;	//status always 'changes', even if from modified to modified
		bool isFileReadOnly = attributes.dwFileAttributes & FILE_ATTRIBUTE_READONLY;
		if (isFileReadOnly != _isFileReadOnly)
		{
			_isFileReadOnly = isFileReadOnly;
			mask |= BufferChangeReadonly;
		}

		LONG res = CompareFileTime(&_timeStamp, &attributes.ftLastWriteTime);
		if (res == -1 || res == 1)
			// (res == -1) => attributes.ftLastWriteTime is later, it means the file has been modified outside of Notepad++ - usual case
			// 
			// (res == 1)  => The timestamp get directly from the file on disk is earlier than buffer's timestamp - unusual case
			//                It can happen when user copies a backup of editing file somewhere-else firstly, then modifies the editing file in Notepad++ and saves it.
			//                Now user copies the backup back to erase the modified editing file outside Notepad++ (via Explorer).
		{
			if (nppParam.doNppLogNetworkDriveIssue())
			{
				char buf[1024]{};
				if (res == 1)
				{
					sprintf_s(buf, _countof(buf) - 1, "  in checkFileState(): attributes.ftLastWriteTime (%lu/%lu) < _timeStamp (%lu/%lu)",
						attributes.ftLastWriteTime.dwLowDateTime, attributes.ftLastWriteTime.dwHighDateTime, _timeStamp.dwLowDateTime, _timeStamp.dwHighDateTime);
				}
				else
				{
					// usual state after a file modification, uncomment if needed to log too
					//sprintf_s(buf, _countof(buf) - 1, "  in checkFileState(): attributes.ftLastWriteTime (%lu/%lu) > _timeStamp (%lu/%lu)",
					//	attributes.ftLastWriteTime.dwLowDateTime, attributes.ftLastWriteTime.dwHighDateTime, _timeStamp.dwLowDateTime, _timeStamp.dwHighDateTime);
				}

				if (buf[0] != '\0')
				{
					wstring issueFn = nppLogNetworkDriveIssue;
					issueFn += L".log";
					wstring nppIssueLog = nppParam.getUserPath();
					pathAppend(nppIssueLog, issueFn);
					std::string msg = wstring2string(_fullPathName, CP_UTF8);
					msg += buf;
					writeLog(nppIssueLog.c_str(), msg.c_str());
				}
			}

			_timeStamp = attributes.ftLastWriteTime;
			mask |= BufferChangeTimestamp;
			_currentStatus = DOC_MODIFIED;
			mask |= BufferChangeStatus;	//status always 'changes', even if from modified to modified
		}
		// else res == 0 => nothing to change

		if (mask != 0)
		{
			if (_reloadFromDiskRequestGuard.try_lock())
			{
				doNotify(mask);
				_reloadFromDiskRequestGuard.unlock();
				return true;
			}
		}

		return false;
	}

#ifndef	_WIN64
	if (isWow64Off)
	{
		nppParam.safeWow64EnableWow64FsRedirection(TRUE);
	}
#endif

	return isOK;
}

void Buffer::reload()
{
	WIN32_FILE_ATTRIBUTE_DATA attributes{};
	attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	if (GetFileAttributesEx(_fullPathName.c_str(), GetFileExInfoStandard, &attributes) != 0)
	{
		_timeStamp = attributes.ftLastWriteTime;
		_currentStatus = DOC_NEEDRELOAD;
		doNotify(BufferChangeTimestamp | BufferChangeStatus);
	}
}

int64_t Buffer::getFileLength() const
{
	if (_currentStatus == DOC_UNNAMED)
		return -1;

	WIN32_FILE_ATTRIBUTE_DATA attributes{};
	attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	if (GetFileAttributesEx(_fullPathName.c_str(), GetFileExInfoStandard, &attributes) != 0)
	{
		LARGE_INTEGER size{};
		size.LowPart = attributes.nFileSizeLow;
		size.HighPart = attributes.nFileSizeHigh;
		return size.QuadPart;
	}
	return -1;
}

wstring Buffer::getTimeString(FILETIME rawtime) const
{
	wstring result;
	SYSTEMTIME utcSystemTime, localSystemTime;
	FileTimeToSystemTime(&rawtime, &utcSystemTime);
	SystemTimeToTzSpecificLocalTime(nullptr, &utcSystemTime, &localSystemTime);

	const size_t dateTimeStrLen = 256;
	wchar_t bufDate[dateTimeStrLen] = { '\0' };
	GetDateFormat(LOCALE_USER_DEFAULT, 0, &localSystemTime, nullptr, bufDate, dateTimeStrLen);
	result += bufDate;
	result += ' ';

	wchar_t bufTime[dateTimeStrLen] = { '\0' };
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, &localSystemTime, nullptr, bufTime, dateTimeStrLen);
	result += bufTime;

	return result;
}

wstring Buffer::getFileTime(fileTimeType ftt) const
{
	WIN32_FILE_ATTRIBUTE_DATA attributes{};
	attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	if (GetFileAttributesEx(_currentStatus == DOC_UNNAMED ? _backupFileName.c_str() : _fullPathName.c_str(), GetFileExInfoStandard, &attributes) != 0)
	{
		FILETIME rawtime;
		switch (ftt)
		{
			case ft_created:
				rawtime = attributes.ftCreationTime;
				break;
			case ft_modified:
				rawtime = attributes.ftLastWriteTime;
				break;
			default:
				rawtime = attributes.ftLastAccessTime;
				break;
		}

		return getTimeString(rawtime);
	}

	return L"";
}


void Buffer::setPosition(const Position & pos, const ScintillaEditView * identifier)
{
	int index = indexOfReference(identifier);
	if (index == -1)
		return;
	_positions[index] = pos;
}


Position& Buffer::getPosition(const ScintillaEditView* identifier)
{
	int index = indexOfReference(identifier);
	return _positions.at(index);
}


void Buffer::setHeaderLineState(const std::vector<size_t> & folds, ScintillaEditView * identifier)
{
	int index = indexOfReference(identifier);
	if (index == -1)
		return;

	//deep copy
	std::vector<size_t> & local = _foldStates[index];
	local.clear();
	size_t size = folds.size();
	for (size_t i = 0; i < size; ++i)
		local.push_back(folds[i]);
}


const std::vector<size_t> & Buffer::getHeaderLineState(const ScintillaEditView * identifier) const
{
	int index = indexOfReference(identifier);
	return _foldStates.at(index);
}


Lang * Buffer::getCurrentLang() const
{
	NppParameters& nppParam = NppParameters::getInstance();
	int i = 0;
	Lang *l = nppParam.getLangFromIndex(i);
	++i;
	while (l)
	{
		if (l->_langID == _lang)
			return l;

		l = nppParam.getLangFromIndex(i);
		++i;
	}
	return nullptr;
}


int Buffer::indexOfReference(const ScintillaEditView * identifier) const
{
	size_t size = _referees.size();
	for (size_t i = 0; i < size; ++i)
	{
		if (_referees[i] == identifier)
			return static_cast<int>(i);
	}
	return -1;	//not found
}


int Buffer::addReference(ScintillaEditView * identifier)
{
	if (indexOfReference(identifier) != -1)
		return _references;

	_referees.push_back(identifier);
	_positions.push_back(Position());
	_foldStates.push_back(std::vector<size_t>());
	++_references;
	return _references;
}


int Buffer::removeReference(const ScintillaEditView * identifier)
{
	int indexToPop = indexOfReference(identifier);
	if (indexToPop == -1)
		return _references;

	_referees.erase(_referees.begin() + indexToPop);
	_positions.erase(_positions.begin() + indexToPop);
	_foldStates.erase(_foldStates.begin() + indexToPop);
	_references--;
	return _references;
}


void Buffer::setHideLineChanged(bool isHide, size_t location)
{
	//First run through all docs without removing markers
	for (int i = 0; i < _references; ++i)
		_referees.at(i)->notifyMarkers(this, isHide, location, false); // (i == _references-1));

	if (!isHide) // no deleting if hiding lines
	{
		//Then all docs to remove markers.
		for (int i = 0; i < _references; ++i)
			_referees.at(i)->notifyMarkers(this, isHide, location, true);
	}
}


void Buffer::setDeferredReload() // triggers a reload on the next Document access
{
	_isDirty = false;	//when reloading, just set to false, since it should be marked as clean
	_needReloading = true;
	doNotify(BufferChangeDirty);
}

bool Buffer::allowBraceMach() const
{
	const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
	return (!_isLargeFile || nppGui._largeFileRestriction._allowBraceMatch) || !nppGui._largeFileRestriction._isEnabled;
}

bool Buffer::allowAutoCompletion() const
{
	const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
	return (!_isLargeFile || nppGui._largeFileRestriction._allowAutoCompletion) || !nppGui._largeFileRestriction._isEnabled;
}

bool Buffer::allowSmartHilite() const
{
	const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
	return (!_isLargeFile || nppGui._largeFileRestriction._allowSmartHilite) || !nppGui._largeFileRestriction._isEnabled;
}

bool Buffer::allowClickableLink() const
{
	const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
	return (!_isLargeFile || nppGui._largeFileRestriction._allowClickableLink) || !nppGui._largeFileRestriction._isEnabled;
}

//filemanager

FileManager::~FileManager()
{
	for (std::vector<Buffer *>::iterator it = _buffers.begin(), end = _buffers.end(); it != end; ++it)
	{
		delete *it;
	}
}

void FileManager::init(Notepad_plus * pNotepadPlus, ScintillaEditView * pscratchTilla)
{
	_pNotepadPlus = pNotepadPlus;
	_pscratchTilla = pscratchTilla;
	_pscratchTilla->execute(SCI_SETUNDOCOLLECTION, false);	//dont store any undo information
	_scratchDocDefault = (Document)_pscratchTilla->execute(SCI_GETDOCPOINTER);
	_pscratchTilla->execute(SCI_ADDREFDOCUMENT, 0, _scratchDocDefault);
}

void FileManager::checkFilesystemChanges(bool bCheckOnlyCurrentBuffer)
{
	if (bCheckOnlyCurrentBuffer)
	{
		Buffer* buffer = _pNotepadPlus->getCurrentBuffer();
		buffer->checkFileState();
	}
	else
	{
		for (int i = int(_nbBufs) - 1; i >= 0; i--)
		{
			if (i >= int(_nbBufs))
			{
				if (_nbBufs == 0)
					return;

				i = int(_nbBufs) - 1;
			}
			_buffers[i]->checkFileState();	//something has changed. Triggers update automatically
		}
	}
}

size_t FileManager::getNbDirtyBuffers() const
{
	size_t nb_dirtyBufs = 0;
	for (size_t i = 0; i < _nbBufs; ++i)
	{
		if (_buffers[i]->_isDirty)
			++nb_dirtyBufs;
	}
	return nb_dirtyBufs;
}

int FileManager::getBufferIndexByID(BufferID id)
{
	for (size_t i = 0; i < _nbBufs; ++i)
	{
		if (_buffers[i]->_id == id)
			return static_cast<int>(i);
	}
	return -1;
}

Buffer* FileManager::getBufferByIndex(size_t index)
{
	if (index >= _buffers.size())
		return nullptr;
	return _buffers.at(index);
}


void FileManager::beNotifiedOfBufferChange(Buffer* theBuf, int mask)
{
	_pNotepadPlus->notifyBufferChanged(theBuf, mask);
}


void FileManager::addBufferReference(BufferID buffer, ScintillaEditView * identifier)
{
	Buffer* buf = getBufferByID(buffer);
	buf->addReference(identifier);
}


void FileManager::closeBuffer(BufferID id, const ScintillaEditView* identifier)
{
	int index = getBufferIndexByID(id);
	Buffer* buf = getBufferByIndex(index);

	int refs = buf->removeReference(identifier);

	if (!refs) // buffer can be deallocated
	{
		_pscratchTilla->execute(SCI_RELEASEDOCUMENT, 0, buf->_doc);	//release for FileManager, Document is now gone
		_buffers.erase(_buffers.begin() + index);
		delete buf;
		_nbBufs--;
	}
}


// backupFileName is sentinel of backup mode: if it's not NULL, then we use it (load it). Otherwise we use filename
BufferID FileManager::loadFile(const wchar_t* filename, Document doc, int encoding, const wchar_t* backupFileName, FILETIME fileNameTimestamp)
{
	if (!filename)
		return BUFFER_INVALID;

	//Get file size
	int64_t fileSize = -1;
	const wchar_t* pPath = filename;
	if (!doesFileExist(pPath))
	{
		pPath = backupFileName;
	}

	if (pPath)
	{
		WIN32_FILE_ATTRIBUTE_DATA attributes{};
		attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
		if (getFileAttributesExWithTimeout(pPath, &attributes))
		{
			LARGE_INTEGER size{};
			size.LowPart = attributes.nFileSizeLow;
			size.HighPart = attributes.nFileSizeHigh;

			fileSize = size.QuadPart;
		}
	}

	if (fileSize == -1)
	{
		// we cannot continue (or Scintilla will throw SC_STATUS_FAILURE in the loadFileData later)
		// - no error message here as this can be also a dir with files to open or globbing attempt
		return BUFFER_INVALID;
	}

	// * the auto-completion feature will be disabled for large files
	// * the session snapshots and periodic backups feature will be disabled for large files
	// * the backups on save feature will be disabled for large files
	const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
	bool isLargeFile = false;
	if (nppGui._largeFileRestriction._isEnabled)
		isLargeFile = fileSize >= nppGui._largeFileRestriction._largeFileSizeDefInByte;

	// Due to the performance issue, the Word Wrap feature will be disabled if it's ON
	if (isLargeFile && nppGui._largeFileRestriction._deactivateWordWrap)
	{
		bool isWrap = _pNotepadPlus->_pEditView->isWrap();
		if (isWrap)
		{
			_pNotepadPlus->command(IDM_VIEW_WRAP);
		}
	}

	bool ownDoc = false;
	if (!doc)
	{
		// if file exceeds the _largeFileSizeDefInByte, disable the Scintilla styling (results in half memory consumption)
		doc = static_cast<Document>(_pscratchTilla->execute(SCI_CREATEDOCUMENT, 0,
			isLargeFile ? SC_DOCUMENTOPTION_STYLES_NONE | SC_DOCUMENTOPTION_TEXT_LARGE : SC_DOCUMENTOPTION_TEXT_LARGE));
		ownDoc = true;
	}

	wchar_t fullpath[MAX_PATH] = { 0 };
	if (isWin32NamespacePrefixedFileName(filename)) // This function checks for the \\?\ prefix
	{
		// use directly the raw file name, skip the GetFullPathName WINAPI
		wcsncpy_s(fullpath, _countof(fullpath), filename, _TRUNCATE);
	}
	else
	{
		::GetFullPathName(filename, MAX_PATH, fullpath, NULL);
		if (wcschr(fullpath, '~'))
		{
			::GetLongPathName(fullpath, fullpath, MAX_PATH);
		}
	}

	bool isSnapshotMode = (backupFileName != NULL) && doesFileExist(backupFileName);
	if (isSnapshotMode && !doesFileExist(fullpath)) // if backup mode and fullpath doesn't exist, we guess is UNTITLED
	{
		wcscpy_s(fullpath, MAX_PATH, filename); // we restore fullpath with filename, in our case is "new  #"
	}

	Utf8_16_Read UnicodeConvertor;	//declare here so we can get information after loading is done

	char* data = new char[blockSize + 8]; // +8 for incomplete multibyte char

	LoadedFileFormat loadedFileFormat;
	loadedFileFormat._encoding = encoding;
	loadedFileFormat._eolFormat = EolType::unknown;
	loadedFileFormat._language = L_TEXT;

	bool loadRes = loadFileData(doc, fileSize, backupFileName ? backupFileName : fullpath, data, &UnicodeConvertor, loadedFileFormat);

	delete[] data;

	if (loadRes)
	{
		Buffer* newBuf = new Buffer(this, _nextBufferID, doc, DOC_REGULAR, fullpath, isLargeFile);
		BufferID id = newBuf;
		newBuf->_id = id;

		if (backupFileName != NULL)
		{
			newBuf->_backupFileName = backupFileName;
			if (!doesFileExist(fullpath))
			{
				newBuf->_currentStatus = DOC_UNNAMED;
				newBuf->setTabCreatedTimeStringFromBakFile();
			}
		}

		const FILETIME zeroTimeStamp = {};
		LONG res = CompareFileTime(&fileNameTimestamp, &zeroTimeStamp);
		if (res != 0) // res == 1 or res == -1
			newBuf->_timeStamp = fileNameTimestamp;

		// restore the encoding (ANSI based) while opening the existing file
		if (newBuf->_lang == L_ASCII)
			newBuf->setEncoding(NPP_CP_DOS_437);
		else
			newBuf->setEncoding(-1);

		// if not a large file, no file extension, and the language has been detected,  we use the detected value
		if (!newBuf->_isLargeFile && ((newBuf->getLangType() == L_TEXT) && (loadedFileFormat._language != L_TEXT)))
			newBuf->setLangType(loadedFileFormat._language);

		setLoadedBufferEncodingAndEol(newBuf, UnicodeConvertor, loadedFileFormat._encoding, loadedFileFormat._eolFormat);

		_buffers.push_back(newBuf);
		++_nbBufs;

		//determine buffer properties
		++_nextBufferID;
		return id;
	}
	else //failed loading, release document
	{
		if (ownDoc)
			_pscratchTilla->execute(SCI_RELEASEDOCUMENT, 0, doc);	//Failure, so release document
		return BUFFER_INVALID;
	}
}


bool FileManager::reloadBuffer(BufferID id)
{
	Buffer* buf = getBufferByID(id);
	Document doc = buf->getDocument();
	Utf8_16_Read UnicodeConvertor;

	LoadedFileFormat loadedFileFormat;
	loadedFileFormat._encoding = buf->getEncoding();
	loadedFileFormat._eolFormat = EolType::unknown;
	loadedFileFormat._language = buf->getLangType();

	buf->setLoadedDirty(false);	// Since the buffer will be reloaded from the disk, and it will be clean (not dirty), we can set _isLoadedDirty false safetly.
								// Set _isLoadedDirty false before calling "_pscratchTilla->execute(SCI_CLEARALL);" in loadFileData() to avoid setDirty in SCN_SAVEPOINTREACHED / SCN_SAVEPOINTLEFT

	//Get file size
	int64_t fileSize = 0;
	WIN32_FILE_ATTRIBUTE_DATA attributes{};
	attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	getFileAttributesExWithTimeout(buf->getFullPathName(), &attributes);
	if (attributes.dwFileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}
	else
	{
		LARGE_INTEGER size{};
		size.LowPart = attributes.nFileSizeLow;
		size.HighPart = attributes.nFileSizeHigh;

		fileSize = size.QuadPart;
	}

	
	char* data = new char[blockSize + 8]; // +8 for incomplete multibyte char

	buf->_canNotify = false;	//disable notify during file load, we don't want dirty status to be triggered
	bool res = loadFileData(doc, fileSize, buf->getFullPathName(), data, &UnicodeConvertor, loadedFileFormat);
	buf->_canNotify = true;

	delete[] data;

	if (res)
	{
		// now we are synchronized with the file on disk, so reset relevant flags
		buf->setUnsync(false);
		buf->setDirty(false); // if the _isUnsync was true before the reloading, the _isDirty had been set to true somehow in the loadFileData()

		buf->setSavePointDirty(false);

		setLoadedBufferEncodingAndEol(buf, UnicodeConvertor, loadedFileFormat._encoding, loadedFileFormat._eolFormat);
	}

	return res;
}


void FileManager::setLoadedBufferEncodingAndEol(Buffer* buf, const Utf8_16_Read& UnicodeConvertor, int encoding, EolType bkformat)
{
	if (encoding == -1)
	{
		NppParameters& nppParamInst = NppParameters::getInstance();
		const NewDocDefaultSettings & ndds = (nppParamInst.getNppGUI()).getNewDocDefaultSettings();

		UniMode um = UnicodeConvertor.getEncoding();
		if (um == uni7Bit)
			um = (ndds._openAnsiAsUtf8) ? uniCookie : uni8Bit;

		buf->setUnicodeMode(um);
	}
	else
	{
		// Test if encoding is set to UTF8 w/o BOM (usually for utf8 indicator of xml or html)
		buf->setEncoding((encoding == SC_CP_UTF8)?-1:encoding);
		buf->setUnicodeMode(uniCookie);
	}

	// Since the buffer will be reloaded from the disk, EOL might have been changed
	if (bkformat != EolType::unknown)
		buf->setEolFormat(bkformat);
}


bool FileManager::reloadBufferDeferred(BufferID id)
{
	Buffer* buf = getBufferByID(id);
	buf->setDeferredReload();
	return true;
}

bool FileManager::deleteFile(BufferID id)
{
	const Buffer* buf = getBufferByID(id);
	wstring fileNamePath = buf->getFullPathName();

	// Make sure to form a string with double '\0' terminator.
	fileNamePath.append(1, '\0');

	if (!doesFileExist(fileNamePath.c_str()))
		return false;

	SHFILEOPSTRUCT fileOpStruct = {};
	fileOpStruct.hwnd = NULL;
	fileOpStruct.pFrom = fileNamePath.c_str();
	fileOpStruct.pTo = NULL;
	fileOpStruct.wFunc = FO_DELETE;
	fileOpStruct.fFlags = FOF_ALLOWUNDO;
	fileOpStruct.fAnyOperationsAborted = false;
	fileOpStruct.hNameMappings         = NULL;
	fileOpStruct.lpszProgressTitle     = NULL;

	return SHFileOperation(&fileOpStruct) == 0;
}


bool FileManager::moveFile(BufferID id, const wchar_t * newFileName)
{
	Buffer* buf = getBufferByID(id);
	const wchar_t *fileNamePath = buf->getFullPathName();
	if (::MoveFileEx(fileNamePath, newFileName, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH) == 0)
		return false;

	buf->setFileName(newFileName);
	return true;
}


/*
Specs and Algorithm of session snapshot & periodic backup system:
Notepad++ quits without asking for saving unsaved file.
It restores all the unsaved files and document as the states they left.

For existing file (c:\tmp\foo.h)
	- Open
	In the next session, Notepad++
	1. load backup\FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) if exist, otherwise load FILENAME (c:\tmp\foo.h).
	2. if backup\FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) is loaded, set it dirty (red).
	3. if backup\FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) is loaded, last modif timestamp of FILENAME (c:\tmp\foo.h), compare with tracked timestamp (in session.xml).
	4. in the case of unequal result, tell user the FILENAME (c:\tmp\foo.h) was modified. ask user if he want to reload FILENAME(c:\tmp\foo.h)

	- Editing
	when a file starts being modified, a file will be created with name: FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776)
	the Buffer object will associate with this FILENAME@CREATION_TIMESTAMP file (backup\foo.h@198776).
	1. sync: (each 3-5 second) backup file will be saved, if buffer is dirty, and modification is present (a bool on modified notificatin).
	2. sync: each save file, or close file, the backup file will be deleted (if buffer is not dirty).
	3. before switch off to another tab (or close files on exit), check 1 & 2 (sync with backup).

	- Close
	In the current session, Notepad++
	1. track FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) if exist (in session.xml).
	2. track last modified timestamp of FILENAME (c:\tmp\foo.h) if FILENAME@CREATION_TIMESTAMP (backup\foo.h@198776) was tracked  (in session.xml).

For untitled document (new  4)
	- Open
	In the next session, Notepad++
	1. open file UNTITLED_NAME@CREATION_TIMESTAMP (backup\new  4@198776)
	2. set label as UNTITLED_NAME (new  4) and disk icon as red.

	- Editing
	when a untitled document starts being modified, a backup file will be created with name: UNTITLED_NAME@CREATION_TIMESTAMP (backup\new  4@198776)
	the Buffer object will associate with this UNTITLED_NAME@CREATION_TIMESTAMP file (backup\new  4@198776).
	1. sync: (each 3-5 second) backup file will be saved, if buffer is dirty, and modification is present (a bool on modified notificatin).
	2. sync: if untitled document is saved, or closed, the backup file will be deleted.
	3. before switch off to another tab (or close documents on exit), check 1 & 2 (sync with backup).

	- CLOSE
	In the current session, Notepad++
	1. track UNTITLED_NAME@CREATION_TIMESTAMP (backup\new  4@198776) in session.xml.
*/

std::mutex backup_mutex;

bool FileManager::backupCurrentBuffer()
{
	Buffer* buffer = _pNotepadPlus->getCurrentBuffer();
	if (buffer->isLargeFile())
		return false;

	std::lock_guard<std::mutex> lock(backup_mutex);

	bool result = false;
	bool hasModifForSession = false;

	if (buffer->isDirty())
	{
		if (buffer->isModified()) // buffer dirty and modified, write the backup file
		{
			UniMode mode = buffer->getUnicodeMode();
			if (mode == uniCookie)
				mode = uni8Bit;	//set the mode to ANSI to prevent converter from adding BOM and performing conversions, Scintilla's data can be copied directly

			Utf8_16_Write UnicodeConvertor;
			UnicodeConvertor.setEncoding(mode);
			int encoding = buffer->getEncoding();

			wstring backupFilePath = buffer->getBackupFileName();
			if (backupFilePath.empty())
			{
				// Create file
				backupFilePath = NppParameters::getInstance().getUserPath();
				backupFilePath += L"\\backup\\";

				// if "backup" folder doesn't exist, create it.
				if (!doesDirectoryExist(backupFilePath.c_str()))
				{
					::CreateDirectory(backupFilePath.c_str(), NULL);
				}

				backupFilePath += buffer->getFileName();

				const int temBufLen = 32;
				wchar_t tmpbuf[temBufLen];
				time_t ltime = time(0);
				const struct tm* today = localtime(&ltime);
				if (!today)
					return false;

				wcsftime(tmpbuf, temBufLen, L"%Y-%m-%d_%H%M%S", today);

				backupFilePath += L"@";
				backupFilePath += tmpbuf;

				// Set created file name in buffer
				buffer->setBackupFileName(backupFilePath);

				// Session changes, save it
				hasModifForSession = true;
			}

			wchar_t fullpath[MAX_PATH]{};
			::GetFullPathName(backupFilePath.c_str(), MAX_PATH, fullpath, NULL);
			if (wcschr(fullpath, '~'))
			{
				::GetLongPathName(fullpath, fullpath, MAX_PATH);
			}

			// Make sure the backup file is not read only
			removeReadOnlyFlagFromFileAttributes(fullpath);

			std::wstring fullpathTemp = fullpath;
			fullpathTemp += L".tmp";
			if (UnicodeConvertor.openFile(buffer->isUntitled() ? fullpathTemp.c_str() : fullpath)) // Use temp only for "new #" due to they don't have the original physical existance on the hard drive
			{
				size_t lengthDoc = _pNotepadPlus->_pEditView->getCurrentDocLen();
				char* buf = (char*)_pNotepadPlus->_pEditView->execute(SCI_GETCHARACTERPOINTER);	//to get characters directly from Scintilla buffer
				bool isWrittenSuccessful = false;

				if (encoding == -1) //no special encoding; can be handled directly by Utf8_16_Write
				{
					isWrittenSuccessful = UnicodeConvertor.writeFile(buf, lengthDoc);
					if (lengthDoc == 0)
						isWrittenSuccessful = true;
				}
				else
				{
					WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
					size_t grabSize = 0;
					for (size_t i = 0; i < lengthDoc; i += grabSize)
					{
						grabSize = lengthDoc - i;
						if (grabSize > blockSize)
							grabSize = blockSize;

						int newDataLen = 0;
						int incompleteMultibyteChar = 0;
						const char *newData = wmc.encode(SC_CP_UTF8, encoding, buf+i, static_cast<int>(grabSize), &newDataLen, &incompleteMultibyteChar);
						grabSize -= incompleteMultibyteChar;
						isWrittenSuccessful = UnicodeConvertor.writeFile(newData, newDataLen);
					}

					if (lengthDoc == 0)
						isWrittenSuccessful = true;
				}
				UnicodeConvertor.closeFile();

				if (isWrittenSuccessful) // backup file has been saved
				{
					if (buffer->isUntitled()) // "new #" file is saved successfully, then we replace its only physical existence by its temp
					{
						if (doesFileExist(fullpath))
							::ReplaceFile(fullpath, fullpathTemp.c_str(), nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS, 0, 0);
						else
							::MoveFileEx(fullpathTemp.c_str(), fullpath, MOVEFILE_REPLACE_EXISTING);
					}

					buffer->setTabCreatedTimeStringFromBakFile();

					result = true;	//all done
				}
			}
		}
		else // buffer dirty but unmodified
		{
			result = true;
		}
	}
	else // buffer not dirty, sync: delete the backup file
	{
		wstring backupFilePath = buffer->getBackupFileName();
		if (!backupFilePath.empty())
		{
			// delete backup file
			wstring file2Delete = buffer->getBackupFileName();
			buffer->setBackupFileName(wstring());
			result = (::DeleteFile(file2Delete.c_str()) != 0);

			// Session changes, save it
			hasModifForSession = true;
		}
		//printStr(L"backup deleted in backupCurrentBuffer"));
		result = true; // no backup file to delete
	}
	//printStr(L"backup sync"));

	if (result && hasModifForSession)
	{
		//printStr(buffer->getBackupFileName().c_str());
		_pNotepadPlus->saveCurrentSession();
	}
	return result;
}

bool FileManager::deleteBufferBackup(BufferID id)
{
	Buffer* buffer = getBufferByID(id);
	bool result = true;
	wstring backupFilePath = buffer->getBackupFileName();
	if (!backupFilePath.empty())
	{
		// delete backup file
		buffer->setBackupFileName(wstring());
		result = (::DeleteFile(backupFilePath.c_str()) != 0);
	}

	return result;
}

std::mutex save_mutex;

SavingStatus FileManager::saveBuffer(BufferID id, const wchar_t* filename, bool isCopy)
{
	std::lock_guard<std::mutex> lock(save_mutex);

	Buffer* buffer = getBufferByID(id);
	bool isHiddenOrSys = false;

	wchar_t fullpath[MAX_PATH] = { 0 };
	if (isWin32NamespacePrefixedFileName(filename))
	{
		// use directly the raw file name, skip the GetFullPathName WINAPI
		wcsncpy_s(fullpath, _countof(fullpath), filename, _TRUNCATE);
	}
	else
	{
		::GetFullPathName(filename, MAX_PATH, fullpath, NULL);
		if (wcschr(fullpath, '~'))
		{
			::GetLongPathName(fullpath, fullpath, MAX_PATH);
		}
	}
	
	wchar_t dirDest[MAX_PATH];
	wcscpy_s(dirDest, MAX_PATH, fullpath);
	::PathRemoveFileSpecW(dirDest);

	const wchar_t* currentBufFilePath = buffer->getFullPathName();
	ULARGE_INTEGER freeBytesForUser;
	 
	BOOL getFreeSpaceSuccessful = getDiskFreeSpaceWithTimeout(dirDest, &freeBytesForUser);
	if (getFreeSpaceSuccessful)
	{
		int64_t fileSize = buffer->getFileLength();
		if (fileSize >= 0 && lstrcmp(fullpath, currentBufFilePath) == 0) // if file to save does exist, and it's an operation "Save" but not "Save As"
		{
			// if file exists and the operation "Save" but not "Save As", its current length should be considered as part of free room space since the file itself will be overrrided 
			freeBytesForUser.QuadPart += fileSize;
		}

		// determinate if free space is enough
		if (freeBytesForUser.QuadPart < buffer->docLength())
			return SavingStatus::NotEnoughRoom;
	}

	WIN32_FILE_ATTRIBUTE_DATA attributes{};
	attributes.dwFileAttributes = INVALID_FILE_ATTRIBUTES;
	getFileAttributesExWithTimeout(fullpath, &attributes);
	if (attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES && !(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		isHiddenOrSys = (attributes.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0;
		if (isHiddenOrSys)
			::SetFileAttributes(filename, attributes.dwFileAttributes & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM));
	}

	UniMode mode = buffer->getUnicodeMode();
	if (mode == uniCookie)
		mode = uni8Bit;	//set the mode to ANSI to prevent converter from adding BOM and performing conversions, Scintilla's data can be copied directly

	Utf8_16_Write UnicodeConvertor;
	UnicodeConvertor.setEncoding(mode);

	int encoding = buffer->getEncoding();

	if (UnicodeConvertor.openFile(fullpath))
	{
		_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, buffer->_doc);	//generate new document

		size_t lengthDoc = _pscratchTilla->getCurrentDocLen();
		char* buf = (char*)_pscratchTilla->execute(SCI_GETCHARACTERPOINTER);	//to get characters directly from Scintilla buffer
		bool isWrittenSuccessful = false;

		if (encoding == -1) //no special encoding; can be handled directly by Utf8_16_Write
		{
			isWrittenSuccessful = UnicodeConvertor.writeFile(buf, lengthDoc);
			if (lengthDoc == 0)
				isWrittenSuccessful = true;
		}
		else
		{
			if (lengthDoc == 0)
			{
				isWrittenSuccessful = UnicodeConvertor.writeFile(buf, 0);
			}
			else
			{
				WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
				size_t grabSize = 0;
				for (size_t i = 0; i < lengthDoc; i += grabSize)
				{
					grabSize = lengthDoc - i;
					if (grabSize > blockSize)
						grabSize = blockSize;

					int newDataLen = 0;
					int incompleteMultibyteChar = 0;
					const char* newData = wmc.encode(SC_CP_UTF8, encoding, buf + i, static_cast<int>(grabSize), &newDataLen, &incompleteMultibyteChar);
					grabSize -= incompleteMultibyteChar;
					isWrittenSuccessful = UnicodeConvertor.writeFile(newData, newDataLen);
				}
			}
		}

		UnicodeConvertor.closeFile();

		// Error, we didn't write the entire document to disk.
		if (!isWrittenSuccessful)
		{
			_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
			return SavingStatus::SaveWritingFailed;
		}

		if (isHiddenOrSys)
			::SetFileAttributes(fullpath, attributes.dwFileAttributes);

		if (isCopy) // "Save a Copy As..." command
		{
			_pscratchTilla->execute(SCI_SETMODEVENTMASK, MODEVENTMASK_OFF);
			_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);
			_pscratchTilla->execute(SCI_SETMODEVENTMASK, MODEVENTMASK_ON);
			return SavingStatus::SaveOK;	//all done - we don't change the current buffer's path to "fullpath", since it's "Save a Copy As..." action.
		}

		buffer->setFileName(fullpath);

		// if not a large file and language is normal text (not defined)
		// we may try determinate its language from its content 
		if (!buffer->isLargeFile() && buffer->_lang == L_TEXT)
		{
			LangType detectedLang = detectLanguageFromTextBegining((unsigned char*)buf, lengthDoc);

			// if a language is detected from the content
			if (detectedLang != L_TEXT)
			{
				buffer->_lang = detectedLang;
				buffer->doNotify(BufferChangeFilename | BufferChangeTimestamp | BufferChangeLanguage);
			}
		}
		buffer->setDirty(false);
		buffer->setUnsync(false);
		buffer->setSavePointDirty(false);
		buffer->setStatus(DOC_REGULAR);
		buffer->checkFileState();


		_pscratchTilla->execute(SCI_SETSAVEPOINT);
		_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);

		wstring backupFilePath = buffer->getBackupFileName();
		if (!backupFilePath.empty())
		{
			// delete backup file
			buffer->setBackupFileName(wstring());
			::DeleteFile(backupFilePath.c_str());
		}

		return SavingStatus::SaveOK;
	}
	else
	{
		return SavingStatus::SaveOpenFailed;
	}
}

size_t FileManager::nextUntitledNewNumber() const
{
	std::vector<size_t> usedNumbers;
	for (size_t i = 0; i < _buffers.size(); i++)
	{
		Buffer *buf = _buffers.at(i);
		if (buf->isUntitled())
		{
			// if untitled document is invisible, then don't put its number into array (so its number is available to be used)
			if ((buf->_referees[0])->isVisible())
			{
				wstring newTitle = ((NppParameters::getInstance()).getNativeLangSpeaker())->getLocalizedStrFromID("tab-untitled-string", UNTITLED_STR);
				wchar_t *numberStr = buf->_fileName + newTitle.length();
				int usedNumber = _wtoi(numberStr);
				usedNumbers.push_back(usedNumber);
			}
		}
	}

	size_t newNumber = 1;
	bool numberAvailable = true;
	bool found = false;
	do
	{
		for (size_t j = 0; j < usedNumbers.size(); j++)
		{
			numberAvailable = true;
			found = false;
			if (usedNumbers[j] == newNumber)
			{
				numberAvailable = false;
				found = true;
				break;
			}
		}

		if (!numberAvailable)
			newNumber++;

		if (!found)
			break;

	} while (!numberAvailable);

	return newNumber;
}

BufferID FileManager::newEmptyDocument()
{
	wstring newTitle = ((NppParameters::getInstance()).getNativeLangSpeaker())->getLocalizedStrFromID("tab-untitled-string", UNTITLED_STR);

	wchar_t nb[10];
	wsprintf(nb, L"%d", static_cast<int>(nextUntitledNewNumber()));
	newTitle += nb;

	Document doc = static_cast<Document>(_pscratchTilla->execute(SCI_CREATEDOCUMENT, 0, SC_DOCUMENTOPTION_TEXT_LARGE)); // this already sets a reference for filemanager
	Buffer* newBuf = new Buffer(this, _nextBufferID, doc, DOC_UNNAMED, newTitle.c_str(), false);

	NppParameters& nppParamInst = NppParameters::getInstance();
	const NewDocDefaultSettings& ndds = (nppParamInst.getNppGUI()).getNewDocDefaultSettings();
	newBuf->_lang = ndds._lang;

	BufferID id = newBuf;
	newBuf->_id = id;
	newBuf->setTabCreatedTimeStringWithCurrentTime();
	_buffers.push_back(newBuf);
	++_nbBufs;
	++_nextBufferID;
	return id;
}

BufferID FileManager::newPlaceholderDocument(const wchar_t* missingFilename, int whichOne, const wchar_t* userCreatedSessionName)
{
	NppParameters& nppParamInst = NppParameters::getInstance();

	if (!nppParamInst.theWarningHasBeenGiven())
	{
		int res = 0;
		if (userCreatedSessionName)
		{
			res = (nppParamInst.getNativeLangSpeaker())->messageBox(
				"FileInaccessibleUserSession",
				_pNotepadPlus->_pEditView->getHSelf(),
				L"Some files from your manually-saved session \"$STR_REPLACE$\" are inaccessible. They can be opened as empty and read-only documents as placeholders.\n\nWould you like to create those placeholders?\n\nNOTE: Choosing not to create the placeholders or closing them later, your manually-saved session will NOT be modified on exit.",
				L"File inaccessible",
				MB_YESNO | MB_APPLMODAL,
				0,
				userCreatedSessionName);
		}
		else
		{
			res = (nppParamInst.getNativeLangSpeaker())->messageBox(
				"FileInaccessibleDefaultSessionXml",
				_pNotepadPlus->_pEditView->getHSelf(),
				L"Some files from your past session are inaccessible. They can be opened as empty and read-only documents as placeholders.\n\nWould you like to create those placeholders?\n\nNOTE: Choosing not to create the placeholders or closing them later, your session WILL BE MODIFIED ON EXIT! We suggest you backup your \"session.xml\" now.",
				L"File inaccessible",
				MB_YESNO | MB_APPLMODAL);
		}

		nppParamInst.setTheWarningHasBeenGiven(true);
		nppParamInst.setPlaceHolderEnable(res == IDYES);
	}

	if (!nppParamInst.isPlaceHolderEnabled())
		return BUFFER_INVALID;

	BufferID buf = MainFileManager.newEmptyDocument();
	_pNotepadPlus->loadBufferIntoView(buf, whichOne);
	buf->setFileName(missingFilename);
	buf->_currentStatus = DOC_INACCESSIBLE;
	return buf;
}

BufferID FileManager::bufferFromDocument(Document doc, bool isMainEditZone)
{
	NppParameters& nppParamInst = NppParameters::getInstance();
	std::wstring newTitle = L"newNonMainEditZoneInvisibleTitle "; // This title is invisible for "Document map", "Find result" or other Scintilla controls other than _mainEditView and _subEditView.
                                                                  // Its strong length and the space at the end are for preventing the tab name modification from the collision with it.

	if (isMainEditZone) // only _mainEditView or _subEditView is main edit zone, so we count new number of doc only for these 2 scintilla edit views.
	{
		newTitle = (nppParamInst.getNativeLangSpeaker())->getLocalizedStrFromID("tab-untitled-string", UNTITLED_STR);
		wchar_t nb[10];
		wsprintf(nb, L"%d", static_cast<int>(nextUntitledNewNumber()));
		newTitle += nb;
	}

	Buffer* newBuf = new Buffer(this, _nextBufferID, doc, DOC_UNNAMED, newTitle.c_str(), false);
	BufferID id = newBuf;
	newBuf->_id = id;
	const NewDocDefaultSettings& ndds = (nppParamInst.getNppGUI()).getNewDocDefaultSettings();
	newBuf->_lang = ndds._lang;
	newBuf->setTabCreatedTimeStringWithCurrentTime();
	_buffers.push_back(newBuf);
	++_nbBufs;

	++_nextBufferID;
	return id;
}

int FileManager::detectCodepage(char* buf, size_t len)
{
	int codepage = -1;
	uchardet_t ud = uchardet_new();
	uchardet_handle_data(ud, buf, len);
	uchardet_data_end(ud);
	const char* cs = uchardet_get_charset(ud);
	if (stricmp(cs, "TIS-620") != 0) // TIS-620 detection is disabled here because uchardet detects usually wrongly UTF-8 as TIS-620
		codepage = EncodingMapper::getInstance().getEncodingFromString(cs);
	uchardet_delete(ud);
	return codepage;
}

LangType FileManager::detectLanguageFromTextBegining(const unsigned char *data, size_t dataLen)
{
	struct FirstLineLanguages
	{
		std::string pattern;
		LangType lang;
	};

	// Is the buffer at least the size of a BOM?
	if (dataLen <= 3)
		return L_TEXT;

	// Eliminate BOM if present
	size_t i = 0;
	if ((data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) || // UTF8 BOM
		(data[0] == 0xFE && data[1] == 0xFF && data[2] == 0x00) || // UTF16 BE BOM
		(data[0] == 0xFF && data[1] == 0xFE && data[2] == 0x00))   // UTF16 LE BOM
		i += 3;

	// Skip any space-like char
	for (; i < dataLen; ++i)
	{
		if (data[i] != ' ' && data[i] != '\t' && data[i] != '\n' && data[i] != '\r')
			break;
	}

	if (i == dataLen)
		return L_TEXT;

	// Create the buffer to need to test
	const size_t longestLength = std::min<size_t>(40, dataLen - i); // shebangs can be large
	std::string buf2Test = std::string((const char *)data + i, longestLength);

	// Is there a \r or \n in the buffer? If so, truncate it
	auto cr = buf2Test.find("\r");
	auto nl = buf2Test.find("\n");
	auto crnl = std::min<size_t>(cr, nl);
	if (crnl != std::string::npos && crnl < longestLength)
		buf2Test = std::string((const char *)data + i, crnl);

	// First test for a Unix-like Shebang
	// See https://en.wikipedia.org/wiki/Shebang_%28Unix%29 for more details about Shebang
	std::string shebang = "#!";

	size_t foundPos = buf2Test.find(shebang);
	if (foundPos == 0)
	{
		// Make a list of the most commonly used languages
		const size_t NB_SHEBANG_LANGUAGES = 6;
		FirstLineLanguages ShebangLangs[NB_SHEBANG_LANGUAGES] = {
			{ "sh",		L_BASH },
			{ "python", L_PYTHON },
			{ "perl",	L_PERL },
			{ "php",	L_PHP },
			{ "ruby",	L_RUBY },
			{ "node",	L_JAVASCRIPT }
		};

		// Go through the list of languages
		for (i = 0; i < NB_SHEBANG_LANGUAGES; ++i)
		{
			if (buf2Test.find(ShebangLangs[i].pattern) != std::string::npos)
			{
				return ShebangLangs[i].lang;
			}
		}

		// Unrecognized shebang (there is always room for improvement ;-)
		return L_TEXT;
	}

	// Are there any other patterns we know off?
	const size_t NB_FIRST_LINE_LANGUAGES = 5;
	FirstLineLanguages languages[NB_FIRST_LINE_LANGUAGES] = {
		{ "<?xml",			L_XML },
		{ "<?php",			L_PHP },
		{ "<html",			L_HTML },
		{ "<!DOCTYPE html",	L_HTML },
		{ "<?",				L_PHP } // MUST be after "<?php" and "<?xml" to get the result as accurate as possible
	};

	for (i = 0; i < NB_FIRST_LINE_LANGUAGES; ++i)
	{
		foundPos = buf2Test.find(languages[i].pattern);
		if (foundPos == 0)
		{
			return languages[i].lang;
		}
	}

	// Unrecognized first line, we assume it is a text file for now
	return L_TEXT;
}

bool FileManager::loadFileData(Document doc, int64_t fileSize, const wchar_t * filename, char* data, Utf8_16_Read * unicodeConvertor, LoadedFileFormat& fileFormat)
{
	// Check file size firstly
	// size/6 is the normal room Scintilla keeps for editing, but here we limit it to 1MiB when loading (maybe we want to load big files without editing them too much)
	int64_t bufferSizeRequested = fileSize + std::min<int64_t>(1LL << 20, fileSize / 6);
	
	NppParameters& nppParam = NppParameters::getInstance();
	NativeLangSpeaker* pNativeSpeaker = nppParam.getNativeLangSpeaker();

	if (bufferSizeRequested > INT_MAX)
	{
		// As a 32bit application, we cannot allocate 2 buffer of more than INT_MAX size (it takes the whole address space).
		if (nppParam.archType() == IMAGE_FILE_MACHINE_I386)
		{
			pNativeSpeaker->messageBox("FileTooBigToOpen",
				_pNotepadPlus->_pEditView->getHSelf(),
				L"File is too big to be opened by Notepad++",
				L"File size problem",
				MB_OK | MB_APPLMODAL);

			return false;
		}
		else // x64
		{
			const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();
			if (!nppGui._largeFileRestriction._suppress2GBWarning) 
			{
				int res = pNativeSpeaker->messageBox("WantToOpenHugeFile",
					_pNotepadPlus->_pEditView->getHSelf(),
					L"Opening a huge file of 2GB+ could take several minutes.\nDo you want to open it?",
					L"Opening huge file warning",
					MB_YESNO | MB_APPLMODAL);

				if (res == IDYES)
				{
					// Do nothing
				}
				else
				{
					return false;
				}
			}
		}
	}

	// Check if the file is located on a network. If it is, verify the existence of the file's directory.
	// Note: We're checking the directory's existence instead of the file itself to avoid the GetAttributesEx cache issue.
	// Just before calling loadFileData, the doesFileExist function was called, which should return false if there's a network problem.
	// If execution reaches here during a network connection problem, it means doesFileExist returned true incorrectly due to GetAttributesEx caching.
	// Therefore, we avoid calling doesFileExist again and instead call doesDirectoryExist to ensure accuracy.
	bool isNetworkDirDisconnected = false;
	if (PathIsNetworkPath(filename))
	{
		wchar_t dir[MAX_PATH]{};
		wcscpy_s(dir,filename);
		PathRemoveFileSpec(dir);
		isNetworkDirDisconnected = !doesDirectoryExist(dir);
	}

	if (isNetworkDirDisconnected)
		return false; // If network ressource is not reachable, we stop here for not having hanging issue because of _wfopen


	FILE* fp = _wfopen(filename, L"rb");

	if (!fp)
		return false;

	//Setup scratchtilla for new filedata
	_pscratchTilla->execute(SCI_SETSTATUS, SC_STATUS_OK); // reset error status
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, doc);
	bool ro = _pscratchTilla->execute(SCI_GETREADONLY) != 0;
	if (ro)
	{
		_pscratchTilla->execute(SCI_SETREADONLY, false);
	}
	_pscratchTilla->execute(SCI_CLEARALL);
	_pscratchTilla->execute(SCI_SETUNDOCOLLECTION, false); // disable undocollection while loading a file


	if (fileFormat._language < L_EXTERNAL)
	{
		const char* lexerNameID = ScintillaEditView::_langNameInfoArray[fileFormat._language]._lexerID;
		_pscratchTilla->execute(SCI_SETILEXER, 0, reinterpret_cast<LPARAM>(CreateLexer(lexerNameID)));
	}
	else
	{
		int id = fileFormat._language - L_EXTERNAL;
		ExternalLangContainer& externalLexer = nppParam.getELCFromIndex(id);
		const char* lexerName = externalLexer._name.c_str();
		if (externalLexer.fnCL)
			_pscratchTilla->execute(SCI_SETILEXER, 0, reinterpret_cast<LPARAM>(externalLexer.fnCL(lexerName)));
	}

	if (fileFormat._encoding != -1)
		_pscratchTilla->execute(SCI_SETCODEPAGE, SC_CP_UTF8);

	bool success = true;
	EolType format = EolType::unknown;
	int sciStatus = SC_STATUS_OK;
	wchar_t szException[64] = {'\0'};
	__try
	{
		// First allocate enough memory for the whole file (this will reduce memory copy during loading)
		_pscratchTilla->execute(SCI_ALLOCATE, WPARAM(bufferSizeRequested));
		sciStatus = static_cast<int>(_pscratchTilla->execute(SCI_GETSTATUS));
		if ((sciStatus > SC_STATUS_OK) && (sciStatus < SC_STATUS_WARN_START))
			throw std::runtime_error("Scintilla error");

		size_t lenFile = 0;
		size_t lenConvert = 0;	//just in case conversion results in 0, but file not empty
		bool isFirstTime = true;
		int incompleteMultibyteChar = 0;

		do
		{
			lenFile = fread(data + incompleteMultibyteChar, 1, blockSize - incompleteMultibyteChar, fp) + incompleteMultibyteChar;
			if (ferror(fp) != 0)
			{
				success = false;
				break;
			}

			if (lenFile == 0) break;

            if (isFirstTime)
            {
				const NppGUI& nppGui = NppParameters::getInstance().getNppGUI();

				// check if file contain any BOM
                if (Utf8_16_Read::determineEncoding((unsigned char *)data, lenFile) != uni8Bit)
                {
                    // if file contains any BOM, then encoding will be erased,
                    // and the document will be interpreted as UTF
					fileFormat._encoding = -1;
				}
				else if (fileFormat._encoding == -1)
				{
					if (nppGui._detectEncoding)
						fileFormat._encoding = detectCodepage(data, lenFile);
                }
				
				bool isLargeFile = fileSize >= nppGui._largeFileRestriction._largeFileSizeDefInByte;
				if (!isLargeFile && fileFormat._language == L_TEXT)
				{
					// check the language du fichier
					fileFormat._language = detectLanguageFromTextBegining((unsigned char *)data, lenFile);
				}

                isFirstTime = false;
            }

			if (fileFormat._encoding != -1)
			{
				if (fileFormat._encoding == SC_CP_UTF8)
				{
					// Pass through UTF-8 (this does not check validity of characters, thus inserting a multi-byte character in two halfs is working)
					_pscratchTilla->execute(SCI_APPENDTEXT, lenFile, reinterpret_cast<LPARAM>(data));
				}
				else
				{
					WcharMbcsConvertor& wmc = WcharMbcsConvertor::getInstance();
					int newDataLen = 0;
					const char *newData = wmc.encode(fileFormat._encoding, SC_CP_UTF8, data, static_cast<int32_t>(lenFile), &newDataLen, &incompleteMultibyteChar);
					_pscratchTilla->execute(SCI_APPENDTEXT, newDataLen, reinterpret_cast<LPARAM>(newData));
				}

				if (format == EolType::unknown)
					format = getEOLFormatForm(data, lenFile, EolType::unknown);
			}
			else
			{
				lenConvert = unicodeConvertor->convert(data, lenFile);
				_pscratchTilla->execute(SCI_APPENDTEXT, lenConvert, reinterpret_cast<LPARAM>(unicodeConvertor->getNewBuf()));
				if (format == EolType::unknown)
					format = getEOLFormatForm(unicodeConvertor->getNewBuf(), unicodeConvertor->getNewSize(), EolType::unknown);
			}

			sciStatus = static_cast<int>(_pscratchTilla->execute(SCI_GETSTATUS));
			if ((sciStatus > SC_STATUS_OK) && (sciStatus < SC_STATUS_WARN_START))
				throw std::runtime_error("Scintilla error");

			if (incompleteMultibyteChar != 0)
			{
				// copy bytes to next buffer
				memcpy(data, data + blockSize - incompleteMultibyteChar, incompleteMultibyteChar);
			}
		}
		while (lenFile > 0);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		switch (sciStatus)
		{
			case SC_STATUS_OK:
				// either the Scintilla doesn't catch this exception or the error is in the Notepad++ code, report the exception anyway
#if defined(__GNUC__)
				// there is the std::current_exception() possibility, but getting the real exception code from there requires an ugly hack,
				// because of the std::exception_ptr has its members _Data1 (GetExceptionCode) and _Data2 (GetExceptionInformation) private
				_stprintf_s(szException, _countof(szException), L"unknown exception");
#else
				_stprintf_s(szException, _countof(szException), L"0x%X (SEH)", ::GetExceptionCode());
#endif
				break;
			case SC_STATUS_BADALLOC:
			{
				pNativeSpeaker->messageBox("FileMemoryAllocationFailed",
					_pNotepadPlus->_pEditView->getHSelf(),
					L"There is probably not enough contiguous free memory for the file being loaded by Notepad++.",
					L"Exception: File memory allocation failed",
					MB_OK | MB_APPLMODAL);
			}
			[[fallthrough]];
			case SC_STATUS_FAILURE:
			default:
				_stprintf_s(szException, _countof(szException), L"%d (Scintilla)", sciStatus);
				break;
		}

		if (sciStatus != SC_STATUS_BADALLOC)
		{
			pNativeSpeaker->messageBox("FileLoadingException",
				_pNotepadPlus->_pEditView->getHSelf(),
				L"An error occurred while loading the file!",
				L"Exception code: $STR_REPLACE$",
				MB_OK | MB_APPLMODAL,
				0,
				szException);
		}
		success = false;
	}

	fclose(fp);

	// broadcast the format
	if (format == EolType::unknown)
	{
		const NewDocDefaultSettings & ndds = (nppParam.getNppGUI()).getNewDocDefaultSettings(); // for ndds._format
		fileFormat._eolFormat = ndds._format;

		//for empty files, if the default for new files is UTF8, and "Apply to opened ANSI files" is set, apply it
		if ((fileSize == 0) && (fileFormat._encoding < 1))
		{
			if (ndds._unicodeMode == uniCookie && ndds._openAnsiAsUtf8)
				fileFormat._encoding = SC_CP_UTF8;
		}
	}
	else
	{
		fileFormat._eolFormat = format;
	}


	_pscratchTilla->execute(SCI_EMPTYUNDOBUFFER);
	_pscratchTilla->execute(SCI_SETSAVEPOINT);
	_pscratchTilla->execute(SCI_SETUNDOCOLLECTION, true);

	if (ro)
		_pscratchTilla->execute(SCI_SETREADONLY, true);

	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, _scratchDocDefault);

	return success;
}


BufferID FileManager::getBufferFromName(const wchar_t* name)
{
	for (auto buf : _buffers)
	{
		if (wcsicmp(name, buf->getFullPathName()) == 0)
		{
			if (!(buf->_referees.empty()) && buf->_referees[0]->isVisible())
			{
				return buf->getID();
			}
		}
	}
	return BUFFER_INVALID;
}


BufferID FileManager::getBufferFromDocument(Document doc)
{
	for (size_t i = 0; i < _nbBufs; ++i)
	{
		if (_buffers[i]->_doc == doc)
			return _buffers[i]->_id;
	}
	return BUFFER_INVALID;
}


bool FileManager::createEmptyFile(const wchar_t * path)
{
	Win32_IO_File file(path);
	return file.isOpened();
}


int FileManager::getFileNameFromBuffer(BufferID id, wchar_t * fn2copy)
{
	if (getBufferIndexByID(id) == -1)
		return -1;

	const Buffer* buf = getBufferByID(id);

	if (fn2copy)
		lstrcpy(fn2copy, buf->getFullPathName());

	return lstrlen(buf->getFullPathName());
}


size_t FileManager::docLength(Buffer* buffer) const
{
	Document curDoc = _pscratchTilla->execute(SCI_GETDOCPOINTER);
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, buffer->_doc);
	size_t docLen = _pscratchTilla->getCurrentDocLen();
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, curDoc);
	return docLen;
}


void FileManager::removeHotSpot(Buffer* buffer) const
{
	Document curDoc = _pscratchTilla->execute(SCI_GETDOCPOINTER);
	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, buffer->_doc);

	_pscratchTilla->execute(SCI_SETINDICATORCURRENT, URL_INDIC);
	size_t docLen = _pscratchTilla->getCurrentDocLen();
	_pscratchTilla->execute(SCI_INDICATORCLEARRANGE, 0, docLen);

	_pscratchTilla->execute(SCI_SETDOCPOINTER, 0, curDoc);
}
