// This file is part of Notepad++ project
// Copyright (C)2021 Michael Kuklinski <mike.k@digitalcarbide.com>

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

#pragma once

#include "Common.h"

// Include that contain COM classes and IDLs
// The COM interfaces that we are about here are only available if we specify Windows 10 or higher,
// So we override the default _WIN32_WINNT that is set in the project file for this include.
#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_WIN10)
#	undef _WIN32_WINNT
#endif
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#include <ShObjIdl.h>

// MinGW and such have IDL headers that do not have IVirtualDesktopManager.
// Thus, we need to provide the declarations in that case, and ideally
// declarations that are syntactically valid for both MSVC _and_ MinGW-g++.
// 
// These are lifted from ShObjldl.h and ShObjldl_core.h
#ifndef __IVirtualDesktopManager_INTERFACE_DEFINED__
#define __IVirtualDesktopManager_INTERFACE_DEFINED__

// MIDL/COM interfaces in C++ expect the IDL format for GUIDs, which is { u32, u16, u16, u8[8] }
// However, declspec(uuid(...)) expects _registry_ format, which is { u32, u16, u16, u16, u8[6] }
// While they're convertible to one another, they are different, and these strings must all be known
// to the preprocessor so that they get propagated down properly to the attributes/declspecs,
// so we need to incur some macro magic in order to make sure that they get string literals

// MinGW defines the UUIDs in a slightly different way than MSVC, and they are not compatible, or rather, MSVC doesn't understand MinGW syntax

// This isn't defined by the normal Windows headers, but it is under MinGW
#ifndef __CRT_UUID_DECL
#	define __CRT_UUID_DECL(type,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)
#endif
// To deal with some macro syntax stupidity, as one of the macros returns an argument list - this coerces it back into being multiple arguments
#define NPP_UUID_DECL(...) __CRT_UUID_DECL __VA_ARGS__

// Turns the provided value into a string literal
#define NPP_STRINGIFY(...) #__VA_ARGS__

// Turns a UUID sequence into a registry-formatted UUID string
#define NPP_REG_UUID_STR(l,w1,w2,w3,q4) \
	NPP_STRINGIFY(l ## - ## w1 ## - ## w2 ## - ## w3 ## - ## q4)

// Turns a UUID sequence into a sequence valid for arguments for a GUID-type
#define NPP_IDL_UUID(l,w1,w2,w3,q4) \
	0x ## l ## ul, \
	std::uint16_t(0x ## w1), \
	std::uint16_t(0x ## w2), \
	std::uint16_t(0x ## w3) >> 8, \
	std::uint16_t(0x ## w3) & 0xFF, \
	((0x ## q4 ## ull) >> 40) & 0xFF, \
	((0x ## q4 ## ull) >> 32) & 0xFF, \
	((0x ## q4 ## ull) >> 24) & 0xFF, \
	((0x ## q4 ## ull) >> 16) & 0xFF, \
	((0x ## q4 ## ull) >> 8) & 0xFF, \
	(0x ## q4 ## ull) & 0xFF

// Create an interface declaration. This should work under MSVC _and_ MinGW
#define NPP_COM_INTERFACE_DECL(name, l, w1, w2, w3, q4) \
	static constexpr const IID IID_ ## name = { NPP_IDL_UUID(l, w1, w2, w3, q4) }; \
	struct name; \
	NPP_UUID_DECL((name, NPP_IDL_UUID(l, w1, w2, w3, q4))) \
	MIDL_INTERFACE(NPP_REG_UUID_STR(l, w1, w2, w3, q4)) name

// Declare the class and the interface.
extern const CLSID CLSID_VirtualDesktopManager = { 0xaa509086, 0x5ca9, 0x4c25, { 0x8f, 0x95, 0x58, 0x9d, 0x3c, 0x07, 0xb4, 0x8a } };

NPP_COM_INTERFACE_DECL(IVirtualDesktopManager, a5cd92ff, 29be, 454c, 8d04, d82879fb3f1b) : public IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE IsWindowOnCurrentVirtualDesktop(
		__RPC__in HWND topLevelWindow,
		__RPC__out BOOL * onCurrentDesktop
	) = 0;
};

// Clean up all the macros that we'd created
#undef NPP_UUID_DECL
#undef NPP_STRINGIFY
#undef NPP_REG_UUID_STR
#undef NPP_IDL_UUID
#undef NPP_COM_INTERFACE_DECL
#endif
