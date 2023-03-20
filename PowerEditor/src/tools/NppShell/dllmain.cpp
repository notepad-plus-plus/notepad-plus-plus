#include "pch.h"

#include "CommandHandlerFactory.h"

using namespace NppShell::Factories;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);

    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow(void)
{
    if (winrt::get_module_lock())
    {
        return S_FALSE;
    }

    winrt::clear_factory_cache();
    return S_OK;
}

_Check_return_
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    if (riid != IID_IClassFactory && riid != IID_IUnknown)
    {
        return E_NOINTERFACE;
    }

    if (rclsid != __uuidof(CommandHandlerFactory))
    {
        return E_INVALIDARG;
    }

    try
    {
        return winrt::make<CommandHandlerFactory>()->QueryInterface(riid, ppv);
    }
    catch (...)
    {
        return winrt::to_hresult();
    }
}