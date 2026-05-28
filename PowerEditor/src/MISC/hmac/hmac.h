#pragma once

#include <windows.h>
#include <bcrypt.h>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

// Read MachineGuid from registry
static std::string getMachineGUID()
{
    char guid[64]{};
    DWORD size = sizeof(guid);
    HKEY hKey;
    if (::RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
        return "";

    ::RegQueryValueExA(hKey, "MachineGuid", nullptr, nullptr, (LPBYTE)guid, &size);
    ::RegCloseKey(hKey);
    return std::string(guid);
}

static std::string computeHMAC(const std::string& key, const std::string& data)
{
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    std::string result;

    if (!BCRYPT_SUCCESS(::BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG)))
        return "";

    DWORD hashLen = 0, cbResult = 0;
    ::BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PUCHAR)&hashLen, sizeof(DWORD), &cbResult, 0);

    if (!BCRYPT_SUCCESS(::BCryptCreateHash(hAlg, &hHash, nullptr, 0, (PUCHAR)key.data(), (ULONG)key.size(), 0)))
    {
        ::BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    ::BCryptHashData(hHash, (PUCHAR)data.data(), (ULONG)data.size(), 0);

    std::vector<BYTE> hash(hashLen);
    ::BCryptFinishHash(hHash, hash.data(), hashLen, 0);
    ::BCryptDestroyHash(hHash);
    ::BCryptCloseAlgorithmProvider(hAlg, 0);

    char hex[3];
    for (BYTE b : hash)
    {
        snprintf(hex, sizeof(hex), "%02x", b);
        result += hex;
    }
    return result;
}
