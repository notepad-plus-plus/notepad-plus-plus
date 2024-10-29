// This file is part of Notepad++ project
// Copyright (C)2021 Pavel Nedev (pg.nedev@gmail.com)

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

#include <locale>
#include <codecvt>
#include <shlwapi.h>
#include "FileInterface.h"
#include "Parameters.h"

using namespace std;

Win32_IO_File::Win32_IO_File(const wchar_t *fname)
{
	if (fname)
	{
		std::wstring fn = fname;
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		_path = converter.to_bytes(fn);

		WIN32_FILE_ATTRIBUTE_DATA attributes_original{};
		DWORD dispParam = CREATE_ALWAYS;
		bool fileExists = false;
		bool isTimeoutReached = false;
		// Store the file creation date & attributes for a possible use later...
		if (getFileAttributesExWithTimeout(fname, &attributes_original, 0, &isTimeoutReached))
		{
			fileExists = (attributes_original.dwFileAttributes != INVALID_FILE_ATTRIBUTES && !(attributes_original.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
		}

		if (fileExists)
		{
			// Check the existence of Alternate Data Streams
			WIN32_FIND_STREAM_DATA findData;
			HANDLE hFind = FindFirstStreamW(fname, FindStreamInfoStandard, &findData, 0);
			if (hFind != INVALID_HANDLE_VALUE) // Alternate Data Streams found
			{
				dispParam = TRUNCATE_EXISTING;
				FindClose(hFind);
			}
		}
		else
		{
			bool isFromNetwork = PathIsNetworkPath(fname);
			if (isFromNetwork && isTimeoutReached) // The file doesn't exist, and the file is a network file, plus the network problem has been detected due to timeout
				return;                             // In this case, we don't call createFile to prevent hanging
		}

		_hFile = ::CreateFileW(fname, _accessParam, _shareParam, NULL, dispParam, _attribParam, NULL);

		// Race condition management:
		//  If file didn't exist while calling getFileAttributesExWithTimeout, but before calling CreateFileW, file is created: use CREATE_ALWAYS is OK
		//  If file did exist while calling getFileAttributesExWithTimeout, but before calling CreateFileW, file is deleted: use TRUNCATE_EXISTING will cause the error
		if (dispParam == TRUNCATE_EXISTING && _hFile == INVALID_HANDLE_VALUE && ::GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			dispParam = CREATE_ALWAYS;
			_hFile = ::CreateFileW(fname, _accessParam, _shareParam, NULL, dispParam, _attribParam, NULL);
		}

		if (fileExists && (dispParam == CREATE_ALWAYS) && (_hFile != INVALID_HANDLE_VALUE))
		{
			// restore back the original creation date & attributes
			::SetFileTime(_hFile, &(attributes_original.ftCreationTime), NULL, NULL);
			::SetFileAttributesW(fname, (_attribParam | attributes_original.dwFileAttributes));
		}

		NppParameters& nppParam = NppParameters::getInstance();
		if (nppParam.isEndSessionStarted() && nppParam.doNppLogNulContentCorruptionIssue())
		{
			wstring issueFn = nppLogNulContentCorruptionIssue;
			issueFn += L".log";
			wstring nppIssueLog = nppParam.getUserPath();
			pathAppend(nppIssueLog, issueFn);

			std::string msg = _path;
			if (_hFile != INVALID_HANDLE_VALUE)
			{
				msg += " is opened.";
			}
			else
			{
				msg += " failed to open, CreateFileW ErrorCode: ";
				msg += std::to_string(::GetLastError());
			}
			writeLog(nppIssueLog.c_str(), msg.c_str());
		}
	}
}

void Win32_IO_File::close()
{
	if (isOpened())
	{
		NppParameters& nppParam = NppParameters::getInstance();

		DWORD flushError = NOERROR;
		if (_written)
		{
			if (!::FlushFileBuffers(_hFile))
			{
				flushError = ::GetLastError();
				std::wstring errNumberMsg = std::to_wstring(flushError) + L" - " + GetLastErrorAsString(flushError);

				if (!nppParam.isEndSessionCritical())
				{
					// because of there is not an externally forced shutdown/restart of Windows in progress,
					// we can at least alert the user that the file data could not actually be saved

					std::wstring curFilePath;
					const DWORD cchPathBuf = MAX_PATH + 128;
					wchar_t pathbuf[cchPathBuf]{};
					// the dwFlags used below are the most error-proof and informative
					DWORD dwRet = ::GetFinalPathNameByHandle(_hFile, pathbuf, cchPathBuf, FILE_NAME_OPENED | VOLUME_NAME_NT);
					if ((dwRet == 0) || (dwRet >= cchPathBuf))
					{
						// probably insufficient path-buffer length, the classic style must suffice
						std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
						curFilePath = converter.from_bytes(_path);
					}
					else
					{
						// ok
						// - the chosen file path style here will allow us to also see the MS MUP (multiple UNC provider) style path
						//   for a possible network drive mapped here as the usual disk-letter, e.g.:
						//     "\Device\Mup\NETWORK_SERVER_DNS_NAME_OR_IP\SERVER_PATH\FILENAME" 
						//     "\Device\Mup\NETWORK_SERVER_DNS_NAME_OR_IP@SSL@PORT_NUMBER\DavWWWRoot\SERVER_PATH\FILENAME"
						// - classic local disk will be visible here like: "\Device\HarddiskVolume2\ABSOLUTE_PATH_WITHOUT_DRIVENAME\FILENAME"
						curFilePath = pathbuf;
					}

					std::wstring errMsg = L"Notepad++ has encountered a serious system problem while saving:\n\n";
					errMsg += curFilePath;
					errMsg += L"\n\nThat file, temporarily stored in the system cache, cannot be finally committed to the storage device selected! \
This is probably a storage driver or hardware issue, beyond the control of the Notepad++. \
Please try using another storage and also check if your saved data is not corrupted.\n\nError Code reported: ";
					errMsg += errNumberMsg;
					::MessageBoxW(NULL, errMsg.c_str(), L"WARNING - filebuffer flushing fail!", MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);
				}
				else
				{
					// writing breif log here
					std::wstring nppFlushFileBuffersFailsLog = L"nppFlushFileBuffersFails.log";
					std::wstring nppIssueLog = nppParam.getUserPath();
					pathAppend(nppIssueLog, nppFlushFileBuffersFailsLog);

					std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
					std::string errNumberMsgA = converter.to_bytes(errNumberMsg);

					writeLog(nppIssueLog.c_str(), errNumberMsgA.c_str());
				}
			}
		}
		::CloseHandle(_hFile);

		_hFile = INVALID_HANDLE_VALUE;

		if (nppParam.isEndSessionStarted() && nppParam.doNppLogNulContentCorruptionIssue())
		{
			wstring issueFn = nppLogNulContentCorruptionIssue;
			issueFn += L".log";
			wstring nppIssueLog = nppParam.getUserPath();
			pathAppend(nppIssueLog, issueFn);

			std::string msg;
			if (flushError != NOERROR)
			{
				msg = "FlushFileBuffers failed with the error code: " + std::to_string(flushError) + " - ";

				LPSTR messageBuffer = nullptr;
				FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					nullptr, flushError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);
				msg += messageBuffer;

				//Free the buffer.
				LocalFree(messageBuffer);
				msg += "\n";
			}
			msg += _path;
			msg += " is closed.";
			writeLog(nppIssueLog.c_str(), msg.c_str());
		}
	}
}


bool Win32_IO_File::write(const void *wbuf, size_t buf_size)
{
	if (!isOpened() || (wbuf == nullptr))
		return false;

	// we need to split any 4GB+ data for the WriteFile WINAPI later
	constexpr DWORD c_max_dword = ~(DWORD(0)); // 0xFFFFFFFF
	size_t total_bytes_written = 0;
	size_t bytes_left_to_write = buf_size;

	BOOL success = FALSE;

	do
	{
		const DWORD bytes_to_write = (bytes_left_to_write < static_cast<size_t>(c_max_dword)) ?
			static_cast<DWORD>(bytes_left_to_write) : c_max_dword;
		DWORD bytes_written = 0;

		success = ::WriteFile(_hFile, static_cast<const char*>(wbuf) + total_bytes_written,
			bytes_to_write, &bytes_written, NULL);

		if (success)
		{
			success = (bytes_written == bytes_to_write);
			bytes_left_to_write -= static_cast<size_t>(bytes_written);
			total_bytes_written += static_cast<size_t>(bytes_written);
		}
	} while (success && bytes_left_to_write);

	NppParameters& nppParam = NppParameters::getInstance();

	if (success == FALSE)
	{
		if (nppParam.isEndSessionStarted() && nppParam.doNppLogNulContentCorruptionIssue())
		{
			wstring issueFn = nppLogNulContentCorruptionIssue;
			issueFn += L".log";
			wstring nppIssueLog = nppParam.getUserPath();
			pathAppend(nppIssueLog, issueFn);

			std::string msg = _path;
			msg += " written failed: ";
			std::wstring lastErrorMsg = GetLastErrorAsString(::GetLastError());
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			msg += converter.to_bytes(lastErrorMsg);
			writeLog(nppIssueLog.c_str(), msg.c_str());
		}

		return false;
	}
	else
	{
		if (nppParam.isEndSessionStarted() && nppParam.doNppLogNulContentCorruptionIssue())
		{
			wstring issueFn = nppLogNulContentCorruptionIssue;
			issueFn += L".log";
			wstring nppIssueLog = nppParam.getUserPath();
			pathAppend(nppIssueLog, issueFn);

			std::string msg = _path;
			msg += "  ";
			msg += std::to_string(total_bytes_written);
			msg += "/";
			msg += std::to_string(buf_size);
			msg += " bytes are written.";
			writeLog(nppIssueLog.c_str(), msg.c_str());
		}
	}

	_written = true;

	return (total_bytes_written == buf_size);
}

