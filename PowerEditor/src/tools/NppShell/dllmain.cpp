#include "pch.h"

#include "Installer.h"
#include "SimpleFactory.h"
#include "EditWithNppExplorerCommandHandler.h"

using namespace NppShell::CommandHandlers;
using namespace NppShell::Factories;

HMODULE thisModule;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            thisModule = hModule;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

STDAPI DllRegisterServer()
{
    return NppShell::Installer::Install();
}

STDAPI DllUnregisterServer()
{
    return NppShell::Installer::Uninstall();
}

__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow(void)
{
    if (winrt::get_module_lock())
    {
        return S_FALSE;
    }
    else
    {
        return S_OK;
    }
}

_Use_decl_annotations_ STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) try
{
    *ppv = nullptr;

    if (rclsid == __uuidof(EditWithNppExplorerCommandHandler))
    {
        return winrt::make<SimpleFactory<EditWithNppExplorerCommandHandler>>().as(riid, ppv);
    }
    else
    {
        return CLASS_E_CLASSNOTAVAILABLE;
    }
}
catch (...)
{
    return winrt::to_hresult();
}