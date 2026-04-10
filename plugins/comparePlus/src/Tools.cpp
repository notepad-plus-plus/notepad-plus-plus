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

#include "Tools.h"


// Initialize array of round constants:
// (first 32 bits of the fractional parts of the cube roots of the first 64 primes 2..311)
const uint32_t SHA256::k[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};


std::map<UINT_PTR, DelayedWork*> DelayedWork::workMap;


bool DelayedWork::post(UINT delay_ms)
{
	const bool isRunning = (_timerId != 0);

	_timerId = ::SetTimer(NULL, _timerId, delay_ms, timerCB);

	if (!isRunning && _timerId)
		workMap[_timerId] = this;

	return (_timerId != 0);
}


void DelayedWork::cancel()
{
	if (_timerId)
	{
		::KillTimer(NULL, _timerId);

		std::map<UINT_PTR, DelayedWork*>::iterator it = workMap.find(_timerId);
		if (it != workMap.end())
			workMap.erase(it);

		_timerId = 0;
	}
}


VOID CALLBACK DelayedWork::timerCB(HWND, UINT, UINT_PTR idEvent, DWORD)
{
	std::map<UINT_PTR, DelayedWork*>::iterator it = workMap.find(idEvent);

	::KillTimer(NULL, idEvent);

	// Normally this shouldn't be the case
	if (it == workMap.end())
		return;

	DelayedWork* work = it->second;
	workMap.erase(it);

	// This is not a valid case
	if (!work)
		return;

	work->_timerId = 0;
	(*work)();
}


IFStreamLineGetter::IFStreamLineGetter(std::ifstream& ifs) : _ifs(ifs)
{
	if (ifs.good())
	{
		_readBuf.resize(cBuffSize);

		ifs.read(_readBuf.data(), _readBuf.size());

		_countRead = static_cast<size_t>(ifs.gcount());
	}
}


std::string IFStreamLineGetter::get()
{
	std::string lineStr;

	while (true)
	{
		const size_t pos = _readPos;

		if (lineStr.empty() || (lineStr.back() != '\r' && lineStr.back() != '\n'))
		{
			while (_readPos < _countRead &&
					*(_readBuf.data() + _readPos) != '\r' && *(_readBuf.data() + _readPos) != '\n')
				++_readPos;
		}

		while (_readPos < _countRead &&
				(*(_readBuf.data() + _readPos) == '\r' || *(_readBuf.data() + _readPos) == '\n'))
			++_readPos;

		lineStr.append(_readBuf.data() + pos, _readBuf.data() + _readPos);

		if (_readPos < _countRead || !_ifs.good())
			break;

		_ifs.read(_readBuf.data(), _readBuf.size());

		_countRead = static_cast<size_t>(_ifs.gcount());
		_readPos = 0;
	}

	return lineStr;
}


void SHA256::process_chunk(const uint8_t* p, uint32_t* h)
{
	uint32_t ah[8];

	// create a 64-entry message schedule array w[0..63] of 32-bit words
	// (The initial values in w[0..63] don't matter, so many implementations zero them here)
	// copy chunk into first 16 words w[0..15] of the message schedule array
	uint32_t w[64] = {0};

	for (int i = 0; i < 16; ++i)
	{
		w[i] = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | (uint32_t)p[3];
		p += 4;
	}

	// Extend the first 16 words into the remaining 48 words w[16..63] of the message schedule array
	for (int i = 16; i < 64; ++i)
	{
		const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
		const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}

	// Initialize working variables to current hash value
	for (int i = 0; i < 8; ++i)
		ah[i] = h[i];

	// Compression function main loop
	for (int i = 0; i < 64; ++i)
	{
		const uint32_t s1 = rotr(ah[4], 6) ^ rotr(ah[4], 11) ^ rotr(ah[4], 25);
		const uint32_t ch = (ah[4] & ah[5]) ^ (~ah[4] & ah[6]);
		const uint32_t temp1 = ah[7] + s1 + ch + k[i] + w[i];
		const uint32_t s0 = rotr(ah[0], 2) ^ rotr(ah[0], 13) ^ rotr(ah[0], 22);
		const uint32_t maj = (ah[0] & ah[1]) ^ (ah[0] & ah[2]) ^ (ah[1] & ah[2]);
		const uint32_t temp2 = s0 + maj;

		ah[7] = ah[6];
		ah[6] = ah[5];
		ah[5] = ah[4];
		ah[4] = ah[3] + temp1;
		ah[3] = ah[2];
		ah[2] = ah[1];
		ah[1] = ah[0];
		ah[0] = temp1 + temp2;
	}

	// Add the compressed chunk to the current hash value
	for (int i = 0; i < 8; i++)
		h[i] += ah[i];
}


std::vector<uint8_t> SHA256::operator()(const std::vector<char>& vec)
{
	// Note 1: All integers (expect indexes) are 32-bit unsigned integers and addition is calculated modulo 2^32.
	// Note 2: For each round, there is one round constant k[i] and one entry in the message schedule array w[i],
	//			0 = i = 63
	// Note 3: The compression function uses 8 working variables, a through h
	// Note 4: Big-endian convention is used when expressing the constants in this pseudocode,
	//			and when parsing message block data from bytes to words, for example,
	//			the first word of the input message "abc" after padding is 0x61626380

	// Initialize hash values:
	// (first 32 bits of the fractional parts of the square roots of the first 8 primes 2..19)
	uint32_t h[] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

	uint8_t last_chunks[CHUNK_SIZE * 2] = {0};

	const uint8_t* data_p = reinterpret_cast<const uint8_t*>(vec.data());
	size_t remaining_len = vec.size();

	for (size_t remaining_chunks = remaining_len; remaining_chunks;
		--remaining_chunks, remaining_len -= CHUNK_SIZE, data_p += CHUNK_SIZE)
	{
		// Handle last data chunks (one or two depending on data size)
		if (remaining_len < CHUNK_SIZE)
		{
			uint8_t *last_chunks_p = last_chunks;

			if (remaining_len)
			{
				memcpy(last_chunks_p, data_p, remaining_len);
				last_chunks_p += remaining_len;
			}

			data_p = last_chunks;

			*last_chunks_p++ = 0x80;

			size_t space_in_chunk = CHUNK_SIZE - remaining_len - 1;

			// Now:
			// - either there is enough space left for the total length, and we can conclude,
			// - or there is too little space left, and we have to pad the rest of this chunk with zeroes.
			// In the latter case, we will conclude at the next invokation of this function.
			if (space_in_chunk < TOTAL_LEN_LEN)
			{
				remaining_chunks = 2;
				remaining_len = CHUNK_SIZE * 2;

				last_chunks_p = last_chunks + CHUNK_SIZE;
				space_in_chunk = CHUNK_SIZE;
			}
			else
			{
				remaining_chunks = 1;
				remaining_len = CHUNK_SIZE;
			}

			const size_t left = space_in_chunk - TOTAL_LEN_LEN;
			size_t len = vec.size();

			last_chunks_p += left;

			// Storing of len * 8 as a big endian 64-bit without overflow
			last_chunks_p[7] = (uint8_t)(len << 3);
			len >>= 5;

			for (int i = 6; i >= 0; --i)
			{
				last_chunks_p[i] = (uint8_t)len;
				len >>= 8;
			}
		}

		process_chunk(data_p, h);
	}

	std::vector<uint8_t> hash;

	// Produce the final hash value (big-endian)
	for (int i = 0; i < 8; ++i)
	{
		hash.emplace_back((uint8_t)(h[i] >> 24));
		hash.emplace_back((uint8_t)(h[i] >> 16));
		hash.emplace_back((uint8_t)(h[i] >> 8));
		hash.emplace_back((uint8_t)h[i]);
	}

	return hash;
}


std::vector<wchar_t> getFromClipboard(bool addLeadingNewLine)
{
	std::vector<wchar_t> content;

	if (!::OpenClipboard(NULL))
		return content;

	HANDLE hData = ::GetClipboardData(CF_UNICODETEXT);

	if (hData != NULL)
	{
		wchar_t* pText = static_cast<wchar_t*>(::GlobalLock(hData));

		if (pText != NULL)
		{
			const size_t len = wcslen(pText) + 1;

			if (addLeadingNewLine)
			{
				content.resize(len + 1);
				content[0] = L'\n'; // Needed for selections alignment after comparing
				wcscpy_s(content.data() + 1, len, pText);
			}
			else
			{
				content.resize(len);
				wcscpy_s(content.data(), len, pText);
			}
		}

		::GlobalUnlock(hData);
	}

	::CloseClipboard();

	return content;
}


bool setToClipboard(const std::vector<wchar_t>& txt)
{
	if (txt.empty())
		return true;

	HGLOBAL hglbCopy = ::GlobalAlloc(GMEM_MOVEABLE, txt.size() * sizeof(wchar_t));
	if (hglbCopy == nullptr)
		return false;

	// Lock the handle and copy the text to the buffer
	wchar_t* pStr = (wchar_t*)::GlobalLock(hglbCopy);
	if (!pStr)
	{
		::GlobalFree(hglbCopy);
		return false;
	}

	wcscpy_s(pStr, txt.size(), txt.data());
	::GlobalUnlock(hglbCopy);

	if (!::OpenClipboard(NULL))
	{
		::GlobalFree(hglbCopy);
		return false;
	}

	if (!::EmptyClipboard())
	{
		::GlobalFree(hglbCopy);
		::CloseClipboard();
		return false;
	}

	// Place the handle on the clipboard
	if (!::SetClipboardData(CF_UNICODETEXT, hglbCopy))
	{
		::GlobalFree(hglbCopy);
		::CloseClipboard();
		return false;
	}

	::CloseClipboard();

	return true;
}


void toLowerCase(std::vector<char>& text, int codepage)
{
	const int len = static_cast<int>(text.size());

	if (len == 0)
		return;

	const int wLen = ::MultiByteToWideChar(codepage, 0, text.data(), len, NULL, 0);

	std::vector<wchar_t> wText(wLen);

	::MultiByteToWideChar(codepage, 0, text.data(), len, wText.data(), wLen);

	wText.push_back(L'\0');
	::CharLowerW((LPWSTR)wText.data());
	wText.pop_back();

	::WideCharToMultiByte(codepage, 0, wText.data(), wLen, text.data(), len, NULL, NULL);
}


std::wstring MBtoWC(const char* mb, int len, int codepage)
{
	if (!mb || !len)
		return {};

	const int wLen = ::MultiByteToWideChar(codepage, 0, mb, len, NULL, 0);

	std::wstring str;
	str.resize(len > 0 ? wLen : wLen - 1);

	::MultiByteToWideChar(codepage, 0, mb, len, &str[0], wLen);

	return str;
}


std::string WCtoMB(const wchar_t* wc, int len, int codepage)
{
	if (!wc || !len)
		return {};

	const int l = ::WideCharToMultiByte(codepage, 0, wc, len, NULL, 0, NULL, NULL);

	std::string str;
	str.resize(len > 0 ? l : l - 1);

	::WideCharToMultiByte(codepage, 0, wc, len, &str[0], l, NULL, NULL);

	return str;
}


void updateDlgCtrlTxt(HWND hDlgWnd, int ctrlId, const wchar_t* txt, bool isCheckBox)
{
	if (hDlgWnd == nullptr)
		return;

	HWND hCtrl = ::GetDlgItem(hDlgWnd, ctrlId);

	// Ensure the control has the BS_MULTILINE style
	// Note: This needs to be done in the resource file (.rc) for standard controls to behave well
	// ::SetWindowLongPtrW(hCtrl, GWL_STYLE, ::GetWindowLongPtrW(hCtrl, GWL_STYLE) | BS_MULTILINE);

	::SetWindowTextW(hCtrl, txt);

	HDC hdc = ::GetDC(hCtrl);
	HFONT hFont = (HFONT)::SendMessageW(hCtrl, WM_GETFONT, 0, 0);
	HFONT hOldFont = (HFONT)::SelectObject(hdc, hFont);

	// Get the current width of the control to use as the maximum width for wrapping
	RECT rcCalc;
	::GetClientRect(hCtrl, &rcCalc);

	// DT_CALCRECT calculates the height needed based on the current width (rcCalc.right)
	// DT_WORDBREAK ensures wrapping at word boundaries
	::DrawText(hdc, txt, -1, &rcCalc, DT_CALCRECT | DT_LEFT | DT_WORDBREAK);

	// rcCalc now contains the minimum required height and possibly a modified width
	::SelectObject(hdc, hOldFont);
	::ReleaseDC(hCtrl, hdc);

	int desiredWidth = rcCalc.right + 5;
	int desiredHeight = rcCalc.bottom;

	// Ensure the final width is sufficient for the calculated wrapped text width + the button glyph
	if (isCheckBox)
	{
		desiredWidth += ::GetSystemMetrics(SM_CXMENUCHECK) + 5;
		desiredHeight += 5;
	}

	::SetWindowPos(hCtrl, NULL, 0, 0, desiredWidth, desiredHeight, SWP_NOMOVE | SWP_NOZORDER);

	// Invalidate the window to force a repaint with the new layout
	::InvalidateRect(hCtrl, NULL, TRUE);
}


HFONT createFontFromSystemDefault(SysFont font, int size, bool underlined)
{
	HFONT hf {nullptr};

	NONCLIENTMETRICS ncm {0};
	ncm.cbSize = sizeof(NONCLIENTMETRICS);

#if (WINVER >= 0x0600)
	if (!::IsWindows7OrGreater())
		ncm.cbSize -= sizeof(int);
#endif

	if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0))
	{
		LOGFONTW lf;

		switch (font)
		{
			case SysFont::Caption:
				lf = ncm.lfCaptionFont;
			break;

			case SysFont::SmallCaption:
				lf = ncm.lfSmCaptionFont;
			break;

			case SysFont::Menu:
				lf = ncm.lfMenuFont;
			break;

			case SysFont::Status:
				lf = ncm.lfStatusFont;
			break;

			case SysFont::Message:
				lf = ncm.lfMessageFont;
			break;

			default:
			return hf;
		}

		if (size)
		{
			HDC hdc = ::GetDC(nullptr);

			if (hdc)
			{
				lf.lfHeight = -::MulDiv(size, ::GetDeviceCaps(hdc, LOGPIXELSY), 72);

				::ReleaseDC(nullptr, hdc);
			}
		}

		lf.lfUnderline = underlined ? TRUE : FALSE;

		hf = ::CreateFontIndirectW(&lf);
	}

	return hf;
}
