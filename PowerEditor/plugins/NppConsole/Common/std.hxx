/*
This file is part of Notepad++ console plugin.
Copyright ©2011 Mykhajlo Pobojnyj <mpoboyny@web.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

//std.hxx

#ifndef _STD_HXX_
#define _STD_HXX_

#ifndef WINVER
#	define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#	define _WIN32_WINNT 0x0501
#endif
#ifndef _WIN32_IE
#	define _WIN32_IE 0x0600
#endif


#include <tchar.h>
#include <stdio.h>
#include <windows.h>
#include <string>
#include <sstream>

typedef unsigned char UCHAR;

using namespace std;

typedef basic_string <wchar_t>        StrW;
typedef basic_istringstream<wchar_t>  InStrW;
typedef basic_ostringstream<wchar_t>  OutStrW;

typedef basic_string <char>       StrA;
typedef basic_istringstream<char> InStrA;
typedef basic_ostringstream<char> OutStrA;

typedef basic_string <TCHAR>       Str;
typedef basic_istringstream<TCHAR> InStr;
typedef basic_ostringstream<TCHAR> OutStr;

#define DIMOF(Array) (sizeof(Array) / sizeof(Array[0]))

inline void MBOK(PCSTR s) {
   char szTMP[128];
   GetModuleFileNameA(NULL, szTMP, DIMOF(szTMP));
   MessageBoxA(GetActiveWindow(), s, szTMP, MB_OK);
}

#if defined(_DEBUG) || defined(DEBUG)
# define _SYSLOG 1
#endif

#ifndef _SYSLOG
#	define SLog(out)
#	define SLogFu
#else //_SYSLOG
#	define SLog(out) {OutStr log; log<<"["<<__FILE__<<":"<<__LINE__<<"] "<<out; OutputDebugString(log.str().c_str());}
#	define _SYSLOG_TAB_PREF 3
	class SLogScope 
	{
		StrA m_fileName, m_fuName, m_tabs;
		int m_line, m_tabCount;
		static int SetCount(int val=0)
		{
			static int s_count =_SYSLOG_TAB_PREF;
			return s_count+=val;
		}
		public: 
			SLogScope(LPCSTR file, LPCSTR fu, int line) : m_fileName(file), m_fuName(fu), m_tabs(""), m_line(line), m_tabCount(SetCount())
			{
				for (int i=0; i<m_tabCount; i++) { 
					m_tabs+="\t ";
				}
				OutStr log;
				log<<"["<<m_fileName.c_str()<<":"<<m_line<<"] "<<m_tabs.c_str()<<m_fuName.c_str()<<" ->"; 
				OutputDebugString(log.str().c_str());
				SetCount(1);
			} 
			~SLogScope()
			{
				OutStr log;
				log<<"["<<m_fileName.c_str()<<":"<<m_line<<"] "<<m_tabs.c_str()<<m_fuName.c_str()<<" <-"; 
				OutputDebugString(log.str().c_str());
				if (m_tabCount > _SYSLOG_TAB_PREF) {
					SetCount(-1);
				}
			}
	};
#	define SLogFu SLogScope logScope(__FILE__, __FUNCTION__, __LINE__);
#endif //_SYSLOG


#define IFR(cond, val)  if ((cond)){\
                          SLog(__FUNCTION__<<" failed "<<#cond<<" return "<<#val);\
                          return (val);\
                        }
#define IFV(cond)  if ((cond)){\
                          SLog(__FUNCTION__<<" failed "<<#cond);\
                          return;\
                        }
#ifndef __GNUC__
#	pragma warning( disable : 4267 )
#	pragma warning( disable : 4244 )
#	pragma warning( disable : 4311 )
#	pragma warning( disable : 4312 )
#	pragma warning( disable : 4996 )
#endif // __GNUC__

#endif //_STD_HXX_
