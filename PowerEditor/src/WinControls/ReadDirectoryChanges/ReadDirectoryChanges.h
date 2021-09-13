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


#pragma once

#define _CRT_SECURE_NO_DEPRECATE

#include "targetver.h"

#include <stdio.h>

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include <windows.h>
#include <vector>
#include <list>

using namespace std;

#include "ThreadSafeQueue.h"

typedef pair<DWORD, std::wstring> TDirectoryChangeNotification;

namespace ReadDirectoryChangesPrivate
{
	class CReadChangesServer;
}

///////////////////////////////////////////////////////////////////////////


/// <summary>
/// Track changes to filesystem directories and report them
/// to the caller via a thread-safe queue.
/// </summary>
/// <remarks>
/// <para>
/// This sample code is based on my blog entry titled, "Understanding ReadDirectoryChangesW"
///	http://qualapps.blogspot.com/2010/05/understanding-readdirectorychangesw.html
/// </para><para>
/// All functions in CReadDirectoryChangesServer run in
/// the context of the calling thread.
/// </para>
/// <example><code>
/// 	CReadDirectoryChanges changes;
/// 	changes.AddDirectory(_T("C:\\"), false, dwNotificationFlags);
///
///		const HANDLE handles[] = { hStopEvent, changes.GetWaitHandle() };
///
///		while (!bTerminate)
///		{
///			::MsgWaitForMultipleObjectsEx(
///				_countof(handles),
///				handles,
///				INFINITE,
///				QS_ALLINPUT,
///				MWMO_INPUTAVAILABLE | MWMO_ALERTABLE);
///			switch (rc)
///			{
///			case WAIT_OBJECT_0 + 0:
///				bTerminate = true;
///				break;
///			case WAIT_OBJECT_0 + 1:
///				// We've received a notification in the queue.
///				{
///					DWORD dwAction;
///					std::wstring wstrFilename;
///					while (changes.Pop(dwAction, wstrFilename))
///						wprintf(L"%s %s\n", ExplainAction(dwAction), wstrFilename);
///				}
///				break;
///			case WAIT_OBJECT_0 + _countof(handles):
///				// Get and dispatch message
///				break;
///			case WAIT_IO_COMPLETION:
///				// APC complete.No action needed.
///				break;
///			}
///		}
/// </code></example>
/// </remarks>
class CReadDirectoryChanges
{
public:
	CReadDirectoryChanges();
	~CReadDirectoryChanges();

	void Init();
	void Terminate();

	/// <summary>
	/// Add a new directory to be monitored.
	/// </summary>
	/// <param name="wszDirectory">Directory to monitor.</param>
	/// <param name="bWatchSubtree">True to also monitor subdirectories.</param>
	/// <param name="dwNotifyFilter">The types of file system events to monitor, such as FILE_NOTIFY_CHANGE_ATTRIBUTES.</param>
	/// <param name="dwBufferSize">The size of the buffer used for overlapped I/O.</param>
	/// <remarks>
	/// <para>
	/// This function will make an APC call to the worker thread to issue a new
	/// ReadDirectoryChangesW call for the given directory with the given flags.
	/// </para>
	/// </remarks>
	void AddDirectory( LPCTSTR wszDirectory, BOOL bWatchSubtree, DWORD dwNotifyFilter, DWORD dwBufferSize=16384 );

	/// <summary>
	/// Return a handle for the Win32 Wait... functions that will be
	/// signaled when there is a queue entry.
	/// </summary>
	HANDLE GetWaitHandle() { return m_Notifications.GetWaitHandle(); }

	bool Pop(DWORD& dwAction, std::wstring& wstrFilename);

	// "Push" is for usage by ReadChangesRequest.  Not intended for external usage.
	void Push(DWORD dwAction, std::wstring& wstrFilename);

	unsigned int GetThreadId() { return m_dwThreadId; }

protected:
	ReadDirectoryChangesPrivate::CReadChangesServer* m_pServer = nullptr;

	HANDLE m_hThread = nullptr;

	unsigned int m_dwThreadId = 0;

	CThreadSafeQueue<TDirectoryChangeNotification> m_Notifications;
};
