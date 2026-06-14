#include <windows.h>

#include "DarkMode.h"

#include "IatHook.h"

#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>

#include <cstdint>
#include <cwchar>
#include <mutex>
#include <unordered_set>
#include <utility>

#if defined(__GNUC__) && __GNUC__ > 8
#define WINAPI_LAMBDA_RETURN(return_t) -> return_t WINAPI
#elif defined(__GNUC__)
#define WINAPI_LAMBDA_RETURN(return_t) WINAPI -> return_t
#else
#define WINAPI_LAMBDA_RETURN(return_t) -> return_t
#endif

enum IMMERSIVE_HC_CACHE_MODE
{
	IHCM_USE_CACHED_VALUE,
	IHCM_REFRESH
};

// 1903 18362
enum class PreferredAppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

enum WINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_HOLOGRAPHIC = 23,
	WCA_EXCLUDED_FROM_DDA = 24,
	WCA_PASSIVEUPDATEMODE = 25,
	WCA_USEDARKMODECOLORS = 26,
	WCA_LAST = 27
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
};

using fnRtlGetNtVersionNumbers = void (WINAPI *)(LPDWORD major, LPDWORD minor, LPDWORD build);
using fnSetWindowCompositionAttribute = BOOL (WINAPI *)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
// 1809 17763
using fnAllowDarkModeForWindow = bool (WINAPI *)(HWND hWnd, bool allow); // ordinal 133
using fnAllowDarkModeForApp = bool (WINAPI *)(bool allow); // ordinal 135, in 1809
using fnFlushMenuThemes = void (WINAPI *)(); // ordinal 136
using fnRefreshImmersiveColorPolicyState = void (WINAPI *)(); // ordinal 104
using fnIsDarkModeAllowedForWindow = bool (WINAPI *)(HWND hWnd); // ordinal 137
using fnGetIsImmersiveColorUsingHighContrast = bool (WINAPI *)(IMMERSIVE_HC_CACHE_MODE mode); // ordinal 106
using fnOpenNcThemeData = HTHEME(WINAPI *)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49
// 1903 18362
using fnShouldSystemUseDarkMode = bool (WINAPI *)(); // ordinal 138
using fnSetPreferredAppMode = PreferredAppMode (WINAPI *)(PreferredAppMode appMode); // ordinal 135, in 1903
using fnIsDarkModeAllowedForApp = bool (WINAPI *)(); // ordinal 139

static fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;
static fnAllowDarkModeForWindow _AllowDarkModeForWindow = nullptr;
static fnAllowDarkModeForApp _AllowDarkModeForApp = nullptr;
static fnFlushMenuThemes _FlushMenuThemes = nullptr;
static fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = nullptr;
static fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow = nullptr;
static fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast = nullptr;
static fnOpenNcThemeData _OpenNcThemeData = nullptr;
// 1903 18362
static fnSetPreferredAppMode _SetPreferredAppMode = nullptr;

bool g_darkModeSupported = false;
bool g_darkModeEnabled = false;
static DWORD g_buildNumber = 0;

bool AllowDarkModeForWindow(HWND hWnd, bool allow)
{
	if (g_darkModeSupported && _AllowDarkModeForWindow)
		return _AllowDarkModeForWindow(hWnd, allow);
	return false;
}

bool IsHighContrast()
{
	HIGHCONTRASTW highContrast{};
	highContrast.cbSize = sizeof(HIGHCONTRASTW);
	if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRASTW), &highContrast, FALSE))
		return (highContrast.dwFlags & HCF_HIGHCONTRASTON) == HCF_HIGHCONTRASTON;
	return false;
}

void SetTitleBarThemeColor(HWND hWnd, BOOL dark)
{
	if (g_buildNumber < 18362)
		SetPropW(hWnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<intptr_t>(dark)));
	else if (_SetWindowCompositionAttribute)
	{
		WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
		_SetWindowCompositionAttribute(hWnd, &data);
	}
}

void RefreshTitleBarThemeColor(HWND hWnd)
{
	BOOL dark = FALSE;
	if (_IsDarkModeAllowedForWindow)
	{
		if (_IsDarkModeAllowedForWindow(hWnd) && !IsHighContrast())
		{
			dark = TRUE;
		}
	}

	SetTitleBarThemeColor(hWnd, dark);
}

bool IsColorSchemeChangeMessage(LPARAM lParam)
{
	const bool isMsg = lParam && (_wcsicmp(reinterpret_cast<LPCWSTR>(lParam), L"ImmersiveColorSet") == 0);
	if (isMsg)
	{
		if (_RefreshImmersiveColorPolicyState != nullptr)
		{
			_RefreshImmersiveColorPolicyState();
		}

		if (_GetIsImmersiveColorUsingHighContrast != nullptr)
		{
			_GetIsImmersiveColorUsingHighContrast(IMMERSIVE_HC_CACHE_MODE::IHCM_REFRESH);
		}
	}
	return isMsg;
}

bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam)
{
	if (message == WM_SETTINGCHANGE)
		return IsColorSchemeChangeMessage(lParam);
	return false;
}

void AllowDarkModeForApp(bool allow)
{
	if (_SetPreferredAppMode)
		_SetPreferredAppMode(allow ? PreferredAppMode::ForceDark : PreferredAppMode::Default);
	else if (_AllowDarkModeForApp)
		_AllowDarkModeForApp(allow);
}

static void FlushMenuThemes()
{
	if (_FlushMenuThemes)
	{
		_FlushMenuThemes();
	}
}

// limit dark scroll bar to specific windows and their children

std::unordered_set<HWND> g_darkScrollBarWindows;
std::mutex g_darkScrollBarMutex;

void EnableDarkScrollBarForWindowAndChildren(HWND hwnd)
{
	std::lock_guard<std::mutex> lock(g_darkScrollBarMutex);
	g_darkScrollBarWindows.insert(hwnd);
}

static bool IsWindowOrParentUsingDarkScrollBar(HWND hwnd)
{
	HWND hwndRoot = GetAncestor(hwnd, GA_ROOT);

	std::lock_guard<std::mutex> lock(g_darkScrollBarMutex);
	if (g_darkScrollBarWindows.count(hwnd))
	{
		return true;
	}

	if (hwnd != hwndRoot && g_darkScrollBarWindows.count(hwndRoot))
	{
		return true;
	}

	return false;
}

static void FixDarkScrollBar()
{
	HMODULE hComctl = LoadLibraryEx(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hComctl)
	{
		auto addr = FindDelayLoadThunkInModule(hComctl, "uxtheme.dll", 49); // OpenNcThemeData
		if (addr)
		{
			DWORD oldProtect;
			if (VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect) && _OpenNcThemeData)
			{
				auto MyOpenThemeData = [](HWND hWnd, LPCWSTR classList) WINAPI_LAMBDA_RETURN(HTHEME) {
					if (wcscmp(classList, L"ScrollBar") == 0)
					{
						if (IsWindowOrParentUsingDarkScrollBar(hWnd))
						{
							hWnd = nullptr;
							classList = L"Explorer::ScrollBar";
						}
					}
					return _OpenNcThemeData(hWnd, classList);
				};

				addr->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<fnOpenNcThemeData>(MyOpenThemeData));
				VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
			}
		}
	}
}

static constexpr bool CheckBuildNumber(DWORD buildNumber)
{
	return (buildNumber == 17763 || // 1809
		buildNumber == 18362 || // 1903
		buildNumber == 18363 || // 1909
		buildNumber == 19041 || // 2004
		buildNumber == 19042 || // 20H2
		buildNumber == 19043 || // 21H1
		buildNumber == 19044 || // 21H2
		(buildNumber > 19044 && buildNumber < 22000) || // Windows 10 any version > 21H2 
		buildNumber >= 22000);  // Windows 11 builds
}

bool IsWindows10() // or later OS version
{
	return (g_buildNumber >= 17763);
}

bool IsWindows11() // or later OS version
{
	return (g_buildNumber >= 22000);
}

DWORD GetWindowsBuildNumber()
{
	return g_buildNumber;
}

void InitDarkMode()
{
	fnRtlGetNtVersionNumbers RtlGetNtVersionNumbers = nullptr;
	HMODULE hNtdllModule = GetModuleHandle(L"ntdll.dll");
	if (hNtdllModule)
	{
		RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(GetProcAddress(hNtdllModule, "RtlGetNtVersionNumbers"));
	}

	if (RtlGetNtVersionNumbers)
	{
		DWORD major, minor;
		RtlGetNtVersionNumbers(&major, &minor, &g_buildNumber);
		g_buildNumber &= ~0xF0000000;
		if (major == 10 && minor == 0 && CheckBuildNumber(g_buildNumber))
		{
			HMODULE hUxtheme = LoadLibraryEx(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			if (hUxtheme)
			{
				_OpenNcThemeData = reinterpret_cast<fnOpenNcThemeData>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(49)));
				_RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
				_GetIsImmersiveColorUsingHighContrast = reinterpret_cast<fnGetIsImmersiveColorUsingHighContrast>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(106)));
				_AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));

				auto ord135 = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
				if (g_buildNumber < 18362)
					_AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(ord135);
				else
					_SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(ord135);

				_FlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
				_IsDarkModeAllowedForWindow = reinterpret_cast<fnIsDarkModeAllowedForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(137)));

				HMODULE hUser32Module = GetModuleHandleW(L"user32.dll");
				if (hUser32Module)
				{
					_SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(hUser32Module, "SetWindowCompositionAttribute"));
				}

				if (_OpenNcThemeData &&
					_RefreshImmersiveColorPolicyState &&
					_AllowDarkModeForWindow &&
					(_AllowDarkModeForApp || _SetPreferredAppMode) &&
					_FlushMenuThemes &&
					_IsDarkModeAllowedForWindow)
				{
					g_darkModeSupported = true;
				}
			}
		}
	}
}

void SetDarkMode(bool useDark, bool fixDarkScrollbar)
{
	if (g_darkModeSupported)
	{
		AllowDarkModeForApp(useDark);
		FlushMenuThemes();
		if (fixDarkScrollbar)
		{
			FixDarkScrollBar();
		}
		g_darkModeEnabled = useDark && !IsHighContrast();
	}
}

class ModuleHandle
{
public:
	ModuleHandle() = delete;

	explicit ModuleHandle(const wchar_t* moduleName) noexcept
		: m_hModule(::LoadLibraryExW(moduleName, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32))
	{
	}

	ModuleHandle(const ModuleHandle&) = delete;
	ModuleHandle& operator=(const ModuleHandle&) = delete;

	ModuleHandle(ModuleHandle&&) = delete;
	ModuleHandle& operator=(ModuleHandle&&) = delete;

	~ModuleHandle()
	{
		if (m_hModule != nullptr)
		{
			::FreeLibrary(m_hModule);
			m_hModule = nullptr;
		}
	}

	[[nodiscard]] HMODULE get() const noexcept
	{
		return m_hModule;
	}

	[[nodiscard]] bool isLoaded() const noexcept
	{
		return m_hModule != nullptr;
	}

private:
	HMODULE m_hModule = nullptr;
};

using fnFindThunkInModule = auto (*)(void* moduleBase, const char* dllName, const char* funcName)->PIMAGE_THUNK_DATA;

using fnGetThemeColor = auto (WINAPI*)(HTHEME hTheme, int iPartId, int iStateId, int iPropId, COLORREF* pColor)->HRESULT;
using fnDrawThemeBackgroundEx = auto (WINAPI*)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pRect, const DTBGOPTS* pOptions)->HRESULT;

template <typename P>
static auto ReplaceFunction(IMAGE_THUNK_DATA* addr, const P& newFunction) noexcept -> P
{
	DWORD oldProtect = 0;
	if (::VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect) == FALSE)
	{
		return nullptr;
	}

	const ULONGLONG oldFunction = addr->u1.Function;
	addr->u1.Function = reinterpret_cast<ULONGLONG>(newFunction);
	::VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
	return reinterpret_cast<P>(oldFunction);
}

template <typename T>
struct HookData
{
	T m_trueFn = nullptr;
	size_t m_ref = 0;
	const char* m_dllName = nullptr;

	const char* m_fnName = nullptr;
	fnFindThunkInModule m_findFn = nullptr;

	std::uint16_t m_ord = 0;

	void init(const char* dllName, const char* funcName, const fnFindThunkInModule& findFn) noexcept
	{
		if (m_dllName == nullptr)
		{
			m_dllName = dllName;
			m_fnName = funcName;
			m_findFn = findFn;

			m_ord = 0;
		}
	}

	void init(const char* dllName, std::uint16_t ord) noexcept
	{
		if (m_dllName == nullptr)
		{
			m_dllName = dllName;
			m_ord = ord;

			m_fnName = nullptr;
			m_findFn = nullptr;
		}
	}

	[[nodiscard]] IMAGE_THUNK_DATA* findAddr(HMODULE hMod) const noexcept
	{
		if (m_fnName != nullptr && m_findFn != nullptr)
		{
			return m_findFn(hMod, m_dllName, m_fnName);
		}

		if (m_ord != 0)
		{
			return FindDelayLoadThunkInModule(hMod, m_dllName, m_ord);
		}

		return nullptr;
	}
};

template <typename T, typename... InitArgs>
static auto HookFunction(HookData<T>& hookData, T newFn, const char* dllName, InitArgs&&... args) noexcept -> bool
{
	const ModuleHandle moduleComctl(L"comctl32.dll");
	if (!moduleComctl.isLoaded())
	{
		return false;
	}

	if (hookData.m_trueFn == nullptr && hookData.m_ref == 0)
	{
		hookData.init(dllName, std::forward<InitArgs>(args)...);

		auto* addr = hookData.findAddr(moduleComctl.get());
		if (addr != nullptr)
		{
			hookData.m_trueFn = ReplaceFunction<T>(addr, newFn);
		}
	}

	if (hookData.m_trueFn != nullptr)
	{
		++hookData.m_ref;
		return true;
	}
	return false;
}

template <typename T>
static void UnhookFunction(HookData<T>& hookData) noexcept
{
	const ModuleHandle moduleComctl(L"comctl32.dll");
	if (!moduleComctl.isLoaded())
	{
		return;
	}

	if (hookData.m_ref > 0)
	{
		--hookData.m_ref;

		if (hookData.m_trueFn != nullptr && hookData.m_ref == 0)
		{
			auto* addr = hookData.findAddr(moduleComctl.get());
			if (addr != nullptr)
			{
				ReplaceFunction<T>(addr, hookData.m_trueFn);
				hookData.m_trueFn = nullptr;
			}
		}
	}
}

static HookData<fnGetThemeColor> g_hookDataGetThemeColor{};
static HookData<fnDrawThemeBackgroundEx> g_hookDataDrawThemeBackgroundEx{};

static constexpr COLORREF kMainInstructionTextClr = RGB(96, 205, 255);
static constexpr COLORREF kOtherTextClr = RGB(255, 255, 255);

static HTHEME g_hDarkTheme = nullptr;

static HRESULT WINAPI MyGetThemeColor(
	HTHEME hTheme,
	int iPartId,
	int iStateId,
	int iPropId,
	COLORREF* pColor
) noexcept
{
	const HRESULT retVal = g_hookDataGetThemeColor.m_trueFn(hTheme, iPartId, iStateId, iPropId, pColor);
	if (!g_darkModeEnabled || pColor == nullptr)
	{
		return retVal;
	}

	if (iPropId == TMT_TEXTCOLOR)
	{
		switch (iPartId)
		{
			case TDLG_MAININSTRUCTIONPANE:
			{
				*pColor = kMainInstructionTextClr;
				break;
			}

			case TDLG_CONTENTPANE:
			case TDLG_EXPANDOTEXT:
			case TDLG_VERIFICATIONTEXT:
			case TDLG_FOOTNOTEPANE:
			case TDLG_EXPANDEDFOOTERAREA:
			{
				if (g_hDarkTheme != nullptr)
				{
					g_hookDataGetThemeColor.m_trueFn(g_hDarkTheme, iPartId, iStateId, iPropId, pColor);
				}
				else
				{
					*pColor = kOtherTextClr;
				}
				break;
			}

			default:
			{
				break;
			}
		}
	}
	return retVal;
}

static constexpr std::uint16_t kDrawThemeBackgroundExOrdinal = 47;

static constexpr COLORREF kMainPaneBgClr = RGB(44, 44, 44);
static constexpr COLORREF kFooterBgClr = RGB(32, 32, 32);

static HBRUSH g_hBrushBg = nullptr;
static HBRUSH g_hBrushBgFooter = nullptr;

static HRESULT WINAPI MyDrawThemeBackgroundEx(
	HTHEME hTheme,
	HDC hdc,
	int iPartId,
	int iStateId,
	LPCRECT pRect,
	const DTBGOPTS* pOptions
) noexcept
{
	if (!g_darkModeEnabled || pOptions == nullptr)
	{
		return g_hookDataDrawThemeBackgroundEx.m_trueFn(hTheme, hdc, iPartId, iStateId, pRect, pOptions);
	}

	switch (iPartId)
	{
		case TDLG_PRIMARYPANEL:
		{
			::FillRect(hdc, pRect, g_hBrushBg);
			break;
		}

		case TDLG_SECONDARYPANEL:
		case TDLG_FOOTNOTEPANE:
		{
			::FillRect(hdc, &pOptions->rcClip, g_hBrushBgFooter);
			break;
		}

		default:
		{
			return g_hookDataDrawThemeBackgroundEx.m_trueFn(hTheme, hdc, iPartId, iStateId, pRect, pOptions);
		}
	}
	return S_OK;
}

bool HookThemeColor() noexcept
{
	COLORREF clrMain = kMainPaneBgClr;
	COLORREF clrFooter = kFooterBgClr;

	if (IsWindows11() && g_hDarkTheme == nullptr)
	{
		g_hDarkTheme = ::OpenThemeData(nullptr, L"DarkMode_Explorer::TaskDialog");
		if (g_hDarkTheme != nullptr)
		{
			if (FAILED(::GetThemeColor(g_hDarkTheme, TDLG_PRIMARYPANEL, 0, TMT_FILLCOLOR, &clrMain)))
			{
				clrMain = kMainPaneBgClr;
			}

			if (FAILED(::GetThemeColor(g_hDarkTheme, TDLG_SECONDARYPANEL, 0, TMT_FILLCOLOR, &clrFooter)))
			{
				clrFooter = kFooterBgClr;
			}
		}
	}

	if (g_hBrushBg == nullptr)
	{
		g_hBrushBg = ::CreateSolidBrush(clrMain);
	}

	if (g_hBrushBgFooter == nullptr)
	{
		g_hBrushBgFooter = ::CreateSolidBrush(clrFooter);
	}

	return
		HookFunction<fnGetThemeColor>(g_hookDataGetThemeColor,
			MyGetThemeColor,
			"uxtheme.dll",
			static_cast<const char*>("GetThemeColor"),
			static_cast<fnFindThunkInModule>(FindDelayLoadThunkInModule))
		&& HookFunction<fnDrawThemeBackgroundEx>(g_hookDataDrawThemeBackgroundEx,
			MyDrawThemeBackgroundEx,
			"uxtheme.dll",
			kDrawThemeBackgroundExOrdinal);
}


void UnhookThemeColor() noexcept
{
	UnhookFunction<fnGetThemeColor>(g_hookDataGetThemeColor);
	UnhookFunction<fnDrawThemeBackgroundEx>(g_hookDataDrawThemeBackgroundEx);
	if (g_hDarkTheme != nullptr && g_hookDataGetThemeColor.m_ref == 0)
	{
		::CloseThemeData(g_hDarkTheme);
		g_hDarkTheme = nullptr;
	}

	if (g_hBrushBg != nullptr)
	{
		::DeleteObject(g_hBrushBg);
		g_hBrushBg = nullptr;
	}

	if (g_hBrushBgFooter != nullptr)
	{
		::DeleteObject(g_hBrushBgFooter);
		g_hBrushBgFooter = nullptr;
	}
}
