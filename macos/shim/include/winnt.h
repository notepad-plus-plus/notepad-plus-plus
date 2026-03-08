#pragma once
// Win32 Shim: Windows NT type definitions for macOS

#include "windef.h"

// Access rights
#define GENERIC_READ     0x80000000L
#define GENERIC_WRITE    0x40000000L
#define GENERIC_EXECUTE  0x20000000L
#define GENERIC_ALL      0x10000000L

#define DELETE                   0x00010000L
#define READ_CONTROL             0x00020000L
#define WRITE_DAC                0x00040000L
#define WRITE_OWNER              0x00080000L
#define SYNCHRONIZE              0x00100000L
#define STANDARD_RIGHTS_REQUIRED 0x000F0000L
#define STANDARD_RIGHTS_READ     READ_CONTROL
#define STANDARD_RIGHTS_WRITE    READ_CONTROL
#define STANDARD_RIGHTS_EXECUTE  READ_CONTROL
#define STANDARD_RIGHTS_ALL      0x001F0000L

// File share modes
#define FILE_SHARE_READ    0x00000001
#define FILE_SHARE_WRITE   0x00000002
#define FILE_SHARE_DELETE  0x00000004

// File attributes
#define FILE_ATTRIBUTE_READONLY   0x00000001
#define FILE_ATTRIBUTE_HIDDEN     0x00000002
#define FILE_ATTRIBUTE_SYSTEM     0x00000004
#define FILE_ATTRIBUTE_DIRECTORY  0x00000010
#define FILE_ATTRIBUTE_ARCHIVE    0x00000020
#define FILE_ATTRIBUTE_NORMAL     0x00000080
#define FILE_ATTRIBUTE_TEMPORARY  0x00000100
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)

// File creation disposition
#define CREATE_NEW        1
#define CREATE_ALWAYS     2
#define OPEN_EXISTING     3
#define OPEN_ALWAYS       4
#define TRUNCATE_EXISTING 5

// File flags
#define FILE_FLAG_WRITE_THROUGH    0x80000000
#define FILE_FLAG_OVERLAPPED       0x40000000
#define FILE_FLAG_NO_BUFFERING     0x20000000
#define FILE_FLAG_RANDOM_ACCESS    0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN  0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE  0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS 0x02000000

// File change notification flags
#define FILE_NOTIFY_CHANGE_FILE_NAME   0x00000001
#define FILE_NOTIFY_CHANGE_DIR_NAME    0x00000002
#define FILE_NOTIFY_CHANGE_ATTRIBUTES  0x00000004
#define FILE_NOTIFY_CHANGE_SIZE        0x00000008
#define FILE_NOTIFY_CHANGE_LAST_WRITE  0x00000010
#define FILE_NOTIFY_CHANGE_CREATION    0x00000040
#define FILE_NOTIFY_CHANGE_SECURITY    0x00000100

// File action codes
#define FILE_ACTION_ADDED              0x00000001
#define FILE_ACTION_REMOVED            0x00000002
#define FILE_ACTION_MODIFIED           0x00000003
#define FILE_ACTION_RENAMED_OLD_NAME   0x00000004
#define FILE_ACTION_RENAMED_NEW_NAME   0x00000005

// Move methods
#define FILE_BEGIN   0
#define FILE_CURRENT 1
#define FILE_END     2

// Memory allocation flags
#define MEM_COMMIT      0x00001000
#define MEM_RESERVE     0x00002000
#define MEM_DECOMMIT    0x00004000
#define MEM_RELEASE     0x00008000
#define MEM_FREE        0x00010000
#define MEM_RESET       0x00080000
#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_EXECUTE           0x10
#define PAGE_EXECUTE_READ      0x20
#define PAGE_EXECUTE_READWRITE 0x40

// Heap flags
#define HEAP_ZERO_MEMORY 0x00000008
#define HEAP_GENERATE_EXCEPTIONS 0x00000004

// Wait return values
#define WAIT_OBJECT_0    0x00000000L
#define WAIT_ABANDONED   0x00000080L
#define WAIT_TIMEOUT     0x00000102L
#define WAIT_FAILED          0xFFFFFFFF
#define WAIT_IO_COMPLETION   0x000000C0L

// Thread access rights
#define THREAD_TERMINATE                 0x0001
#define THREAD_SUSPEND_RESUME            0x0002
#define THREAD_GET_CONTEXT               0x0008
#define THREAD_SET_CONTEXT               0x0010
#define THREAD_SET_INFORMATION           0x0020
#define THREAD_QUERY_INFORMATION         0x0040
#define THREAD_SET_THREAD_TOKEN          0x0080
#define THREAD_IMPERSONATE               0x0100
#define THREAD_DIRECT_IMPERSONATION      0x0200

// Process access rights
#define PROCESS_TERMINATE                0x0001
#define PROCESS_CREATE_THREAD            0x0002
#define PROCESS_VM_OPERATION             0x0008
#define PROCESS_VM_READ                  0x0010
#define PROCESS_VM_WRITE                 0x0020
#define PROCESS_DUP_HANDLE               0x0040
#define PROCESS_CREATE_PROCESS           0x0080
#define PROCESS_SET_QUOTA                0x0100
#define PROCESS_SET_INFORMATION          0x0200
#define PROCESS_QUERY_INFORMATION        0x0400
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000

// Locale
#define LOCALE_USER_DEFAULT   0x0400
#define LOCALE_SYSTEM_DEFAULT 0x0800
#define LOCALE_INVARIANT      0x007F

#define LOCALE_NAME_SYSTEM_DEFAULT   nullptr
#define LOCALE_NAME_USER_DEFAULT     nullptr
#define LOCALE_NAME_INVARIANT        L""

// Locale info flags
#define LOCALE_IDEFAULTANSICODEPAGE 0x00001004
#define LOCALE_IDEFAULTCODEPAGE     0x0000000B
#define LOCALE_RETURN_NUMBER        0x20000000

// Code pages
#define CP_ACP        0
#define CP_OEMCP      1
#define CP_MACCP      2
#define CP_THREAD_ACP 3
#define CP_SYMBOL     42
#define CP_UTF7       65000
#define CP_UTF8       65001

// MAKELANGID / PRIMARYLANGID / SUBLANGID
#define MAKELANGID(p, s) ((((WORD)(s)) << 10) | (WORD)(p))
#define PRIMARYLANGID(lgid) ((WORD)(lgid) & 0x3ff)
#define SUBLANGID(lgid) ((WORD)(lgid) >> 10)

#define LANG_NEUTRAL        0x00
#define LANG_INVARIANT      0x7f
#define LANG_ENGLISH        0x09
#define SUBLANG_NEUTRAL     0x00
#define SUBLANG_DEFAULT     0x01
#define SUBLANG_SYS_DEFAULT 0x02

// MAKELCID
#define MAKELCID(lgid, srtid) ((DWORD)((((DWORD)((WORD)(srtid))) << 16) | ((DWORD)((WORD)(lgid)))))
#define SORT_DEFAULT 0x0

// Interlocked operations (using GCC/Clang builtins)
inline LONG InterlockedIncrement(volatile LONG* addend)
{
	return __sync_add_and_fetch(addend, 1);
}

inline LONG InterlockedDecrement(volatile LONG* addend)
{
	return __sync_sub_and_fetch(addend, 1);
}

inline LONG InterlockedExchange(volatile LONG* target, LONG value)
{
	return __sync_lock_test_and_set(target, value);
}

inline LONG InterlockedCompareExchange(volatile LONG* dest, LONG exchange, LONG comparand)
{
	return __sync_val_compare_and_swap(dest, comparand, exchange);
}

inline LONG InterlockedExchangeAdd(volatile LONG* addend, LONG value)
{
	return __sync_fetch_and_add(addend, value);
}

// CONTAINING_RECORD
#define CONTAINING_RECORD(address, type, field) \
	((type*)((char*)(address) - offsetof(type, field)))

// TEXT macro variants
#define __TEXT(quote) L##quote

// UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#define DBG_UNREFERENCED_PARAMETER(P) (void)(P)
#define DBG_UNREFERENCED_LOCAL_VARIABLE(V) (void)(V)

// C_ASSERT (compile-time assert)
#define C_ASSERT(e) static_assert(e, #e)

// PE image machine types
#define IMAGE_FILE_MACHINE_UNKNOWN 0
#define IMAGE_FILE_MACHINE_I386    0x014c
#define IMAGE_FILE_MACHINE_AMD64   0x8664
#define IMAGE_FILE_MACHINE_ARM64   0xAA64

// ARRAYSIZE
#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))
#endif
#ifndef _countof
#define _countof(a) ARRAYSIZE(a)
#endif

// Byte swap intrinsics (MSVC compatibility)
#include <libkern/OSByteOrder.h>
#ifndef _byteswap_ushort
#define _byteswap_ushort(x) OSSwapInt16(x)
#endif
#ifndef _byteswap_ulong
#define _byteswap_ulong(x)  OSSwapInt32(x)
#endif
#ifndef _byteswap_uint64
#define _byteswap_uint64(x) OSSwapInt64(x)
#endif

// IsTextUnicode flags
#define IS_TEXT_UNICODE_ASCII16             0x0001
#define IS_TEXT_UNICODE_REVERSE_ASCII16     0x0010
#define IS_TEXT_UNICODE_STATISTICS          0x0002
#define IS_TEXT_UNICODE_REVERSE_STATISTICS  0x0020
#define IS_TEXT_UNICODE_CONTROLS            0x0004
#define IS_TEXT_UNICODE_REVERSE_CONTROLS    0x0040
#define IS_TEXT_UNICODE_SIGNATURE           0x0008
#define IS_TEXT_UNICODE_REVERSE_SIGNATURE   0x0080
#define IS_TEXT_UNICODE_ILLEGAL_CHARS       0x0100
#define IS_TEXT_UNICODE_ODD_LENGTH          0x0200
#define IS_TEXT_UNICODE_NULL_BYTES          0x1000
#define IS_TEXT_UNICODE_UNICODE_MASK        0x000F
#define IS_TEXT_UNICODE_REVERSE_MASK        0x00F0
#define IS_TEXT_UNICODE_NOT_UNICODE_MASK    0x0F00
#define IS_TEXT_UNICODE_NOT_ASCII_MASK      0xF000

BOOL IsTextUnicode(const void* lpv, int iSize, LPINT lpiResult);

// ============================================================
// Security Identifiers (SID)
// ============================================================
typedef struct _SID_IDENTIFIER_AUTHORITY {
	BYTE Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;

typedef void* PSID;

#define SECURITY_NT_AUTHORITY {0,0,0,0,0,5}
#define SECURITY_BUILTIN_DOMAIN_RID 0x00000020L
#define DOMAIN_ALIAS_RID_ADMINS     0x00000220L

inline BOOL AllocateAndInitializeSid(
	PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
	BYTE nSubAuthorityCount,
	DWORD sa0, DWORD sa1, DWORD sa2, DWORD sa3,
	DWORD sa4, DWORD sa5, DWORD sa6, DWORD sa7,
	PSID* pSid)
{
	(void)pIdentifierAuthority; (void)nSubAuthorityCount;
	(void)sa0; (void)sa1; (void)sa2; (void)sa3;
	(void)sa4; (void)sa5; (void)sa6; (void)sa7;
	if (pSid) *pSid = nullptr;
	return FALSE;
}

inline BOOL CheckTokenMembership(HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember)
{
	(void)TokenHandle; (void)SidToCheck;
	if (IsMember) *IsMember = FALSE;
	return TRUE;
}

inline void* FreeSid(PSID pSid) { (void)pSid; return nullptr; }
