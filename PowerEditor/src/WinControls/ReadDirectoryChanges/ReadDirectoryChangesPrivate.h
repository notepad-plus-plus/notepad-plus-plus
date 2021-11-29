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

class CReadDirectoryChanges;

namespace ReadDirectoryChangesPrivate
{

class CReadChangesServer;

///////////////////////////////////////////////////////////////////////////

// All functions in CReadChangesRequest run in the context of the worker thread.
// One instance of this object is created for each call to AddDirectory().
class CReadChangesRequest
{
public:
	CReadChangesRequest(CReadChangesServer* pServer, LPCTSTR sz, BOOL b, DWORD dw, DWORD size);

	~CReadChangesRequest();

	bool OpenDirectory();

	void BeginRead();

	// The dwSize is the actual number of bytes sent to the APC.
	void BackupBuffer(DWORD dwSize)
	{
		// We could just swap back and forth between the two
		// buffers, but this code is easier to understand and debug.
		memcpy(&m_BackupBuffer[0], &m_Buffer[0], dwSize);
	}

	void ProcessNotification();

	void RequestTermination()
	{
		::CancelIo(m_hDirectory);
		::CloseHandle(m_hDirectory);
		m_hDirectory = nullptr;
	}

	CReadChangesServer* m_pServer;

protected:

	static VOID CALLBACK NotificationCompletion(
			DWORD dwErrorCode,							// completion code
			DWORD dwNumberOfBytesTransfered,			// number of bytes transferred
			LPOVERLAPPED lpOverlapped);					// I/O information buffer

	// Parameters from the caller for ReadDirectoryChangesW().
	DWORD m_dwFilterFlags = 0;
	BOOL m_bIncludeChildren = FALSE;
	std::wstring m_wstrDirectory;

	// Result of calling CreateFile().
	HANDLE		m_hDirectory = nullptr;

	// Required parameter for ReadDirectoryChangesW().
	OVERLAPPED	m_Overlapped;

	// Data buffer for the request.
	// Since the memory is allocated by malloc, it will always
	// be aligned as required by ReadDirectoryChangesW().
	vector<BYTE> m_Buffer;

	// Double buffer strategy so that we can issue a new read
	// request before we process the current buffer.
	vector<BYTE> m_BackupBuffer;
};

///////////////////////////////////////////////////////////////////////////

// All functions in CReadChangesServer run in the context of the worker thread.
// One instance of this object is allocated for each instance of CReadDirectoryChanges.
// This class is responsible for thread startup, orderly thread shutdown, and shimming
// the various C++ member functions with C-style Win32 functions.
class CReadChangesServer
{
public:
	explicit CReadChangesServer(CReadDirectoryChanges* pParent)
	{
		m_bTerminate=false; m_nOutstandingRequests=0;m_pBase=pParent;
	}

	static unsigned int WINAPI ThreadStartProc(LPVOID arg)
	{
		CReadChangesServer* pServer = static_cast<CReadChangesServer*>(arg);
		pServer->Run();
		return 0;
	}

	// Called by QueueUserAPC to start orderly shutdown.
	static void CALLBACK TerminateProc(  ULONG_PTR arg)
	{
		CReadChangesServer* pServer = reinterpret_cast<CReadChangesServer*>(arg);
		pServer->RequestTermination();
	}

	// Called by QueueUserAPC to add another directory.
	static void CALLBACK AddDirectoryProc(  ULONG_PTR arg)
	{
		CReadChangesRequest* pRequest = reinterpret_cast<CReadChangesRequest*>(arg);
		pRequest->m_pServer->AddDirectory(pRequest);
	}

	CReadDirectoryChanges* m_pBase = nullptr;

	volatile DWORD m_nOutstandingRequests;

protected:

	void Run()
	{
		while (m_nOutstandingRequests || !m_bTerminate)
		{
			::SleepEx(INFINITE, true);
		}
	}

	void AddDirectory( CReadChangesRequest* pBlock )
	{
		if (pBlock->OpenDirectory())
		{
			::InterlockedIncrement(&pBlock->m_pServer->m_nOutstandingRequests);
			m_pBlocks.push_back(pBlock);
			pBlock->BeginRead();
		}
		else
			delete pBlock;
	}

	void RequestTermination()
	{
		m_bTerminate = true;

		for (DWORD i=0; i<m_pBlocks.size(); ++i)
		{
			// Each Request object will delete itself.
			m_pBlocks[i]->RequestTermination();
		}

		m_pBlocks.clear();
	}

	vector<CReadChangesRequest*> m_pBlocks;

	bool m_bTerminate = false;
};

}
