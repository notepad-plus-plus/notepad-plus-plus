//This code was retrieved from
//http://www.thunderguy.com/semicolon/2002/08/15/visual-c-exception-handling/3/
//(Visual C++ exception handling)
//By Bennett
//Formatting Slightly modified for N++

#include "Win32Exception.h"
#include "eh.h"

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
