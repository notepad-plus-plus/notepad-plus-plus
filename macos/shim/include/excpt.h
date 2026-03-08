#pragma once
// Win32 Shim: Structured Exception Handling (SEH) stubs for macOS
// SEH is MSVC-specific. On macOS we provide the type definitions
// but __try/__except are not available (use C++ exceptions instead).

#include "windef.h"

// Exception filter return values
#ifndef EXCEPTION_EXECUTE_HANDLER
#define EXCEPTION_EXECUTE_HANDLER    1
#define EXCEPTION_CONTINUE_SEARCH    0
#define EXCEPTION_CONTINUE_EXECUTION (-1)
#endif

// Exception codes
#ifndef EXCEPTION_ACCESS_VIOLATION
#define EXCEPTION_ACCESS_VIOLATION   0xC0000005
#define EXCEPTION_STACK_OVERFLOW     0xC00000FD
#define EXCEPTION_INT_DIVIDE_BY_ZERO 0xC0000094
#endif

// Exception record (simplified)
typedef struct _EXCEPTION_RECORD {
	DWORD ExceptionCode;
	DWORD ExceptionFlags;
	struct _EXCEPTION_RECORD* ExceptionRecord;
	PVOID ExceptionAddress;
	DWORD NumberParameters;
	ULONG_PTR ExceptionInformation[15];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

// Context (simplified - platform specific, just a stub)
typedef struct _CONTEXT {
	DWORD ContextFlags;
} CONTEXT, *PCONTEXT;

typedef struct _EXCEPTION_POINTERS {
	PEXCEPTION_RECORD ExceptionRecord;
	PCONTEXT ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

// ============================================================
// SEH-to-C++ exception translation macros
// __try/__except are MSVC extensions; approximate with try/catch
// ============================================================
#ifndef __try
#define __try try
#endif
#ifndef __except
#define __except(filter) catch(...)
#endif
#ifndef __finally
#define __finally catch(...) {} /* finally */
#endif
#ifndef GetExceptionCode
#define GetExceptionCode() 0
#endif
#ifndef GetExceptionInformation
#define GetExceptionInformation() nullptr
#endif
