/*
 * This file is part of ComparePlus plugin for Notepad++
 * Copyright (C) 2016-2025 Pavel Nedev (pg.nedev@gmail.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <windows.h>
#include <wchar.h>
#include <uxtheme.h>
#include <versionhelpers.h>


/**
 *  \struct
 *  \brief
 */
template <typename T>
struct ScopedIncrementer
{
	ScopedIncrementer(T& useCount) : _useCount(useCount)
	{
		++_useCount;
	}

	~ScopedIncrementer()
	{
		--_useCount;
	}

	ScopedIncrementer& operator=(const ScopedIncrementer&) = delete;

private:
	T&	_useCount;
};


using ScopedIncrementerInt = ScopedIncrementer<int>;


/**
 *  \class
 *  \brief
 */
class DelayedWork
{
public:
	bool post(UINT delay_ms);
	void cancel();

	bool isPending() const
	{
		return (_timerId != 0);
	}

	explicit operator bool() const
	{
		return (_timerId != 0);
	}

	bool operator!() const
	{
		return (_timerId == 0);
	}

protected:
	DelayedWork() : _timerId(0) {}
	DelayedWork(const DelayedWork&) = delete;
	DelayedWork& operator=(const DelayedWork&) = delete;

	virtual ~DelayedWork()
	{
		cancel();
	}

	virtual void operator()() = 0;

	UINT_PTR _timerId;

private:
	static VOID CALLBACK timerCB(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

	static std::map<UINT_PTR, DelayedWork*> workMap;
};


/**
 *  \class
 *  \brief	Reads chunks of data from file stream and returns it line-by-line on each get() call
 *			preserving original EOL
 */
class IFStreamLineGetter
{
public:
	IFStreamLineGetter(std::ifstream& ifs);
	~IFStreamLineGetter() {}

	std::string get();

private:
	static constexpr size_t cBuffSize {2048};

	IFStreamLineGetter(const IFStreamLineGetter&) = delete;
	IFStreamLineGetter(IFStreamLineGetter&&) = delete;
	IFStreamLineGetter& operator=(const IFStreamLineGetter&) = delete;

	std::ifstream& 		_ifs;

	std::vector<char>	_readBuf;
	size_t				_readPos	{0};
	size_t				_countRead	{0};
};


/**
 *  \class
 *  \brief	Calculates SHA256 - Original code is borrowed from Notepad++ Sha256 calculation function and modified
 *			into a class that is intended to be able to calculate SHA256 from different data sources
 */
class SHA256
{
public:
	SHA256() {}
	~SHA256() {}

	// Calculate from vector
	std::vector<uint8_t> operator()(const std::vector<char>& vec);

private:
	static constexpr int CHUNK_SIZE = 64;
	static constexpr int TOTAL_LEN_LEN = 8;

	static const uint32_t k[64];

	SHA256(const SHA256&) = delete;
	SHA256(SHA256&&) = delete;
	SHA256& operator=(const SHA256&) = delete;

	// Rotate right
	inline uint32_t rotr(uint32_t value, unsigned int count)
	{
		return value >> count | value << (32 - count);
	};

	void process_chunk(const uint8_t* p, uint32_t* h);
};


inline void setExplorerLightTheme(HWND hwnd)
{
	if (::IsWindows10OrGreater())
		::SetWindowTheme(hwnd, L"Explorer", nullptr);
}


inline void setExplorerDarkTheme(HWND hwnd)
{
	if (::IsWindows10OrGreater())
		::SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
}


inline void flushMsgQueue()
{
	MSG msg;

	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE));
}


inline bool fileExists(const wchar_t* filePath)
{
	if (filePath == nullptr)
		return false;

	DWORD dwAttrib = ::GetFileAttributesW(filePath);
	return (bool)(dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}


std::vector<wchar_t> getFromClipboard(bool addLeadingNewLine = false);
bool setToClipboard(const std::vector<wchar_t>& txt);

void toLowerCase(std::vector<char>& text, int codepage = CP_UTF8);

std::wstring MBtoWC(const char* mb, int len = -1, int codepage = CP_UTF8);
std::string WCtoMB(const wchar_t* wc, int len = -1, int codepage = CP_UTF8);

void updateDlgCtrlTxt(HWND hDlgWnd, int ctrlId, const wchar_t* txt, bool isCheckBox = false);


enum class SysFont {
	Caption,
	SmallCaption,
	Menu,
	Status,
	Message
};


HFONT createFontFromSystemDefault(SysFont font, int size = 0, bool underlined = false);
