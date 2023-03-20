#include "pch.h"
#include "CommandHandlerFactory.h"

#include "EditWithNppExplorerCommandHandler.h"

using namespace NppShell::CommandHandlers;
using namespace NppShell::Factories;

IFACEMETHODIMP CommandHandlerFactory::CreateInstance(_In_opt_ IUnknown* pUnkOuter, _In_ REFIID riid, _COM_Outptr_ void** ppvObject) noexcept
{
    UNREFERENCED_PARAMETER(pUnkOuter);

    try
    {
        return winrt::make<EditWithNppExplorerCommandHandler>()->QueryInterface(riid, ppvObject);
    }
    catch (...)
    {
        return winrt::to_hresult();
    }
}

IFACEMETHODIMP CommandHandlerFactory::LockServer(_In_ BOOL fLock) noexcept
{
    if (fLock)
    {
        ++winrt::get_module_lock();
    }
    else
    {
        --winrt::get_module_lock();
    }

    return S_OK;
}