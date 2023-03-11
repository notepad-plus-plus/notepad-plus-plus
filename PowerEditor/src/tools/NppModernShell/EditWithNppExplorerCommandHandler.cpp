#include "pch.h"
#include "EditWithNppExplorerCommandHandler.h"

#include "Helpers.h"

using namespace NppModernShell::CommandHandlers;
using namespace NppModernShell::Helpers;

const wstring EditWithNppExplorerCommandHandler::GetNppExecutableFullPath()
{
    const wstring path = GetInstallationPath();
    const wstring fileName = L"\\notepad++.exe";

    return L"\"" + path + fileName + L"\"";
}

const wstring EditWithNppExplorerCommandHandler::Title()
{
    return L"Edit with Notepad++";
}

const wstring EditWithNppExplorerCommandHandler::Icon()
{
    const wstring fileName = GetNppExecutableFullPath();

    return fileName;
}

const wstring EditWithNppExplorerCommandHandler::GetCommandLine()
{
    const wstring fileName = GetNppExecutableFullPath();
    const wstring parameters = L"\"%1\"";

    return fileName + L" " + parameters;
}

IFACEMETHODIMP EditWithNppExplorerCommandHandler::Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) noexcept try
{
    if (!selection)
    {
        return S_OK;
    }

    DWORD count;
    RETURN_IF_FAILED(selection->GetCount(&count));

    IShellItem* psi;
    LPWSTR itemName;

    for (DWORD i = 0; i < count; ++i)
    {
        selection->GetItemAt(i, &psi);
        RETURN_IF_FAILED(psi->GetDisplayName(SIGDN_FILESYSPATH, &itemName));

        std::wstring cmdline = this->GetCommandLine();
        cmdline = cmdline.replace(cmdline.find(L"%1"), 2, itemName);

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        wchar_t* command = (LPWSTR)cmdline.c_str();

        if (!CreateProcess(nullptr, command, nullptr, nullptr, false, CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &si, &pi))
        {
            return S_OK;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return S_OK;
}
CATCH_RETURN();