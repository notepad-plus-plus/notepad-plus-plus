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


#include "Win32Exception.h"


Win32Exception::Win32Exception(EXCEPTION_POINTERS * info) {
	_location = info->ExceptionRecord->ExceptionAddress;
	_code = info->ExceptionRecord->ExceptionCode;
	_info = info;
	switch (_code) {
		case EXCEPTION_ACCESS_VIOLATION:
			_event = "Access violation";
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			_event = "Division by zero";
			break;
		default:
			_event = "Unlisted exception";
	}
}

void Win32Exception::installHandler() {
	_set_se_translator(Win32Exception::translate);
}

void  Win32Exception::removeHandler() {
	_set_se_translator(NULL);
}

void Win32Exception::translate(unsigned code, EXCEPTION_POINTERS * info) {
	// Windows guarantees that *(info->ExceptionRecord) is valid
	switch (code) {
		case EXCEPTION_ACCESS_VIOLATION:
			throw Win32AccessViolation(info);
			break;
		default:
			throw Win32Exception(info);
	}
}

Win32AccessViolation::Win32AccessViolation(EXCEPTION_POINTERS * info) : Win32Exception(info) {
	_isWrite = info->ExceptionRecord->ExceptionInformation[0] == 1;
	_badAddress = reinterpret_cast<ExceptionAddress>(info->ExceptionRecord->ExceptionInformation[1]);
}
