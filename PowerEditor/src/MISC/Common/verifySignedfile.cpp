// This file is part of Notepad++ project
// Copyright (C)2003-2017 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid
// misunderstandings, we consider an application to constitute a
// "derivative work" for the purpose of this license if it does any of the
// following:
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


// VerifyDLL.cpp : Verification of an Authenticode signed DLL
//

#include <memory>
#include <windows.h>
#include <wintrust.h>
#include <softpub.h>
#include <wincrypt.h>
#include <sensapi.h>
#include <iomanip>
#include "VerifySignedFile.h"
#include "Common.h"

using namespace std;

bool VerifySignedLibrary(const wstring& filepath,
                         const wstring& cert_key_id_hex,
                         const wstring& cert_subject,
                         const wstring& cert_display_name,
                         bool doCheckRevocation,
                         bool doCheckChainOfTrust)
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
	WINTRUST_FILE_INFO file_data = { 0 };
	file_data.cbStruct = sizeof(WINTRUST_FILE_INFO);
	file_data.pcwszFilePath = pwszfilepath;

	// Initialise WinTrust data	
	WINTRUST_DATA winTEXTrust_data = { 0 };
	winTEXTrust_data.cbStruct = sizeof(winTEXTrust_data);
	winTEXTrust_data.dwUIChoice          = WTD_UI_NONE;	         // do not display optional dialog boxes
	winTEXTrust_data.dwUnionChoice       = WTD_CHOICE_FILE;        // we are not checking catalog signed files
	winTEXTrust_data.dwStateAction       = WTD_STATEACTION_VERIFY; // only checking
	winTEXTrust_data.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;  // verify the whole certificate chain
	winTEXTrust_data.pFile               = &file_data;

	if (!doCheckRevocation)
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

	if (doCheckChainOfTrust)
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
	HCERTSTORE        hStore      = nullptr;
	HCRYPTMSG         hMsg        = nullptr;
	PCMSG_SIGNER_INFO pSignerInfo = nullptr;
	DWORD dwEncoding, dwContentType, dwFormatType;
	DWORD dwSignerInfo = 0L;
	bool status        = true;

	try {
		BOOL result = ::CryptQueryObject(CERT_QUERY_OBJECT_FILE, filepath.c_str(),
										 CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED, CERT_QUERY_FORMAT_FLAG_BINARY, 0,
										 &dwEncoding, &dwContentType, &dwFormatType,
										 &hStore, &hMsg, NULL);

		if (!result)
		{
			throw wstring( TEXT("Checking certificate of ") ) + filepath + TEXT(" : ") + GetLastErrorAsString(GetLastError());
		}

		// Get signer information size.
		result = ::CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &dwSignerInfo);
		if (!result)
		{			
			throw wstring( TEXT("CryptMsgGetParam first call: ")) + GetLastErrorAsString(GetLastError());
		}

		// Get Signer Information.
		pSignerInfo = (PCMSG_SIGNER_INFO)LocalAlloc(LPTR, dwSignerInfo);
		if (NULL == pSignerInfo )
		{
			throw wstring( TEXT("Failed to allocate memory for signature processing"));
		}

		result = ::CryptMsgGetParam(hMsg, CMSG_SIGNER_INFO_PARAM, 0, (PVOID)pSignerInfo, &dwSignerInfo);
		if (!result)
		{
			throw wstring( TEXT("CryptMsgGetParam: ")) + GetLastErrorAsString(GetLastError());
		}

		// Get the signer certificate from temporary certificate store.	
		CERT_INFO cert_info = { 0 };
		cert_info.Issuer       = pSignerInfo->Issuer;
		cert_info.SerialNumber = pSignerInfo->SerialNumber;
		PCCERT_CONTEXT context = ::CertFindCertificateInStore( hStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, (PVOID)&cert_info, NULL);
		if (!context)
		{
			throw wstring( TEXT("Certificate context: ")) + GetLastErrorAsString(GetLastError());
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
		if (!::CertGetCertificateContextProperty( context, CERT_KEY_IDENTIFIER_PROP_ID, NULL, &key_id_sze))
		{
			throw wstring( TEXT("x509 property not found")) + GetLastErrorAsString(GetLastError());
		}

		std::unique_ptr<BYTE[]> key_id_buff( new BYTE[key_id_sze] );
		if (!::CertGetCertificateContextProperty( context, CERT_KEY_IDENTIFIER_PROP_ID, key_id_buff.get(), &key_id_sze))
		{
			throw wstring( TEXT("Getting certificate property problem.")) + GetLastErrorAsString(GetLastError());
		}

		wstringstream ss;
		for (unsigned i = 0; i < key_id_sze; i++)
		{
			ss << std::uppercase << std::setfill(TCHAR('0')) << std::setw(2) << std::hex 
			   << key_id_buff[i];
		}
		key_id_hex = ss.str();
		wstring dbg = key_id_hex + TEXT("\n");
		OutputDebugString( dbg.c_str() );

		// Getting the display name			
		auto sze = ::CertGetNameString( context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
		if (sze <= 1)
		{
			throw wstring( TEXT("Getting data size problem.")) + GetLastErrorAsString(GetLastError());
		}
		
		// Get display name.
		std::unique_ptr<TCHAR[]> display_name_buffer( new TCHAR[sze] );
		if (::CertGetNameString( context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, display_name_buffer.get(), sze) <= 1)
		{
			throw wstring( TEXT("Cannot get certificate info.")) + GetLastErrorAsString(GetLastError());
		}
		display_name = display_name_buffer.get();

	} catch (const wstring& s) {
		::MessageBox(NULL, s.c_str(), TEXT("DLL signature verification failed"), MB_ICONERROR);
		OutputDebugString(TEXT("VerifyLibrary: error while getting certificate informations\n"));
		status = false;
	} catch (...) {
		// Unknown error
		OutputDebugString(TEXT("VerifyLibrary: error while getting certificate informations\n"));
		wstring errMsg(TEXT("Unknown exception occurred. "));
		errMsg += GetLastErrorAsString(GetLastError());
		::MessageBox(NULL, errMsg.c_str(), TEXT("DLL signature verification failed"), MB_ICONERROR);
		status = false;
	}

	//
	// fields verifications - if status is true, and string to compare (from the parameter) is not empty, then do compare
	//
	if ( status && !cert_display_name.empty() && cert_display_name != display_name )
	{
		status = false;
		OutputDebugString(TEXT("VerifyLibrary: Invalid certificate display name\n"));
	}

	if ( status && !cert_subject.empty() && cert_subject != subject)
	{
		status = false;
		OutputDebugString(TEXT("VerifyLibrary: Invalid certificate subject\n"));
	}

	if ( status && !cert_key_id_hex.empty() && cert_key_id_hex != key_id_hex )
	{
		status = false;
		OutputDebugString(TEXT("VerifyLibrary: Invalid certificate key id\n"));
	}

	// Clean up.
	
	if (hStore != NULL)       CertCloseStore(hStore, 0);
	if (hMsg   != NULL)       CryptMsgClose(hMsg);
	if (pSignerInfo != NULL)  LocalFree(pSignerInfo);

	return status;
}

#undef VerifySignedLibrary_DISABLE_REVOCATION_CHECK
