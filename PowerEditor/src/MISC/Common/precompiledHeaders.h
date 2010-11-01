//this file is part of notepad++
//Copyright (C)2003 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef PRECOMPILEHEADER_H
#define PRECOMPILEHEADER_H

// w/o precompiled headers file : 1 minute 55 sec

#define _WIN32_WINNT 0x0501

// C RunTime Header Files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <functional>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

// STL Headers
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <memory>
#include <algorithm>
#include <exception>	

// Windows Header Files
#include <windows.h>
#include <commctrl.h>
#include <Shlobj.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include <Oleacc.h>
#include <dbghelp.h>
#include <eh.h>

#ifdef UNICODE
#include <wchar.h>
#endif

// Notepad++
#include "Common.h"
#include "Window.h"
#include "StaticDialog.h"

#endif PRECOMPILEHEADER_H
