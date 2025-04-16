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


#include <stdexcept>
#include <memory>
#include "ImageListSet.h"
#include "Parameters.h"
#include "NppDarkMode.h"
#include "dpiManagerV2.h"

void IconList::init(HINSTANCE hInst, int iconSize) 
{
	InitCommonControls();
	_hInst = hInst;
	_iconSize = iconSize;
	const int nbMore = 45;
	_hImglst = ImageList_Create(iconSize, iconSize, ILC_COLOR32 | ILC_MASK, 0, nbMore);
	if (!_hImglst)
		throw std::runtime_error("IconList::create : ImageList_Create() function returns null");
}


void IconList::create(int iconSize, HINSTANCE hInst, int* iconIDArray, int iconIDArraySize)
{
	init(hInst, iconSize);
	_pIconIDArray = iconIDArray;
	_iconIDArraySize = iconIDArraySize;

	for (int i = 0; i < iconIDArraySize; ++i)
		addIcon(iconIDArray[i], iconSize, iconSize);
}

void IconList::addIcon(int iconID, int cx, int cy, int failIconID, bool isToolbarNormal) const
{
	HICON hIcon = nullptr;
	DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(iconID), cx, cy, &hIcon, LR_DEFAULTSIZE);

	if (!hIcon)
	{
		static bool ignoreWarning = false;
		int userAnswer = 0;
		if (!ignoreWarning)
		{
			userAnswer = ::MessageBoxA(NULL, "IconList::addIcon : LoadIcon() function return null.\nIgnore the error?\n\n\"Yes\": ignore the error and launch Notepad++\n\"No\": Quit Notepad++\n\"Cancel\": display all errors", std::to_string(iconID).c_str(), MB_YESNOCANCEL | MB_ICONWARNING);
			ignoreWarning = userAnswer == IDYES;
		}

		if (userAnswer == IDNO)
		{
			throw std::runtime_error("IconList::addIcon : LoadIcon() function returns null");
		}
		else
		{
			if (failIconID != -1)
			{
				HBITMAP hBmp = static_cast<HBITMAP>(::LoadImage(_hInst, MAKEINTRESOURCE(failIconID), IMAGE_BITMAP, cx, cy, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT));
				if (hBmp != nullptr)
				{
					::ImageList_AddMasked(_hImglst, hBmp, ::GetSysColor(COLOR_3DFACE));
					::DeleteObject(hBmp);
					return;
				}
			}
			DPIManagerV2::loadIcon(_hInst, MAKEINTRESOURCE(IDI_ICONABSENT), cx, cy, &hIcon);
		}
	}

	if (hIcon != nullptr)
	{
		if (isToolbarNormal)
			IconList::changeFluentIconColor(&hIcon);
		::ImageList_AddIcon(_hImglst, hIcon);
		::DestroyIcon(hIcon);
	}
}

void IconList::addIcon(HICON hIcon) const
{
	if (hIcon)
		ImageList_AddIcon(_hImglst, hIcon);
}

bool IconList::changeIcon(size_t index, const wchar_t* iconLocation) const
{
	HICON hIcon = nullptr;
	DPIManagerV2::loadIcon(nullptr, iconLocation, _iconSize, _iconSize, &hIcon, LR_LOADFROMFILE | LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
	if (!hIcon)
		return false;
	size_t i = ::ImageList_ReplaceIcon(_hImglst, static_cast<int>(index), hIcon);
	::DestroyIcon(hIcon);
	return (i == index);
}


bool IconList::changeFluentIconColor(HICON* phIcon, const std::vector<std::pair<COLORREF, COLORREF>>& colorMappings, int tolerance) const
{
	if (!*phIcon)
	{
		return false;
	}

	HDC hdcScreen = nullptr;
	HDC hdcBitmap = nullptr;
	BITMAP bm{};
	ICONINFO ii{};
	HBITMAP hbmNew = nullptr;
	std::unique_ptr<RGBQUAD[]> pixels;

	const bool changeEverything = colorMappings[0].first == 0;

	auto cleanup = [&]()
		{
			if (hdcScreen) ::ReleaseDC(nullptr, hdcScreen);
			if (hdcBitmap) ::DeleteDC(hdcBitmap);
			if (ii.hbmColor) ::DeleteObject(ii.hbmColor);
			if (ii.hbmMask) ::DeleteObject(ii.hbmMask);
			if (hbmNew) ::DeleteObject(hbmNew);
		};

	hdcScreen = ::GetDC(nullptr);
	hdcBitmap = ::CreateCompatibleDC(nullptr);

	if (!hdcScreen || !hdcBitmap || !::GetIconInfo(*phIcon, &ii) || !ii.hbmColor || !::GetObject(ii.hbmColor, sizeof(BITMAP), &bm))
	{
		cleanup();
		return false;
	}

	BITMAPINFO bmi{};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bm.bmWidth;
	bmi.bmiHeader.biHeight = -bm.bmHeight; // Top-down bitmap
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	pixels = std::make_unique<RGBQUAD[]>(static_cast<size_t>(bm.bmWidth) * bm.bmHeight);
	if (!pixels || !::GetDIBits(hdcBitmap, ii.hbmColor, 0, bm.bmHeight, pixels.get(), &bmi, DIB_RGB_COLORS))
	{
		cleanup();
		return false;
	}

	for (int i = 0; i < bm.bmWidth * bm.bmHeight; i++)
	{
		if (pixels[i].rgbReserved != 0) // Modify non-transparent pixels
		{
			if (changeEverything)
			{
				COLORREF cNew = colorMappings[0].second == 0 ? NppDarkMode::getAccentColor() : colorMappings[0].second;
				pixels[i].rgbRed = GetRValue(cNew);
				pixels[i].rgbGreen = GetGValue(cNew);
				pixels[i].rgbBlue = GetBValue(cNew);
			}
			else
			{
				for (const auto& [cToChange, cNew] : colorMappings)
				{

					if (std::abs(pixels[i].rgbRed - GetRValue(cToChange)) <= tolerance &&
						std::abs(pixels[i].rgbGreen - GetGValue(cToChange)) <= tolerance &&
						std::abs(pixels[i].rgbBlue - GetBValue(cToChange)) <= tolerance)
					{
						COLORREF finalNewColor = (cNew == 0) ? NppDarkMode::getAccentColor() : cNew;
						pixels[i].rgbRed = GetRValue(finalNewColor);
						pixels[i].rgbGreen = GetGValue(finalNewColor);
						pixels[i].rgbBlue = GetBValue(finalNewColor);
						break;
					}
				}
			}
		}
	}

	hbmNew = ::CreateCompatibleBitmap(hdcScreen, bm.bmWidth, bm.bmHeight);
	if (!hbmNew || !::SetDIBits(hdcBitmap, hbmNew, 0, bm.bmHeight, pixels.get(), &bmi, DIB_RGB_COLORS))
	{
		cleanup();
		return false;
	}

	if (ii.hbmColor)
	{
		::DeleteObject(ii.hbmColor);
		ii.hbmColor = nullptr;
	}

	ii.hbmColor = hbmNew;
	HICON hIconNew = ::CreateIconIndirect(&ii);
	if (!hIconNew)
	{
		cleanup();
		return false;
	}

	::DestroyIcon(*phIcon);
	*phIcon = hIconNew;

	cleanup();
	return true;
}

bool IconList::changeFluentIconColor(HICON* phIcon) const
{
	const auto cMain = NppDarkMode::isEnabled() ? g_cDefaultMainDark : g_cDefaultMainLight;
	const auto cSecondary = NppDarkMode::isEnabled() ? g_cDefaultSecondaryDark : g_cDefaultSecondaryLight;
	std::vector<std::pair<COLORREF, COLORREF>> colorMappings;

	NppParameters& nppParams = NppParameters::getInstance();
	const auto& tbInfo = nppParams.getNppGUI()._tbIconInfo;

	COLORREF cOld = tbInfo._tbUseMono ? 0 : cSecondary;
	COLORREF cNew = 0;

	switch (tbInfo._tbColor)
	{
		case FluentColor::accent:
		{
			cNew = 0;
			break;
		}

		case FluentColor::red:
		{
			cNew = RGB(0xE8, 0x11, 0x23);
			break;
		}

		case FluentColor::green:
		{
			cNew = RGB(0x00, 0x8B, 0x00);
			break;
		}

		case FluentColor::blue:
		{
			cNew = RGB(0x00, 0x78, 0xD4);
			break;
		}

		case FluentColor::purple:
		{
			cNew = RGB(0xB1, 0x46, 0xC2);
			break;
		}

		case FluentColor::cyan:
		{
			cNew = RGB(0x00, 0xB7, 0xC3);
			break;
		}

		case FluentColor::olive:
		{
			cNew = RGB(0x49, 0x82, 0x05);
			break;
		}

		case FluentColor::yellow:
		{
			cNew = RGB(0xFF, 0xB9, 0x00);
			break;
		}

		case FluentColor::custom:
		{
			if (tbInfo._tbCustomColor != 0)
			{
				cNew = tbInfo._tbCustomColor;
				break;
			}
			[[fallthrough]];
		}

		case FluentColor::defaultColor:
		{
			if (tbInfo._tbUseMono)
			{
				cNew = cMain;
				break;
			}
			[[fallthrough]];
		}

		default:
		{
			return false;
		}
	}

	colorMappings = { {cOld, cNew} };
	return IconList::changeFluentIconColor(phIcon, colorMappings);
}

void ToolBarIcons::init(ToolBarButtonUnit *buttonUnitArray, int arraySize, const std::vector<DynamicCmdIcoBmp>& moreCmds)
{
	for (int i = 0 ; i < arraySize ; ++i)
		_tbiis.push_back(buttonUnitArray[i]);

	_moreCmds = moreCmds;
}

void ToolBarIcons::reInit(int size)
{
	ImageList_SetIconSize(getDefaultLst(), size, size);
	ImageList_SetIconSize(getDisableLst(), size, size);

	ImageList_SetIconSize(getDefaultLstSet2(), size, size);
	ImageList_SetIconSize(getDisableLstSet2(), size, size);

	ImageList_SetIconSize(getDefaultLstDM(), size, size);
	ImageList_SetIconSize(getDisableLstDM(), size, size);

	ImageList_SetIconSize(getDefaultLstSetDM2(), size, size);
	ImageList_SetIconSize(getDisableLstSetDM2(), size, size);

	for (size_t i = 0; i < _iconListVector.size(); ++i)
	{
		_iconListVector[i].removeAll();
	}

	for (size_t i = 0, len = _tbiis.size(); i < len; ++i)
	{
		if (_tbiis[i]._defaultIcon != -1)
		{
			_iconListVector[HLIST_DEFAULT].addIcon(_tbiis[i]._defaultIcon, size, size, _tbiis[i]._stdIcon, true);
			_iconListVector[HLIST_DISABLE].addIcon(_tbiis[i]._grayIcon, size, size, _tbiis[i]._stdIcon);
			_iconListVector[HLIST_DEFAULT2].addIcon(_tbiis[i]._defaultIcon2, size, size, _tbiis[i]._stdIcon, true);
			_iconListVector[HLIST_DISABLE2].addIcon(_tbiis[i]._grayIcon2, size, size, _tbiis[i]._stdIcon);

			_iconListVector[HLIST_DEFAULT_DM].addIcon(_tbiis[i]._defaultDarkModeIcon, size, size, _tbiis[i]._stdIcon, true);
			_iconListVector[HLIST_DISABLE_DM].addIcon(_tbiis[i]._grayDarkModeIcon, size, size, _tbiis[i]._stdIcon);
			_iconListVector[HLIST_DEFAULT_DM2].addIcon(_tbiis[i]._defaultDarkModeIcon2, size, size, _tbiis[i]._stdIcon, true);
			_iconListVector[HLIST_DISABLE_DM2].addIcon(_tbiis[i]._grayDarkModeIcon2, size, size, _tbiis[i]._stdIcon);
		}
	}

	// Add dynamic icons (from plugins)
	for (const auto& i : _moreCmds)
	{
		_iconListVector[HLIST_DEFAULT].addIcon(i._hIcon);
		_iconListVector[HLIST_DISABLE].addIcon(i._hIcon);
		_iconListVector[HLIST_DEFAULT2].addIcon(i._hIcon);
		_iconListVector[HLIST_DISABLE2].addIcon(i._hIcon);

		HICON hIcon = nullptr;

		if (i._hIcon_DM)
		{
			hIcon = i._hIcon_DM;
		}
		else
		{
			ICONINFO iconinfoSrc;
			GetIconInfo(i._hIcon, &iconinfoSrc);

			HDC dcScreen = ::GetDC(NULL);

			BITMAP bmp{};
			int nbByteBmp = ::GetObject(iconinfoSrc.hbmColor, sizeof(BITMAP), &bmp);

			if (!nbByteBmp)
			{
				hIcon = i._hIcon;
			}
			else
			{
				BITMAPINFOHEADER bi = {};

				bi.biSize = sizeof(BITMAPINFOHEADER);
				bi.biWidth = bmp.bmWidth;
				bi.biHeight = bmp.bmHeight;
				bi.biPlanes = 1;
				bi.biBitCount = 32;
				bi.biCompression = BI_RGB;
				bi.biSizeImage = 0;
				bi.biXPelsPerMeter = 0;
				bi.biYPelsPerMeter = 0;
				bi.biClrUsed = 0;
				bi.biClrImportant = 0;

				DWORD dwLineSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4;
				DWORD dwBmpSize = dwLineSize * bmp.bmHeight;

				std::unique_ptr<BYTE[]> dibits(new BYTE[dwBmpSize]);

				GetDIBits(dcScreen, iconinfoSrc.hbmColor, 0, bi.biHeight, dibits.get(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

				for (int scanLine = 0; scanLine < bi.biHeight; ++scanLine)
				{
					BYTE* rawLine = dibits.get() + (dwLineSize * scanLine);
					RGBQUAD* pLine = (RGBQUAD*)rawLine;

					for (int pixel = 0; pixel < bi.biWidth; ++pixel)
					{
						RGBQUAD rgba = pLine[pixel];

						COLORREF c = RGB(rgba.rgbRed, rgba.rgbGreen, rgba.rgbBlue);
						COLORREF invert = NppDarkMode::invertLightness(c);

						rgba.rgbRed = GetRValue(invert);
						rgba.rgbBlue = GetBValue(invert);
						rgba.rgbGreen = GetGValue(invert);

						pLine[pixel] = rgba;
					}
				}

				HBITMAP hBmpNew = ::CreateCompatibleBitmap(dcScreen, bmp.bmWidth, bmp.bmHeight);

				SetDIBits(dcScreen, hBmpNew, 0, bi.biHeight, dibits.get(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);

				::ReleaseDC(NULL, dcScreen);

				ICONINFO iconinfoDest = {};
				iconinfoDest.fIcon = TRUE;
				iconinfoDest.hbmColor = hBmpNew;
				iconinfoDest.hbmMask = iconinfoSrc.hbmMask;

				hIcon = ::CreateIconIndirect(&iconinfoDest);

				::DeleteObject(hBmpNew);
				::DeleteObject(iconinfoSrc.hbmColor);
				::DeleteObject(iconinfoSrc.hbmMask);
			}
		}
		_iconListVector[HLIST_DEFAULT_DM].addIcon(hIcon);
		_iconListVector[HLIST_DISABLE_DM].addIcon(hIcon);
		_iconListVector[HLIST_DEFAULT_DM2].addIcon(hIcon);
		_iconListVector[HLIST_DISABLE_DM2].addIcon(hIcon);
	}
}

void ToolBarIcons::create(HINSTANCE hInst, int iconSize)
{
	_iconListVector.push_back(IconList());
	_iconListVector.push_back(IconList());
	_iconListVector.push_back(IconList());
	_iconListVector.push_back(IconList());
	
	_iconListVector.push_back(IconList());
	_iconListVector.push_back(IconList());
	_iconListVector.push_back(IconList());
	_iconListVector.push_back(IconList());
	

	_iconListVector[HLIST_DEFAULT].init(hInst, iconSize);
	_iconListVector[HLIST_DISABLE].init(hInst, iconSize);
	_iconListVector[HLIST_DEFAULT2].init(hInst, iconSize);
	_iconListVector[HLIST_DISABLE2].init(hInst, iconSize);

	_iconListVector[HLIST_DEFAULT_DM].init(hInst, iconSize);
	_iconListVector[HLIST_DISABLE_DM].init(hInst, iconSize);
	_iconListVector[HLIST_DEFAULT_DM2].init(hInst, iconSize);
	_iconListVector[HLIST_DISABLE_DM2].init(hInst, iconSize);

	reInit(iconSize);
}

void ToolBarIcons::destroy()
{
	_iconListVector[HLIST_DEFAULT].destroy();
	_iconListVector[HLIST_DEFAULT2].destroy();
	_iconListVector[HLIST_DISABLE].destroy();
	_iconListVector[HLIST_DISABLE2].destroy();
}
