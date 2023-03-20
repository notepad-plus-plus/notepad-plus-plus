#pragma once
#pragma warning(disable:4324)

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <pathcch.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shobjidl_core.h>
#include <strsafe.h>
#include <Unknwn.h>

// WinRT Header Files
#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Management.Deployment.h>

// Windows Implementation Library Header Files
#include "wil\winrt.h"

// Link libraries
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "runtimeobject.lib")
#pragma comment(lib, "pathcch.lib")