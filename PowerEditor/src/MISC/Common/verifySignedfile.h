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
#pragma once

#include <string>
#include <vector>

enum SecurityMode { sm_certif = 0, sm_sha256 = 1 };
enum NppModule { nm_scilexer = 0, nm_gup = 1, nm_pluginList = 2 };

class SecurityGuard final
{
public:
	SecurityGuard();
	bool checkModule(const std::wstring& filePath, NppModule module2check);

private:
	// SHA256
	static SecurityMode _securityMode;
	std::vector<std::wstring> _scilexerSha256;
	std::vector<std::wstring> _gupSha256;
	std::vector<std::wstring> _pluginListSha256;

	bool checkSha256(const std::wstring& filePath, NppModule module2check);

	// Code signing certificate
	std::wstring _signer_display_name = TEXT("Notepad++");
	std::wstring _signer_subject = TEXT("C=FR, S=Ile-de-France, L=Saint Cloud, O=\"Notepad++\", CN=\"Notepad++\"");
	std::wstring _signer_key_id = TEXT("E687332916D6B681FE28C5EF423CEE259D3953B9");
	bool _doCheckRevocation = false;
	bool _doCheckChainOfTrust = false;

	bool verifySignedLibrary(const std::wstring& filepath, NppModule module2check);
};

