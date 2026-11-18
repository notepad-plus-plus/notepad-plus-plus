//
//	The MIT License
//
//	Copyright (c) 2010 James E Beveridge
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//	THE SOFTWARE.


//	This sample code is for my blog entry titled, "Understanding ReadDirectoryChangesW"
//	http://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw.html
//	See ReadMe.txt for overview information.

#include <shlwapi.h>
#include <cassert>
#include "ReadDirectoryChanges.h"
#include "ReadDirectoryChangesPrivate.h"


// The namespace is a convenience to emphasize that these are internals
// interfaces.  The namespace can be safely removed if you need to.
namespace ReadDirectoryChangesPrivate
{

///////////////////////////////////////////////////////////////////////////
// CReadChangesRequest

CReadChangesRequest::CReadChangesRequest(CReadChangesServer* pServer, LPCTSTR sz, BOOL b, DWORD dw, DWORD size)
: m_pServer(pServer), m_wstrDirectory(sz), m_bIncludeChildren(b), m_dwFilterFlags(dw)
{
	::ZeroMemory(&m_Overlapped, sizeof(OVERLAPPED));

	// The hEvent member is not used when there is a completion
	// function, so it's ok to use it to point to the object.
	m_Overlapped.hEvent = this;

	m_Buffer.resize(size);
	m_BackupBuffer.resize(size);
}


CReadChangesRequest::~CReadChangesRequest()
{
	// RequestTermination() must have been called successfully.
	assert(m_hDirectory == NULL);
}


bool CReadChangesRequest::OpenDirectory()
{
	// Allow this routine to be called redundantly.
	if (m_hDirectory)
		return true;

	m_hDirectory = ::CreateFileW(
		m_wstrDirectory.c_str(),					// pointer to the file name
		FILE_LIST_DIRECTORY,                // access (read/write) mode
		FILE_SHARE_READ						// share mode
		 | FILE_SHARE_WRITE
		 | FILE_SHARE_DELETE,
		NULL,                               // security descriptor
		OPEN_EXISTING,                      // how to create
		FILE_FLAG_BACKUP_SEMANTICS			// file attributes
		 | FILE_FLAG_OVERLAPPED,
		NULL);                              // file with attributes to copy

	if (m_hDirectory == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	return true;
}

void CReadChangesRequest::BeginRead()
{
	DWORD dwBytes=0;

	// This call needs to be reissued after every APC.
	::ReadDirectoryChangesW(
		m_hDirectory,						// handle to directory
		&m_Buffer[0],                       // read results buffer
		static_cast<DWORD>(m_Buffer.size()),                    // length of buffer
		m_bIncludeChildren,                 // monitoring option
		m_dwFilterFlags,                    // filter conditions
		&dwBytes,                           // bytes returned
		&m_Overlapped,                      // overlapped buffer
		&NotificationCompletion);           // completion routine
}

//static
VOID CALLBACK CReadChangesRequest::NotificationCompletion(
	DWORD dwErrorCode,									// completion code
	DWORD dwNumberOfBytesTransfered,					// number of bytes transferred
	LPOVERLAPPED lpOverlapped)							// I/O information buffer
{
	CReadChangesRequest* pBlock = static_cast<CReadChangesRequest*>(lpOverlapped->hEvent);

	if (dwErrorCode == ERROR_OPERATION_ABORTED)
	{
		::InterlockedDecrement(&pBlock->m_pServer->m_nOutstandingRequests);
		delete pBlock;
		return;
	}

	// Can't use sizeof(FILE_NOTIFY_INFORMATION) because
	// the structure is padded to 16 bytes.
	assert((dwNumberOfBytesTransfered == 0) ||
		(dwNumberOfBytesTransfered >= offsetof(FILE_NOTIFY_INFORMATION, FileName) + sizeof(wchar_t)));

	pBlock->BackupBuffer(dwNumberOfBytesTransfered);

	// Get the new read issued as fast as possible. The documentation
	// says that the original OVERLAPPED structure will not be used
	// again once the completion routine is called.
	pBlock->BeginRead();

	pBlock->ProcessNotification();
}

void CReadChangesRequest::ProcessNotification()
{
	BYTE* pBase = m_BackupBuffer.data();

	for (;;)
	{
		FILE_NOTIFY_INFORMATION& fni = (FILE_NOTIFY_INFORMATION&)*pBase;

		std::wstring wstrFilename(fni.FileName, fni.FileNameLength/sizeof(wchar_t));
		// Handle a trailing backslash, such as for a root directory.
		if (!wstrFilename.empty() && wstrFilename.back() != L'\\')
			wstrFilename = m_wstrDirectory + L"\\" + wstrFilename;
		else
			wstrFilename = m_wstrDirectory + wstrFilename;

		// If it could be a short filename, expand it.
		LPCWSTR wszFilename = ::PathFindFileNameW(wstrFilename.c_str());
		int len = lstrlenW(wszFilename);
		// The maximum length of an 8.3 filename is twelve, including the dot.
		if (len <= 12 && wcschr(wszFilename, L'~'))
		{
			// Convert to the long filename form. Unfortunately, this
			// does not work for deletions, so it's an imperfect fix.
			wchar_t wbuf[MAX_PATH];
			if (::GetLongPathNameW(wstrFilename.c_str(), wbuf, _countof(wbuf)) > 0)
				wstrFilename = wbuf;
		}

		m_pServer->m_pBase->Push(fni.Action, wstrFilename);

		if (!fni.NextEntryOffset)
			break;
		pBase += fni.NextEntryOffset;
	};
}

}
