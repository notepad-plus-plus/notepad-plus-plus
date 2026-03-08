#pragma once
// Win32 Shim: SensAPI stub for macOS
// Network connectivity sensing

#include "windef.h"

#define NETWORK_ALIVE_LAN  0x00000001
#define NETWORK_ALIVE_WAN  0x00000002
#define NETWORK_ALIVE_AOL  0x00000004

typedef struct _QOCINFO {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwInSpeed;
	DWORD dwOutSpeed;
} QOCINFO, *LPQOCINFO;

inline BOOL IsNetworkAlive(LPDWORD lpdwFlags)
{
	if (lpdwFlags) *lpdwFlags = NETWORK_ALIVE_LAN;
	return TRUE; // assume network available
}

inline BOOL IsDestinationReachableW(LPCWSTR lpszDestination, LPVOID lpQOCInfo)
{
	return TRUE;
}
#define IsDestinationReachable IsDestinationReachableW
