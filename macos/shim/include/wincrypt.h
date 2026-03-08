#pragma once
// Win32 Shim: WinCrypt API stubs for macOS
// On macOS, crypto operations use CommonCrypto or Security.framework.
// These stubs allow code that uses WinCrypt to compile.

#include "windef.h"

// ============================================================
// Crypto provider types and handles
// ============================================================
typedef ULONG_PTR HCRYPTPROV;
typedef ULONG_PTR HCRYPTHASH;
typedef ULONG_PTR HCRYPTKEY;

// Provider types
#define PROV_RSA_FULL    1
#define PROV_RSA_SIG     2
#define PROV_DSS         3
#define PROV_FORTEZZA    4
#define PROV_MS_EXCHANGE 5
#define PROV_SSL         6
#define PROV_RSA_AES     24

// CryptAcquireContext flags
#define CRYPT_VERIFYCONTEXT  0xF0000000
#define CRYPT_NEWKEYSET      0x00000008
#define CRYPT_DELETEKEYSET   0x00000010
#define CRYPT_MACHINE_KEYSET 0x00000020
#define CRYPT_SILENT         0x00000040

// Algorithm IDs
#define ALG_CLASS_HASH (4 << 13)
#define ALG_SID_MD5    3
#define ALG_SID_SHA1   4
#define ALG_SID_SHA_256 12
#define ALG_SID_SHA_384 13
#define ALG_SID_SHA_512 14

#define CALG_MD5     (ALG_CLASS_HASH | ALG_SID_MD5)
#define CALG_SHA1    (ALG_CLASS_HASH | ALG_SID_SHA1)
#define CALG_SHA_256 (ALG_CLASS_HASH | ALG_SID_SHA_256)
#define CALG_SHA_384 (ALG_CLASS_HASH | ALG_SID_SHA_384)
#define CALG_SHA_512 (ALG_CLASS_HASH | ALG_SID_SHA_512)

// Hash parameters
#define HP_HASHVAL  0x0002
#define HP_HASHSIZE 0x0004

// ============================================================
// Crypto function stubs
// ============================================================
inline BOOL CryptAcquireContextW(HCRYPTPROV* phProv, LPCWSTR szContainer,
                                  LPCWSTR szProvider, DWORD dwProvType, DWORD dwFlags)
{
	if (phProv) *phProv = 1; // non-zero = success token
	return TRUE;
}
inline BOOL CryptAcquireContextA(HCRYPTPROV* phProv, LPCSTR szContainer,
                                  LPCSTR szProvider, DWORD dwProvType, DWORD dwFlags)
{
	if (phProv) *phProv = 1;
	return TRUE;
}
#define CryptAcquireContext CryptAcquireContextW

inline BOOL CryptCreateHash(HCRYPTPROV hProv, DWORD Algid, HCRYPTKEY hKey, DWORD dwFlags, HCRYPTHASH* phHash)
{
	if (phHash) *phHash = 1;
	return TRUE;
}

inline BOOL CryptHashData(HCRYPTHASH hHash, const BYTE* pbData, DWORD dwDataLen, DWORD dwFlags)
{
	return TRUE;
}

inline BOOL CryptGetHashParam(HCRYPTHASH hHash, DWORD dwParam, BYTE* pbData, DWORD* pdwDataLen, DWORD dwFlags)
{
	// Stub: zero-fill the hash output
	if (pbData && pdwDataLen && *pdwDataLen > 0)
		memset(pbData, 0, *pdwDataLen);
	return TRUE;
}

inline BOOL CryptDestroyHash(HCRYPTHASH hHash) { return TRUE; }
inline BOOL CryptReleaseContext(HCRYPTPROV hProv, DWORD dwFlags) { return TRUE; }
