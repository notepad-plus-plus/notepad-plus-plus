//This code was retrieved from
//http://www.thunderguy.com/semicolon/2002/08/15/visual-c-exception-handling/3/
//(Visual C++ exception handling)
//By Bennett
//Formatting Slightly modified for N++

// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef WIN32_EXCEPTION_H
#define  WIN32_EXCEPTION_H

#include <exception>
#include <windows.h>

typedef const void* ExceptionAddress; // OK on Win32 platform

class Win32Exception : public std::exception
{
public:
    static void 		installHandler();
    static void 		removeHandler();
    virtual const char* what()  const throw() { return _event;    };
    ExceptionAddress 	where() const         { return _location; };
    unsigned int		code()  const         { return _code;     };
	EXCEPTION_POINTERS* info()  const         { return _info;     };
	
protected:
    Win32Exception(EXCEPTION_POINTERS * info);	//Constructor only accessible by exception handler
    static void 		translate(unsigned code, EXCEPTION_POINTERS * info);

private:
    const char * _event;
    ExceptionAddress _location;
    unsigned int _code;

	EXCEPTION_POINTERS * _info;
};

class Win32AccessViolation: public Win32Exception
{
public:
    bool 				isWrite()    const { return _isWrite;    };
    ExceptionAddress	badAddress() const { return _badAddress; };
private:
    Win32AccessViolation(EXCEPTION_POINTERS * info);

    bool _isWrite;
    ExceptionAddress _badAddress;

    friend void Win32Exception::translate(unsigned code, EXCEPTION_POINTERS* info);
};

#endif // WIN32_EXCEPTION_H