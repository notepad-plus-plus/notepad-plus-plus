//This code was retrieved from
//http://www.thunderguy.com/semicolon/2002/08/15/visual-c-exception-handling/3/
//(Visual C++ exception handling)
//By Bennett
//Formatting Slightly modified for N++

// This file is part of Notepad++ project
// Copyright (C)2021 Don HO <don.h@free.fr>

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

#pragma once

#include <exception>
#include <windows.h>

typedef const void* ExceptionAddress; // OK on Win32 platform

class Win32Exception : public std::exception
{
public:
    static void			installHandler();
    static void			removeHandler();
    virtual const char* what()  const throw() { return _event;    }
    ExceptionAddress	where() const         { return _location; }
    unsigned int		code()  const         { return _code;     }
	EXCEPTION_POINTERS* info()  const         { return _info;     }

protected:
    explicit Win32Exception(EXCEPTION_POINTERS * info);	//Constructor only accessible by exception handler
    static void			translate(unsigned code, EXCEPTION_POINTERS * info);

private:
    const char * _event = nullptr;
    ExceptionAddress _location;
    unsigned int _code = 0;

	EXCEPTION_POINTERS * _info = nullptr;
};


class Win32AccessViolation: public Win32Exception
{
public:
    bool				isWrite()    const { return _isWrite;    }
    ExceptionAddress	badAddress() const { return _badAddress; }
private:
    explicit Win32AccessViolation(EXCEPTION_POINTERS * info);

    bool _isWrite = false;
    ExceptionAddress _badAddress;

    friend void Win32Exception::translate(unsigned code, EXCEPTION_POINTERS* info);
};
