#pragma once
// Win32 Shim: WinTrust API stub for macOS
// Code signing verification is not applicable on macOS (use Security.framework)

#include "windef.h"

// Stub types
typedef void* HCATADMIN;
typedef void* HCATINFO;

// WINTRUST_DATA (simplified stub)
typedef struct _WINTRUST_DATA {
	DWORD  cbStruct;
	LPVOID pPolicyCallbackData;
	LPVOID pSIPClientData;
	DWORD  dwUIChoice;
	DWORD  fdwRevocationChecks;
	DWORD  dwUnionChoice;
	union {
		LPVOID pFile;
		LPVOID pCatalog;
		LPVOID pBlob;
		LPVOID pSgnr;
		LPVOID pCert;
	};
	DWORD  dwStateAction;
	HANDLE hWVTStateData;
	LPWSTR pwszURLReference;
	DWORD  dwProvFlags;
	DWORD  dwUIContext;
} WINTRUST_DATA, *PWINTRUST_DATA;

typedef struct _WINTRUST_FILE_INFO {
	DWORD   cbStruct;
	LPCWSTR pcwszFilePath;
	HANDLE  hFile;
	GUID*   pgKnownSubject;
} WINTRUST_FILE_INFO, *PWINTRUST_FILE_INFO;

#define WTD_UI_ALL    1
#define WTD_UI_NONE   2
#define WTD_UI_NOBAD  3
#define WTD_UI_NOGOOD 4

#define WTD_CHOICE_FILE    1
#define WTD_CHOICE_CATALOG 2
#define WTD_CHOICE_BLOB    3
#define WTD_CHOICE_SIGNER  4
#define WTD_CHOICE_CERT    5

#define WTD_REVOKE_NONE       0x00000000
#define WTD_REVOKE_WHOLECHAIN 0x00000001

#define WTD_STATEACTION_IGNORE           0x00000000
#define WTD_STATEACTION_VERIFY           0x00000001
#define WTD_STATEACTION_CLOSE            0x00000002
#define WTD_STATEACTION_AUTO_CACHE       0x00000003
#define WTD_STATEACTION_AUTO_CACHE_FLUSH 0x00000004

#define WINTRUST_ACTION_GENERIC_VERIFY_V2 \
	{0xaac56b, 0xcd44, 0x11d0, {0x8c, 0xc2, 0x0, 0xc0, 0x4f, 0xc2, 0x95, 0xee}}

// Always return "not trusted" on macOS
inline LONG WinVerifyTrust(HWND hwnd, GUID* pgActionID, LPVOID pWVTData)
{
	return 0x800B0100L; // TRUST_E_NOSIGNATURE
}

// ============================================================
// WinCrypt stubs (certificate/message types used by verifySignedfile.cpp)
// ============================================================
typedef void* HCERTSTORE;
typedef void* HCRYPTMSG;

// CRYPT_DATA_BLOB / CERT_NAME_BLOB
typedef struct _CRYPTOAPI_BLOB {
	DWORD cbData;
	BYTE* pbData;
} CRYPT_DATA_BLOB, CERT_NAME_BLOB, CRYPT_INTEGER_BLOB, CRYPT_OBJID_BLOB,
  CRYPT_HASH_BLOB, CRYPT_ATTR_BLOB;

typedef struct _CRYPT_ALGORITHM_IDENTIFIER {
	LPSTR            pszObjId;
	CRYPT_OBJID_BLOB Parameters;
} CRYPT_ALGORITHM_IDENTIFIER;

typedef struct _CRYPT_BIT_BLOB {
	DWORD cbData;
	BYTE* pbData;
	DWORD cUnusedBits;
} CRYPT_BIT_BLOB;

typedef struct _CERT_INFO {
	DWORD                      dwVersion;
	CRYPT_INTEGER_BLOB         SerialNumber;
	CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
	CERT_NAME_BLOB             Issuer;
	CERT_NAME_BLOB             Subject;
} CERT_INFO, *PCERT_INFO;

typedef struct _CERT_CONTEXT {
	DWORD      dwCertEncodingType;
	BYTE*      pbCertEncoded;
	DWORD      cbCertEncoded;
	PCERT_INFO pCertInfo;
	HCERTSTORE hCertStore;
} CERT_CONTEXT, *PCERT_CONTEXT;
typedef const CERT_CONTEXT* PCCERT_CONTEXT;

typedef struct _CMSG_SIGNER_INFO {
	DWORD              dwVersion;
	CERT_NAME_BLOB     Issuer;
	CRYPT_INTEGER_BLOB SerialNumber;
} CMSG_SIGNER_INFO, *PCMSG_SIGNER_INFO;

#define CERT_QUERY_OBJECT_FILE  1
#define CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED 0x00000400
#define CERT_QUERY_FORMAT_FLAG_BINARY 0x00000002
#define CMSG_SIGNER_INFO_PARAM 6
#define CERT_FIND_SUBJECT_CERT 11
#define X509_ASN_ENCODING  0x00000001
#define PKCS_7_ASN_ENCODING 0x00010000
#define CERT_NAME_SIMPLE_DISPLAY_TYPE 4
#define CERT_NAME_ISSUER_FLAG 0x1
#define CERT_X500_NAME_STR 3
#define CERT_KEY_IDENTIFIER_PROP_ID 20

// Crypto function stubs
inline BOOL CryptQueryObject(DWORD dwObjectType, const void* pvObject, DWORD dwExpectedContentTypeFlags,
                             DWORD dwExpectedFormatTypeFlags, DWORD dwFlags,
                             DWORD* pdwMsgAndCertEncodingType, DWORD* pdwContentType,
                             DWORD* pdwFormatType, HCERTSTORE* phCertStore,
                             HCRYPTMSG* phMsg, const void** ppvContext)
{
	return FALSE;
}

inline BOOL CryptMsgGetParam(HCRYPTMSG hCryptMsg, DWORD dwParamType, DWORD dwIndex, void* pvData, DWORD* pcbData)
{
	return FALSE;
}

inline PCCERT_CONTEXT CertFindCertificateInStore(HCERTSTORE hCertStore, DWORD dwCertEncodingType,
                                                  DWORD dwFindFlags, DWORD dwFindType,
                                                  const void* pvFindPara, PCCERT_CONTEXT pPrevCertContext)
{
	return nullptr;
}

inline DWORD CertGetNameStringW(PCCERT_CONTEXT pCertContext, DWORD dwType, DWORD dwFlags,
                                void* pvTypePara, LPWSTR pszNameString, DWORD cchNameString)
{
	if (pszNameString && cchNameString > 0) pszNameString[0] = L'\0';
	return 0;
}
#define CertGetNameString CertGetNameStringW

inline DWORD CertNameToStrW(DWORD dwCertEncodingType, CERT_NAME_BLOB* pName, DWORD dwStrType,
                             LPWSTR psz, DWORD csz)
{
	if (psz && csz > 0) psz[0] = L'\0';
	return 1; // returns length including null
}
#define CertNameToStr CertNameToStrW

inline BOOL CertGetCertificateContextProperty(PCCERT_CONTEXT pCertContext, DWORD dwPropId,
                                               void* pvData, DWORD* pcbData)
{
	if (pcbData) *pcbData = 0;
	return FALSE;
}

inline BOOL CertCloseStore(HCERTSTORE hCertStore, DWORD dwFlags) { return TRUE; }
inline BOOL CryptMsgClose(HCRYPTMSG hCryptMsg) { return TRUE; }
inline BOOL CertFreeCertificateContext(PCCERT_CONTEXT pCertContext) { return TRUE; }
