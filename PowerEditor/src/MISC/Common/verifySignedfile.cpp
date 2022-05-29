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


// VerifyDLL.cpp : Verification of an Authenticode signed DLL
//

#include <memory>
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#include <sensapi.h>
#include <iomanip>
#include "verifySignedfile.h"
#include "Common.h"
#include "sha-256.h"

using namespace std;

//SecurityMode SecurityGuard::_securityMode = sm_sha256;
SecurityMode SecurityGuard::_securityMode = sm_certif;

SecurityGuard::SecurityGuard()
{
	_scilexerSha256.push_back(TEXT("03c9177631d2b32de3d32c73a8841cf68fc2cb17f306825489dc3df98000db85")); // v3.5.6 32 bit (signed)
	_scilexerSha256.push_back(TEXT("9896c4089275e21412fd80421827912ebd80e357394b05145a613d190462e211")); // v3.5.6 64 bit (signed)

	_gupSha256.push_back(TEXT("4c8191f511c2ad67148ef809b40c1108aaa074130547157c335a959404d8d6f6")); // v5.1 32 bit (signed)
	_gupSha256.push_back(TEXT("268a65829e86d5c3d324eea79b51e59f0a7d07c69d3ba0f700c9cb3aa772566f")); // v5.1 64 bit (signed)

	_pluginListSha256.push_back(TEXT("be9e251a30fd712fd2ff98febd360805df51110b6659de8c9a0000220d7ae535")); // v1.0.7 32 bit (unsigned)
	_pluginListSha256.push_back(TEXT("3ecd7f9c56bcd659a4126c659eb69b354789c78574a82390749ac751ae539bc6")); // v1.0.7 64 bit (unsigned)

	_pluginListSha256.push_back(TEXT("a4a7e57d605f29b294378d0d94fc867b9febd6a1cc63f1bb69bcb7609dc25f2c")); // v1.0.8 32 bit (unsigned)
	_pluginListSha256.push_back(TEXT("1c404fd3578273f5ecde585af82179ff3b63c635fb4fa24be21ebde708e403e4")); // v1.0.8 64 bit (unsigned)
}

bool SecurityGuard::checkModule(const std::wstring& filePath, NppModule module2check)
{
#ifndef _DEBUG
	if (_securityMode == sm_certif)
		return verifySignedLibrary(filePath, module2check);
	else if (_securityMode == sm_sha256)
		return checkSha256(filePath, module2check);
	else
		return false;
#else
	// Do not check integrity if npp is running in debug mode
	// This is helpful for developers to skip signature checking
	// while analyzing issue or modifying the lexer dll
	(void)filePath;
	(void)module2check;
	return true;
#endif
}

bool SecurityGuard::checkSha256(const std::wstring& filePath, NppModule module2check)
{
	// Uncomment the following code if the components are rebuilt for testing
	// It should be stay in commenting out
	/*
	bool dontCheck = true;
	if (dontCheck)
		return true;
	*/

	std::string content = getFileContent(filePath.c_str());
	uint8_t sha2hash[32];
	calc_sha_256(sha2hash, reinterpret_cast<const uint8_t*>(content.c_str()), content.length());

	wchar_t sha2hashStr[65] = { '\0' };
	for (size_t i = 0; i < 32; i++)
		wsprintf(sha2hashStr + i * 2, TEXT("%02x"), sha2hash[i]);

	std::vector<std::wstring>* moduleSha256 = nullptr;
	if (module2check == nm_scilexer)
		moduleSha256 = &_scilexerSha256;
	else if (module2check == nm_gup)
		moduleSha256 = &_gupSha256;
	else if (module2check == nm_pluginList)
		moduleSha256 = &_pluginListSha256;
	else
		return false;

	for (auto i : *moduleSha256)
	{
		if (i == sha2hashStr)
		{
			//::MessageBox(NULL, filePath.c_str(), TEXT("OK"), MB_OK);
			return true;
		}
	}

	//::MessageBox(NULL, filePath.c_str(), TEXT("KO"), MB_OK);
	return false;
}

bool SecurityGuard::verifySignedLibrary(const std::wstring& filepath, NppModule module2check)
{
	wstring display_name;
	wstring key_id_hex;
	wstring subject;

	wstring dmsg(TEXT("VerifyLibrary: "));
	dmsg += filepath;
	dmsg += TEXT("\n");

	OutputDebugString(dmsg.c_str());

	//
	// Signature verification
	//

	// Initialize the WINTRUST_FILE_INFO structure.
	LPCWSTR pwszfilepath = filepath.c_str();
	WINTRUST_FILE_INFO file_data = {};
	file_data.cbStruct = sizeof(WINTRUST_FILE_INFO);
	file_data.pcwszFilePath = pwszfilepath;

	// Initialise WinTrust data	
	WINTRUST_DATA winTEXTrust_data = {};
	winTEXTrust_data.cbStruct = sizeof(winTEXTrust_data);
	winTEXTrust_data.dwUIChoice = WTD_UI_NONE;	         // do not display optional dialog boxes
	winTEXTrust_data.dwUnionChoice = WTD_CHOICE_FILE;        // we are not checking catalog signed files
	winTEXTrust_data.dwStateAction = WTD_STATEACTION_VERIFY; // only checking
	winTEXTrust_data.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;  // verify the whole certificate chain
	winTEXTrust_data.pFile = &file_data;

	if (!_doCheckRevocation)
	{
		winTEXTrust_data.fdwRevocationChecks = WTD_REVOKE_NONE;
		OutputDebugString(TEXT("VerifyLibrary: certificate revocation checking is disabled\n"));
	}
	else
	{
		// if offline, revocation is not checked
		// depending of windows version, this may introduce a latency on offline systems
		DWORD netstatus;
		QOCINFO oci;
		oci.dwSize = sizeof(oci);
		CONST TCHAR* msftTEXTest_site = TEXT("http://www.msftncsi.com/ncsi.txt");
		bool online = false;
		online = (0 != IsNetworkAlive(&netstatus));
		online = online && (0 == GetLastError());
		online = online && (0 == IsDestinationReachable(msftTEXTest_site, &oci));
		if (!online)
		{
			winTEXTrust_data.fdwRevocationChecks = WTD_REVOKE_NONE;
			OutputDebugString(TEXT("VerifyLibrary: system is offline - certificate revocation wont be checked\n"));
		}
	}

	if (_doCheckChainOfTrust)
	{
		// Verify signature and cert-chain validity
		GUID policy = WINTRUST_ACTION_GENERIC_VERIFY_V2;
		LONG vtrust = ::WinVerifyTrust(NULL, &policy, &winTEXTrust_data);

		// Post check cleanup
		winTEXTrust_data.dwStateAction = WTD_STATEACTION_CLOSE;
		LONG t2 = ::WinVerifyTrust(NULL, &policy, &winTEXTrust_data);

		if (vtrust)
		{
			OutputDebugString(TEXT("VerifyLibrary: trust verification failed\n"));
			return false;
		}

		if (t2)
		{
			OutputDebugString(TEXT("VerifyLibrary: error encountered while cleaning up after WinVerifyTrust\n"));
			return false;
		}
	}

	//
	// Certificate verification
	//
	HCERTSTORE        hStore = nullptr;
	HCRYPTMSG         hMsg = nullptr;
	PCMSG_SIGNER_INFO pSignerInfo = nullptr;
	DWORD dwEncoding, dwContentType, dwFormatType;
	DWORD dwSignerInfo = 0L;
	bool status = true;

	try {
		BOOL result = ::CryptQueryObject(CERT_QUERY_OBJECT_FILE, filepath.c_str(),
			CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED, CERT_QUERY_FORMAT_FLAG_BINARY, 0,
			&dwEncoding, &dwContentType, &dwFormatType,
			&hStore, &hMsg, NULL);

		if (!result)
		{
			throw wstring(TEXT("Checking certificate of ")) + filepath + TEXT(" : ") + GetLastErrorAsString(GetLastError());
		}

		// Get signer information size.
		result = ::CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &dwSignerInfo);
		if (!result)
		{
			throw wstring(TEXT("CryptMsgGetParam first call: ")) + GetLastErrorAsString(GetLastError());
		}

		// Get Signer Information.
		pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
		if (NULL == pSignerInfo)
		{
			throw wstring(TEXT("Failed to allocate memory for signature processing"));
		}

		result = ::CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, (PVOID)pSignerInfo, &dwSignerInfo);
		if (!result)
		{
			throw wstring(TEXT("CryptMsgGetParam: ")) + GetLastErrorAsString(GetLastError());
		}

		// Get the signer certificate from temporary certificate store.	
		CERT_INFO cert_info = {};
		cert_info.Issuer = pSignerInfo->Issuer;
		cert_info.SerialNumber = pSignerInfo->SerialNumber;
		PCCERT_CONTEXT context = ::CertFindCertificateInStore(hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, (PVOID)&cert_info, NULL);
		if (!context)
		{
			throw wstring(TEXT("Certificate context: ")) + GetLastErrorAsString(GetLastError());
		}

		// Getting the full subject				
		auto subject_sze = ::CertNameToStr(X509_ASN_ENCODING, &context->pCertInfo->Subject, CERT_X500_NAME_STR, NULL, 0);
		if (subject_sze <= 1)
		{
			throw wstring(TEXT("Getting x509 field size problem."));
		}

		std::unique_ptr<TCHAR[]> subject_buffer(new TCHAR[subject_sze]);
		if (::CertNameToStr(X509_ASN_ENCODING, &context->pCertInfo->Subject, CERT_X500_NAME_STR, subject_buffer.get(), subject_sze) <= 1)
		{
			throw wstring(TEXT("Failed to get x509 filed infos from certificate."));
		}
		subject = subject_buffer.get();

		// Getting key_id 
		DWORD key_id_sze = 0;
		if (!::CertGetCertificateContextProperty(context, CERT_KEY_IDENTIFIER_PROP_ID, NULL, &key_id_sze))
		{
			throw wstring(TEXT("x509 property not found")) + GetLastErrorAsString(GetLastError());
		}

		std::unique_ptr<BYTE[]> key_id_buff(new BYTE[key_id_sze]);
		if (!::CertGetCertificateContextProperty(context, CERT_KEY_IDENTIFIER_PROP_ID, key_id_buff.get(), &key_id_sze))
		{
			throw wstring(TEXT("Getting certificate property problem.")) + GetLastErrorAsString(GetLastError());
		}

		wstringstream ss;
		for (unsigned i = 0; i < key_id_sze; i++)
		{
			ss << std::uppercase << std::setfill(TCHAR('0')) << std::setw(2) << std::hex
				<< key_id_buff[i];
		}
		key_id_hex = ss.str();
		wstring dbg = key_id_hex + TEXT("\n");
		OutputDebugString(dbg.c_str());

		// Getting the display name			
		auto sze = ::CertGetNameString(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
		if (sze <= 1)
		{
			throw wstring(TEXT("Getting data size problem.")) + GetLastErrorAsString(GetLastError());
		}

		// Get display name.
		std::unique_ptr<TCHAR[]> display_name_buffer(new TCHAR[sze]);
		if (::CertGetNameString(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, display_name_buffer.get(), sze) <= 1)
		{
			throw wstring(TEXT("Cannot get certificate info.")) + GetLastErrorAsString(GetLastError());
		}
		display_name = display_name_buffer.get();

	}
	catch (const wstring& s) {
		if (module2check == nm_scilexer)
			::MessageBox(NULL, s.c_str(), TEXT("DLL signature verification failed"), MB_ICONERROR);
		OutputDebugString(TEXT("VerifyLibrary: error while getting certificate informations\n"));
		status = false;
	}
	catch (...) {
		// Unknown error
		OutputDebugString(TEXT("VerifyLibrary: error while getting certificate informations\n"));
		if (module2check == nm_scilexer)
		{
			wstring errMsg(TEXT("Unknown exception occurred. "));
			errMsg += GetLastErrorAsString(GetLastError());
			::MessageBox(NULL, errMsg.c_str(), TEXT("DLL signature verification failed"), MB_ICONERROR);
		}
		status = false;
	}

	//
	// fields verifications - if status is true, and string to compare (from the parameter) is not empty, then do compare
	//
	if (status &&  _signer_display_name != display_name)
	{
		status = false;
		OutputDebugString(TEXT("VerifyLibrary: Invalid certificate display name\n"));
	}

	if (status && _signer_subject != subject)
	{
		status = false;
		OutputDebugString(TEXT("VerifyLibrary: Invalid certificate subject\n"));
	}

	if (status && _signer_key_id != key_id_hex)
	{
		status = false;
		OutputDebugString(TEXT("VerifyLibrary: Invalid certificate key id\n"));
	}

	// Clean up.

	if (hStore != NULL)       CertCloseStore(hStore, 0);
	if (hMsg != NULL)       CryptMsgClose(hMsg);
	if (pSignerInfo != NULL)  LocalFree(pSignerInfo);

	return status;
}
