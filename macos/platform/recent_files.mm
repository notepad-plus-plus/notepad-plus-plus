// recent_files.mm — Recent files list management
// Part of the Notepad++ macOS port modular refactor.

#include "recent_files.h"
#include "npp_constants.h"
#include "app_state.h"
#include "string_utils.h"
#include "windows.h"

// Forward declaration — defined in file_operations.mm
bool openFileAtPath(NSString* path);

void addRecentFile(const std::wstring& path)
{
	auto it = std::find(ctx().recentFiles.begin(), ctx().recentFiles.end(), path);
	if (it != ctx().recentFiles.end())
		ctx().recentFiles.erase(it);

	ctx().recentFiles.insert(ctx().recentFiles.begin(), path);

	if (static_cast<int>(ctx().recentFiles.size()) > ctx().MAX_RECENT_FILES)
		ctx().recentFiles.resize(ctx().MAX_RECENT_FILES);
}

void rebuildRecentMenu()
{
	if (!ctx().recentMenu) return;

	int itemCount = GetMenuItemCount(ctx().recentMenu);
	for (int i = itemCount - 1; i >= 0; --i)
		DeleteMenu(ctx().recentMenu, i, MF_BYPOSITION);

	if (ctx().recentFiles.empty())
	{
		AppendMenuW(ctx().recentMenu, MF_STRING | MF_GRAYED, 0, L"(No recent files)");
		return;
	}

	for (int i = 0; i < static_cast<int>(ctx().recentFiles.size()); ++i)
	{
		std::wstring display = ctx().recentFiles[i];
		size_t lastSlash = display.rfind(L'/');
		if (lastSlash != std::wstring::npos)
			display = display.substr(lastSlash + 1);

		wchar_t label[300];
		swprintf(label, 300, L"&%d %ls", i + 1, display.c_str());
		AppendMenuW(ctx().recentMenu, MF_STRING, IDM_FILE_RECENT_BASE + i, label);
	}

	AppendMenuW(ctx().recentMenu, MF_SEPARATOR, 0, nullptr);
	AppendMenuW(ctx().recentMenu, MF_STRING, IDM_FILE_RECENT_CLEAR, L"Clear Recent Files List");
}

void openRecentFile(int index)
{
	if (index < 0 || index >= static_cast<int>(ctx().recentFiles.size()))
		return;

	std::wstring wpath = ctx().recentFiles[index];
	NSString* path = WideToNSString(wpath.c_str());

	if (!openFileAtPath(path))
	{
		ctx().recentFiles.erase(ctx().recentFiles.begin() + index);
		rebuildRecentMenu();
	}
}
