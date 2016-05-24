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

#warning CReadDirectoryChanges/CStringW stub used for MinGW/GCC

#define _CRT_SECURE_NO_DEPRECATE

#include "targetver.h"

#include <stdio.h>

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include <windows.h>

//#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

//#include <atlbase.h>
//#include <atlstr.h>

#include <vector>
#include <list>

using namespace std;

//#include "ThreadSafeQueue.h"

class CStringW;

class CReadDirectoryChanges
{
public:
	CReadDirectoryChanges(int nMaxChanges=1000) {};
	~CReadDirectoryChanges() {};

	void Terminate() {};

	void AddDirectory( LPCTSTR wszDirectory, BOOL bWatchSubtree, DWORD dwNotifyFilter, DWORD dwBufferSize=16384 ) {};

	HANDLE GetWaitHandle() { return (HANDLE)NULL; }

	bool Pop(DWORD& dwAction, CStringW& wstrFilename) {};

	bool CheckOverflow() {};
};

class CStringW
{
public:
	CStringW() {};
	~CStringW() {};

	GetString() { return ""; };
};
