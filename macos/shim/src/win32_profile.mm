// win32_profile.mm — INI file (Private Profile) shim for macOS
// Implements GetPrivateProfileIntW, GetPrivateProfileStringW,
// WritePrivateProfileStringW using standard INI file parsing.

#import <Foundation/Foundation.h>
#include "windows.h"
#include <fstream>
#include <string>
#include <map>
#include <sstream>
#include <cwchar>

// Convert wchar_t* to NSString
static NSString* wideToNS(const wchar_t* w)
{
	if (!w) return @"";
	size_t len = wcslen(w);
	NSData* data = [NSData dataWithBytes:w length:len * sizeof(wchar_t)];
	return [[NSString alloc] initWithData:data encoding:NSUTF32LittleEndianStringEncoding] ?: @"";
}

// Convert wchar_t* to std::string (UTF-8)
static std::string wideToUtf8(const wchar_t* w)
{
	return [wideToNS(w) UTF8String] ?: "";
}

// Parse an INI file into section->key->value map
using IniMap = std::map<std::string, std::map<std::string, std::string>>;
static IniMap parseIniFile(const std::string& path)
{
	IniMap result;
	std::ifstream file(path);
	if (!file.is_open())
		return result;

	std::string currentSection;
	std::string line;
	while (std::getline(file, line))
	{
		// Trim whitespace
		size_t start = line.find_first_not_of(" \t\r\n");
		if (start == std::string::npos) continue;
		line = line.substr(start);
		size_t end = line.find_last_not_of(" \t\r\n");
		if (end != std::string::npos)
			line = line.substr(0, end + 1);

		if (line.empty() || line[0] == ';' || line[0] == '#')
			continue;

		if (line[0] == '[')
		{
			size_t close = line.find(']');
			if (close != std::string::npos)
				currentSection = line.substr(1, close - 1);
			continue;
		}

		size_t eq = line.find('=');
		if (eq != std::string::npos)
		{
			std::string key = line.substr(0, eq);
			std::string value = line.substr(eq + 1);
			// Trim key and value
			size_t ks = key.find_first_not_of(" \t");
			size_t ke = key.find_last_not_of(" \t");
			if (ks != std::string::npos && ke != std::string::npos)
				key = key.substr(ks, ke - ks + 1);
			size_t vs = value.find_first_not_of(" \t");
			size_t ve = value.find_last_not_of(" \t");
			if (vs != std::string::npos && ve != std::string::npos)
				value = value.substr(vs, ve - vs + 1);
			else
				value = "";
			result[currentSection][key] = value;
		}
	}
	return result;
}

// Write an INI map back to file
static bool writeIniFile(const std::string& path, const IniMap& ini)
{
	std::ofstream file(path);
	if (!file.is_open())
		return false;

	for (const auto& [section, keys] : ini)
	{
		file << "[" << section << "]\n";
		for (const auto& [key, value] : keys)
			file << key << "=" << value << "\n";
		file << "\n";
	}
	return true;
}

UINT GetPrivateProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName)
{
	@autoreleasepool {
		std::string path = wideToUtf8(lpFileName);
		std::string section = wideToUtf8(lpAppName);
		std::string key = wideToUtf8(lpKeyName);

		IniMap ini = parseIniFile(path);
		auto sit = ini.find(section);
		if (sit == ini.end())
			return static_cast<UINT>(nDefault);
		auto kit = sit->second.find(key);
		if (kit == sit->second.end())
			return static_cast<UINT>(nDefault);

		try { return static_cast<UINT>(std::stoi(kit->second)); }
		catch (...) { return static_cast<UINT>(nDefault); }
	}
}

DWORD GetPrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault,
                               LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName)
{
	@autoreleasepool {
		std::string path = wideToUtf8(lpFileName);
		std::string section = wideToUtf8(lpAppName);
		std::string key = wideToUtf8(lpKeyName);

		IniMap ini = parseIniFile(path);
		std::string value;

		auto sit = ini.find(section);
		if (sit != ini.end())
		{
			auto kit = sit->second.find(key);
			if (kit != sit->second.end())
				value = kit->second;
			else
				value = lpDefault ? wideToUtf8(lpDefault) : "";
		}
		else
		{
			value = lpDefault ? wideToUtf8(lpDefault) : "";
		}

		NSString* nsValue = [NSString stringWithUTF8String:value.c_str()];
		NSData* data = [nsValue dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
		size_t wcharCount = data.length / sizeof(wchar_t);
		size_t copyLen = (wcharCount < nSize) ? wcharCount : (nSize - 1);

		if (lpReturnedString && nSize > 0)
		{
			memcpy(lpReturnedString, data.bytes, copyLen * sizeof(wchar_t));
			lpReturnedString[copyLen] = L'\0';
		}

		return static_cast<DWORD>(copyLen);
	}
}

BOOL WritePrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName)
{
	@autoreleasepool {
		std::string path = wideToUtf8(lpFileName);
		std::string section = wideToUtf8(lpAppName);
		std::string key = wideToUtf8(lpKeyName);
		std::string value = lpString ? wideToUtf8(lpString) : "";

		IniMap ini = parseIniFile(path);
		ini[section][key] = value;

		return writeIniFile(path, ini) ? TRUE : FALSE;
	}
}
