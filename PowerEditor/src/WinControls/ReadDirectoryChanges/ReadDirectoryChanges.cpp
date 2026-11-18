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

#include <process.h>
#include "ReadDirectoryChanges.h"
#include "ReadDirectoryChangesPrivate.h"

using namespace ReadDirectoryChangesPrivate;

///////////////////////////////////////////////////////////////////////////
// CReadDirectoryChanges

CReadDirectoryChanges::CReadDirectoryChanges(): m_Notifications()
{
	m_pServer	= new CReadChangesServer(this);
}

CReadDirectoryChanges::~CReadDirectoryChanges()
{
	Terminate();
	delete m_pServer;
}

void CReadDirectoryChanges::Init()
{
	//
	// Kick off the worker thread, which will be
	// managed by CReadChangesServer.
	//
	m_hThread = (HANDLE)_beginthreadex(NULL,
		0,
		CReadChangesServer::ThreadStartProc,
		m_pServer,
		0,
		&m_dwThreadId
		);
}

void CReadDirectoryChanges::Terminate()
{
	if (m_hThread)
	{
		::QueueUserAPC(CReadChangesServer::TerminateProc, m_hThread, (ULONG_PTR)m_pServer);
		::WaitForSingleObjectEx(m_hThread, 10000, true);
		::CloseHandle(m_hThread);

		m_hThread = NULL;
		m_dwThreadId = 0;
	}
}

void CReadDirectoryChanges::AddDirectory( LPCTSTR szDirectory, BOOL bWatchSubtree, DWORD dwNotifyFilter, DWORD dwBufferSize )
{
	if (!m_hThread)
		Init();

	if (!m_hThread)
		return;

	CReadChangesRequest* pRequest = new CReadChangesRequest(m_pServer, szDirectory, bWatchSubtree, dwNotifyFilter, dwBufferSize);
	QueueUserAPC(CReadChangesServer::AddDirectoryProc, m_hThread, (ULONG_PTR)pRequest);
}

void CReadDirectoryChanges::Push(DWORD dwAction, std::wstring& wstrFilename)
{
	TDirectoryChangeNotification dirChangeNotif = TDirectoryChangeNotification(dwAction, wstrFilename);
	m_Notifications.push(dirChangeNotif);
}

bool  CReadDirectoryChanges::Pop(DWORD& dwAction, std::wstring& wstrFilename)
{
	TDirectoryChangeNotification pair;
	if (!m_Notifications.pop(pair))
		return false;

	dwAction = pair.first;
	wstrFilename = pair.second;

	return true;
}
