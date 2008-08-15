//This code was retrieved from
//http://www.thunderguy.com/semicolon/2002/08/15/visual-c-exception-handling/3/
//(Visual C++ exception handling)
//By Bennett
//Formatting Slightly modified for N++

/*
this file is part of Notepad++
Copyright (C)2003 Don HO <donho@altern.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef WIN32EXCEPTION_H
#define WIN32EXCEPTION_H

#include "windows.h"
#include <exception>

typedef const void* ExceptionAddress; // OK on Win32 platform

class Win32Exception : public std::exception
{
public:
    static void 		installHandler();
    static void 		removeHandler();
    virtual const char* what()  const throw() { return _event;    };
    ExceptionAddress 	where() const         { return _location; };
    unsigned 			code()  const         { return _code;     };

protected:
    Win32Exception(const EXCEPTION_RECORD * info);	//Constructor only accessible by exception handler
    static void 		translate(unsigned code, EXCEPTION_POINTERS * info);

private:
    const char * _event;
    ExceptionAddress _location;
    unsigned int _code;
};

class Win32AccessViolation: public Win32Exception
{
public:
    bool 				isWrite()    const { return _isWrite;    };
    ExceptionAddress	badAddress() const { return _badAddress; };
private:
    Win32AccessViolation(const EXCEPTION_RECORD * info);

    bool _isWrite;
    ExceptionAddress _badAddress;

    friend void Win32Exception::translate(unsigned code, EXCEPTION_POINTERS* info);
};

#endif //WIN32EXCEPTION_H
