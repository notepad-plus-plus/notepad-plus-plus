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
#include "NppDarkMode.h"

void IconList::init(HINSTANCE hInst, int iconSize) 
{
	InitCommonControls();
	_hInst = hInst;
	_iconSize = iconSize;
	const int nbMore = 45;
	_hImglst = ImageList_Create(iconSize, iconSize, ILC_COLOR32 | ILC_MASK, 0, nbMore);
	if (!_hImglst)
		throw std::runtime_error("IconList::create : ImageList_Create() function returns null");
};

void IconList::create(int iconSize, HINSTANCE hInst, int *iconIDArray, int iconIDArraySize)
{
	init(hInst, iconSize);
	_pIconIDArray = iconIDArray;
	_iconIDArraySize = iconIDArraySize;

	for (int i = 0 ; i < iconIDArraySize ; ++i)
		addIcon(iconIDArray[i]);
};

void IconList::addIcon(int iconID) const 
{
	HICON hIcon = ::LoadIcon(_hInst, MAKEINTRESOURCE(iconID));
	if (!hIcon)
		throw std::runtime_error("IconList::addIcon : LoadIcon() function return null");

	ImageList_AddIcon(_hImglst, hIcon);
	::DestroyIcon(hIcon);
};


void IconList::addIcon(HICON hIcon) const
{
	if (hIcon)
		ImageList_AddIcon(_hImglst, hIcon);
};

bool IconList::changeIcon(size_t index, const TCHAR *iconLocation) const
{
	HBITMAP hBmp = (HBITMAP)::LoadImage(_hInst, iconLocation, IMAGE_ICON, _iconSize, _iconSize, LR_LOADFROMFILE | LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
	if (!hBmp)
		return false;
	size_t i = ImageList_ReplaceIcon(_hImglst, int(index), (HICON)hBmp);
	ImageList_AddMasked(_hImglst, (HBITMAP)hBmp, RGB(255,0,255));
	::DeleteObject(hBmp);
	return (i == index);
}

// used by tabbar only
void IconList::addIcons(int size) const
{
	ImageList_SetIconSize(_hImglst, size, size);
	for (int i = 0 ; i < _iconIDArraySize ; ++i)
		addIcon(_pIconIDArray[i]);
}

void ToolBarIcons::init(ToolBarButtonUnit *buttonUnitArray, int arraySize, std::vector<DynamicCmdIcoBmp> moreCmds)
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
			_iconListVector[HLIST_DEFAULT].addIcon(_tbiis[i]._defaultIcon);
			_iconListVector[HLIST_DISABLE].addIcon(_tbiis[i]._grayIcon);
			_iconListVector[HLIST_DEFAULT2].addIcon(_tbiis[i]._defaultIcon2);
			_iconListVector[HLIST_DISABLE2].addIcon(_tbiis[i]._grayIcon2);

			_iconListVector[HLIST_DEFAULT_DM].addIcon(_tbiis[i]._defaultDarkModeIcon);
			_iconListVector[HLIST_DISABLE_DM].addIcon(_tbiis[i]._grayDarkModeIcon);
			_iconListVector[HLIST_DEFAULT_DM2].addIcon(_tbiis[i]._defaultDarkModeIcon2);
			_iconListVector[HLIST_DISABLE_DM2].addIcon(_tbiis[i]._grayDarkModeIcon2);
		}
	}

	// Add dynamic icons (from plugins)
	for (auto i : _moreCmds)
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

			BITMAP bmp;
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




