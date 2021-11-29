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

#include <list>
#include <mutex>

template <typename C>
class CThreadSafeQueue : protected std::list<C>
{
protected:
	using Base = std::list<C>;

public:
	CThreadSafeQueue()
	{
		m_hEvent = ::CreateEvent(
			NULL,		// no security attributes
			FALSE,		// auto reset
			FALSE,		// non-signalled
			NULL);		// anonymous
	}

	~CThreadSafeQueue()
	{
		::CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}

	void push(C& c)
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			Base::push_back(c);
		}
		::SetEvent(m_hEvent);
	}

	bool pop(C& c)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (Base::empty())
		{
			return false;
		}

		c = Base::front();
		Base::pop_front();

		return true;
	}

	HANDLE GetWaitHandle() { return m_hEvent; }

protected:
	HANDLE m_hEvent = nullptr;
	std::mutex m_mutex;
};
