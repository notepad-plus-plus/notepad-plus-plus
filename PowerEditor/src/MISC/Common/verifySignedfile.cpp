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
	_gupSha256.push_back(L"1f72af0d9f108d99981f58837c26de16b46f6233ccd76ef560ba756094699404"); // v5.3.3 x64 bit (unsigned)
	_gupSha256.push_back(L"7a5068be842ed50d9857be29da2e27e7b0243f6ced3763d1ac4640a9cadc6ee7"); // v5.3.3 x86 bit (unsigned)
	_gupSha256.push_back(L"57f10b58d9492026d1bf74611a522da9ed05a682ae5ddeffe6c1c16ba839a89b"); // v5.3.3 arm64 bit (unsigned)

	_pluginListSha256.push_back(L"311a92116cf2ea649f87c6f05f4325d8b8370ca6b624ecf1174ec559859b203c"); // v1.8.4 x64 bit (unsigned)
	_pluginListSha256.push_back(L"c7253eaafb43d5d63356830122d27ae4f9b22b98e4656a195c20d5ae35d537f3"); // v1.8.4 x86 bit (unsigned)
	_pluginListSha256.push_back(L"2a684a000843f43d81096b5515f3386120c4256f369323db2def401e72e38792"); // v1.8.4 arm64 bit (unsigned)
}

bool SecurityGuard::checkModule([[maybe_unused]] const std::wstring& filePath, [[maybe_unused]] NppModule module2check)
{
#ifndef _DEBUG
	if (_securityMode == sm_certif)
		return verifySignedLibrary(filePath);
	else if (_securityMode == sm_sha256)
		return checkSha256(filePath, module2check);
	else
		return false;
#else
	// Do not check integrity if npp is running in debug mode
	// This is helpful for developers to skip signature checking
	// while analyzing issue or modifying the lexer dll
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
		wsprintf(sha2hashStr + i * 2, L"%02x", sha2hash[i]);

	const std::vector<std::wstring>* moduleSha256 = nullptr;

	if (module2check == nm_gup)
		moduleSha256 = &_gupSha256;
	else if (module2check == nm_pluginList)
		moduleSha256 = &_pluginListSha256;
	else
		return false;

	for (const auto& i : *moduleSha256)
	{
		if (i == sha2hashStr)
		{
			//::MessageBox(NULL, filePath.c_str(), L"OK", MB_OK);
			return true;
		}
	}

	//::MessageBox(NULL, filePath.c_str(), L"KO", MB_OK);
	return false;
}

// Debug use
bool doLogCertifError = false;

bool SecurityGuard::verifySignedLibrary(const std::wstring& filepath)
{
	wstring display_name;
	wstring key_id_hex;
	wstring subject;

	if (doLogCertifError)
	{	
		string dmsg("VerifyLibrary: ");
		dmsg += wstring2string(filepath, CP_UTF8);
		writeLog(L"c:\\tmp\\certifError.log", dmsg.c_str());
	}	

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

		if (doLogCertifError)
			writeLog(L"c:\\tmp\\certifError.log", "VerifyLibrary: certificate revocation checking is disabled");
	}
	else
	{
		// if offline, revocation is not checked
		// depending on windows version, this may introduce a latency on offline systems
		DWORD netstatus;
		QOCINFO oci;
		oci.dwSize = sizeof(oci);
		CONST wchar_t* msftTEXTest_site = L"http://www.msftncsi.com/ncsi.txt";
		bool online = false;
		online = (0 != IsNetworkAlive(&netstatus));
		online = online && (0 == GetLastError());
		online = online && (0 == IsDestinationReachable(msftTEXTest_site, &oci));
		if (!online)
		{
			winTEXTrust_data.fdwRevocationChecks = WTD_REVOKE_NONE;

			if (doLogCertifError)
				writeLog(L"c:\\tmp\\certifError.log", "VerifyLibrary: system is offline - certificate revocation won't be checked");
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
			if (doLogCertifError)
				writeLog(L"c:\\tmp\\certifError.log", "VerifyLibrary: trust verification failed");

			return false;
		}

		if (t2)
		{
			if (doLogCertifError)
				writeLog(L"c:\\tmp\\certifError.log", "VerifyLibrary: error encountered while cleaning up after WinVerifyTrust");

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
			throw string("Checking certificate of ") + wstring2string(filepath, CP_UTF8) + " : " + wstring2string(GetLastErrorAsString(GetLastError()), CP_UTF8);
		}

		// Get signer information size.
		result = ::CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &dwSignerInfo);
		if (!result)
		{
			throw string("CryptMsgGetParam first call: ") + wstring2string(GetLastErrorAsString(GetLastError()), CP_UTF8);
		}

		// Get Signer Information.
		pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
		if (NULL == pSignerInfo)
		{
			throw string("Failed to allocate memory for signature processing");
		}

		result = ::CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, (PVOID)pSignerInfo, &dwSignerInfo);
		if (!result)
		{
			throw string("CryptMsgGetParam: ") + wstring2string(GetLastErrorAsString(GetLastError()), CP_UTF8);
		}

		// Get the signer certificate from temporary certificate store.	
		CERT_INFO cert_info = {};
		cert_info.Issuer = pSignerInfo->Issuer;
		cert_info.SerialNumber = pSignerInfo->SerialNumber;
		PCCERT_CONTEXT context = ::CertFindCertificateInStore(hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, (PVOID)&cert_info, NULL);
		if (!context)
		{
			throw string("Certificate context: ") + wstring2string(GetLastErrorAsString(GetLastError()), CP_UTF8);
		}

		// Getting the full subject
		auto subject_sze = ::CertNameToStr(X509_ASN_ENCODING, &context->pCertInfo->Subject, CERT_X500_NAME_STR, NULL, 0);
		if (subject_sze <= 1)
		{
			throw string("Getting x509 field size problem.");
		}

		std::unique_ptr<wchar_t[]> subject_buffer(new wchar_t[subject_sze]);
		if (::CertNameToStr(X509_ASN_ENCODING, &context->pCertInfo->Subject, CERT_X500_NAME_STR, subject_buffer.get(), subject_sze) <= 1)
		{
			throw string("Failed to get x509 field infos from certificate.");
		}
		subject = subject_buffer.get();

		// Getting key_id 
		DWORD key_id_sze = 0;
		if (!::CertGetCertificateContextProperty(context, CERT_KEY_IDENTIFIER_PROP_ID, NULL, &key_id_sze))
		{
			throw string("x509 property not found") + wstring2string(GetLastErrorAsString(GetLastError()), CP_UTF8);
		}

		std::unique_ptr<BYTE[]> key_id_buff(new BYTE[key_id_sze]);
		if (!::CertGetCertificateContextProperty(context, CERT_KEY_IDENTIFIER_PROP_ID, key_id_buff.get(), &key_id_sze))
		{
			throw string("Getting certificate property problem.") + wstring2string(GetLastErrorAsString(GetLastError()), CP_UTF8);
		}

		wstringstream ss;
		for (unsigned i = 0; i < key_id_sze; i++)
		{
			ss << std::uppercase << std::setfill(wchar_t('0')) << std::setw(2) << std::hex
				<< key_id_buff[i];
		}
		key_id_hex = ss.str();

		if (doLogCertifError)
			writeLog(L"c:\\tmp\\certifError.log", wstring2string(key_id_hex, CP_UTF8).c_str());

		// Getting the display name			
		auto sze = ::CertGetNameString(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
		if (sze <= 1)
		{
			throw string("Getting data size problem.") + wstring2string(GetLastErrorAsString(GetLastError()), CP_UTF8);
		}

		// Get display name.
		std::unique_ptr<wchar_t[]> display_name_buffer(new wchar_t[sze]);
		if (::CertGetNameString(context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, display_name_buffer.get(), sze) <= 1)
		{
			throw string("Cannot get certificate info." + wstring2string(GetLastErrorAsString(GetLastError()), CP_UTF8));
		}
		display_name = display_name_buffer.get();

	}
	catch (const string& s) {
		if (doLogCertifError)
		{
			string msg = s;
			msg += " - VerifyLibrary: error while getting certificate information";
			writeLog(L"c:\\tmp\\certifError.log", msg.c_str());
		}
		status = false;
	}
	catch (...) {
		// Unknown error
		if (doLogCertifError)
			writeLog(L"c:\\tmp\\certifError.log", "VerifyLibrary: error while getting certificate information");

		status = false;
	}

	//
	// fields verifications - if status is true, and string to compare (from the parameter) is not empty, then do compare
	//
	if (status &&  (_signer_display_name != display_name))
	{
		status = false;

		if (doLogCertifError)
			writeLog(L"c:\\tmp\\certifError.log", "VerifyLibrary: Invalid certificate display name");
	}

	if (status && (_signer_subject != subject))
	{
		status = false;

		if (doLogCertifError)
			writeLog(L"c:\\tmp\\certifError.log", "VerifyLibrary: Invalid certificate subject");
	}

	if (status && (_signer_key_id != key_id_hex))
	{
		status = false;

		if (doLogCertifError)
			writeLog(L"c:\\tmp\\certifError.log", "VerifyLibrary: Invalid certificate key id");
	}

	// Clean up.

	if (hStore != NULL)       CertCloseStore(hStore, 0);
	if (hMsg != NULL)       CryptMsgClose(hMsg);
	if (pSignerInfo != NULL)  LocalFree(pSignerInfo);

	return status;
}
