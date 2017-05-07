

#pragma once

//#define VerifySignedLibrary_DISABLE_REVOCATION_CHECK "Dont check certificat revocation"

/*
* Verifies an Authenticde DLL signature and ownership
*
* Parameters:
*  @param filepath        path to the DLL file to examine
*  @param cert_display_name if specified, the signing certificate display name to compare to. Ignored if set to "", (weak comparison)
*  @param cert_subject    if specified, the full signing certificate subject name. Ignored if set to "" (strong comparison)
*  @param cert_key_id_hex if specified, the signing certificate key id (fingerprint), Ignored if set to "" (very strong comparison)
*
* @return true if the verification was positive, false if it was negative of encountered some error
*
* Dependencies:
*  This function uses 3 APIs: WinTrust, CryptoAPI, SENS API
*  It requires to link on : wintrust.lib, crypt32.lib (or crypt64.lib depending on the compilation target) and sensapi.lib
*  Those functions are available on Windows starting with Windows-XP
*
* Limitations:
*  Certificate revocation checking requires an access to Internet.
*  The functions checks for connectivity and will disable revocation checking if the machine is offline or if Microsoft
*  connectivity checking site is not reachable (supposely implying we are on an airgapped network).
*  Depending on Windows version, this test will be instantaneous (Windows 8 and up) or may take a few seconds.
*  This behaviour can be disabled by setting a define at compilation time.
*  If macro VerifySignedLibrary_DISABLE_REVOCATION_CHECK is defined, the revocation
*  state of the certificates will *not* be checked.
*
*/

#include <string>

bool VerifySignedLibrary(const std::wstring& filepath,
                         const std::wstring& key_id_hex,
                         const std::wstring& cert_subject,
                         const std::wstring& display_name);
