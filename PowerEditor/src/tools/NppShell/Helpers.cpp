#include "pch.h"
#include "Helpers.h"

using namespace NppShell::Helpers;

extern HMODULE thisModule;

const wstring NppShell::Helpers::GetInstallationPath()
{
    wchar_t path[FILENAME_MAX] = { 0 };
    GetModuleFileName(thisModule, path, FILENAME_MAX);
    return std::filesystem::path(path).parent_path().wstring();
}