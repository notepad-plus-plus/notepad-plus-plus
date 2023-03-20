#include "pch.h"
#include "Helpers.h"

using namespace NppShell::Helpers;

const HMODULE GetThisModule()
{
    HMODULE hm = NULL;

    BOOL result = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&GetInstallationPath, &hm);

    if (result == FALSE)
    {
        throw "Failed to locate current module, unable to proceed";
    }

    return hm;
}

const wstring NppShell::Helpers::GetInstallationPath()
{
    HMODULE thisModule = GetThisModule();

    wchar_t path[FILENAME_MAX] = { 0 };
    GetModuleFileName(thisModule, path, FILENAME_MAX);
    return std::filesystem::path(path).parent_path().wstring();
}