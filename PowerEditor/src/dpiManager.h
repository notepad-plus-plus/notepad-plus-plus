// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
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

#ifndef DPIMANAGER_H
#define DPIMANAGER_H

class DPIManager
{
public:
    DPIManager() { 
		init();
	};
    
    // Get screen DPI.
    int getDPIX() { return _dpiX; };
    int getDPIY() { return _dpiY; };

    // Convert between raw pixels and relative pixels.
    int scaleX(int x) { return MulDiv(x, _dpiX, 96); };
    int scaleY(int y) { return MulDiv(y, _dpiY, 96); };
    int unscaleX(int x) { return MulDiv(x, 96, _dpiX); };
    int unscaleY(int y) { return MulDiv(y, 96, _dpiY); };

    // Determine the screen dimensions in relative pixels.
    int scaledScreenWidth() { return scaledSystemMetricX(SM_CXSCREEN); }
    int scaledScreenHeight() { return scaledSystemMetricY(SM_CYSCREEN); }

    // Scale rectangle from raw pixels to relative pixels.
    void scaleRect(__inout RECT *pRect) {
        pRect->left = scaleX(pRect->left);
        pRect->right = scaleX(pRect->right);
        pRect->top = scaleY(pRect->top);
        pRect->bottom = scaleY(pRect->bottom);
    }

    // Scale Point from raw pixels to relative pixels.
    void scalePoint(__inout POINT *pPoint)
    {
        pPoint->x = scaleX(pPoint->x);
        pPoint->y = scaleY(pPoint->y);        
    }

    // Scale Size from raw pixels to relative pixels.
    void scaleSize(__inout SIZE *pSize)
    {
        pSize->cx = scaleX(pSize->cx);
        pSize->cy = scaleY(pSize->cy);		
    }

    // Determine if screen resolution meets minimum requirements in relative pixels.
    bool isResolutionAtLeast(int cxMin, int cyMin) 
    { 
        return (scaledScreenWidth() >= cxMin) && (scaledScreenHeight() >= cyMin); 
    }

    // Convert a point size (1/72 of an inch) to raw pixels.
    int pointsToPixels(int pt) { return MulDiv(pt, _dpiY, 72); };

    // Invalidate any cached metrics.
    void Invalidate() { init(); };

private:
	// X and Y DPI values are provided, though to date all 
    // Windows OS releases have equal X and Y scale values
    int _dpiX;			
    int _dpiY;


	void init() {
	    HDC hdc = GetDC(NULL);
        if (hdc)
        {
            // Initialize the DPIManager member variable
            // This will correspond to the DPI setting
            // With all Windows OS's to date the X and Y DPI will be identical					
            _dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
            _dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(NULL, hdc);
        }
	};

    // This returns a 96-DPI scaled-down equivalent value for nIndex 
    // For example, the value 120 at 120 DPI setting gets scaled down to 96		
    // X and Y versions are provided, though to date all Windows OS releases 
    // have equal X and Y scale values
    int scaledSystemMetricX(int nIndex) {
        return MulDiv(GetSystemMetrics(nIndex), 96, _dpiX); 
    };

    // This returns a 96-DPI scaled-down equivalent value for nIndex 
    // For example, the value 120 at 120 DPI setting gets scaled down to 96		
    // X and Y versions are provided, though to date all Windows OS releases 
    // have equal X and Y scale values
    int scaledSystemMetricY(int nIndex) 
    {
        return MulDiv(GetSystemMetrics(nIndex), 96, _dpiY); 
    }
};

#endif //DPIMANAGER_H