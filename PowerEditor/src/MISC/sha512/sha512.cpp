// This file is part of Notepad++ project
// Copyright (C)2023 Don HO <don.h@free.fr>

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

#include <windows.h>
#include <wincrypt.h>
#include "sha512.h"

//#if defined(_MSC_VER)
//#pragma comment(lib, "crypt32.lib")
//#endif

void calc_sha_512(unsigned char hash[64], const void *input, size_t len) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD dwHashLen = 64;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        //std::cerr << "CryptAcquireContext failed: " << GetLastError() << std::endl;
        return;
    }

    if (!CryptCreateHash(hProv, CALG_SHA_512, 0, 0, &hHash)) {
        //std::cerr << "CryptCreateHash failed: " << GetLastError() << std::endl;
        CryptReleaseContext(hProv, 0);
        return;
    }

    if (!CryptHashData(hHash, (BYTE*)input, DWORD(len), 0)) {
        //std::cerr << "CryptHashData failed: " << GetLastError() << std::endl;
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &dwHashLen, 0)) {
        //std::cerr << "CryptGetHashParam failed: " << GetLastError() << std::endl;
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return;
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
}

