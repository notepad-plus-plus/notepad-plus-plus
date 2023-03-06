// This file contains code from
// https://github.com/stevemk14ebr/PolyHook_2_0/blob/master/sources/IatHook.cpp
// which is licensed under the MIT License.
// See PolyHook_2_0-LICENSE for more information.

#pragma once

#include <stdint.h>

template <typename T, typename T1, typename T2>
constexpr T RVA2VA(T1 base, T2 rva)
{
	return reinterpret_cast<T>(reinterpret_cast<ULONG_PTR>(base) + rva);
}

template <typename T>
constexpr T DataDirectoryFromModuleBase(void *moduleBase, size_t entryID)
{
	auto dosHdr = static_cast<PIMAGE_DOS_HEADER>(moduleBase);
	auto ntHdr = RVA2VA<PIMAGE_NT_HEADERS>(moduleBase, dosHdr->e_lfanew);
	auto dataDir = ntHdr->OptionalHeader.DataDirectory;
	return RVA2VA<T>(moduleBase, dataDir[entryID].VirtualAddress);
}

PIMAGE_THUNK_DATA FindAddressByName(void *moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, const char *funcName)
{
	for (; impName->u1.Ordinal; ++impName, ++impAddr)
	{
		if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal))
			continue;

		auto import = RVA2VA<PIMAGE_IMPORT_BY_NAME>(moduleBase, impName->u1.AddressOfData);
		if (strcmp(reinterpret_cast<const char *>(import->Name), funcName) != 0)
			continue;
		return impAddr;
	}
	return nullptr;
}

PIMAGE_THUNK_DATA FindAddressByOrdinal(void *moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, uint16_t ordinal)
{
	UNREFERENCED_PARAMETER(moduleBase);
	for (; impName->u1.Ordinal; ++impName, ++impAddr)
	{
		if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal)
			return impAddr;
	}
	return nullptr;
}

PIMAGE_THUNK_DATA FindIatThunkInModule(void *moduleBase, const char *dllName, const char *funcName)
{
	auto imports = DataDirectoryFromModuleBase<PIMAGE_IMPORT_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_IMPORT);
	for (; imports->Name; ++imports)
	{
		if (_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->Name), dllName) != 0)
			continue;

		auto origThunk = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->OriginalFirstThunk);
		auto thunk = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->FirstThunk);
		return FindAddressByName(moduleBase, origThunk, thunk, funcName);
	}
	return nullptr;
}

PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void *moduleBase, const char *dllName, const char *funcName)
{
	auto imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	for (; imports->DllNameRVA; ++imports)
	{
		if (_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
			continue;

		auto impName = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
		auto impAddr = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
		return FindAddressByName(moduleBase, impName, impAddr, funcName);
	}
	return nullptr;
}

PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void *moduleBase, const char *dllName, uint16_t ordinal)
{
	auto imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	for (; imports->DllNameRVA; ++imports)
	{
		if (_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
			continue;

		auto impName = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
		auto impAddr = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
		return FindAddressByOrdinal(moduleBase, impName, impAddr, ordinal);
	}
	return nullptr;
}
