#pragma once
#include "pch.h"

namespace NppModernShell::Factories
{
    class __declspec(uuid("4EACAA14-3B43-4595-A44C-FBA8F0848620")) CommandHandlerFactory : public winrt::implements<NppModernShell::Factories::CommandHandlerFactory, IClassFactory>
    {
    public:
        IFACEMETHODIMP CreateInstance(_In_opt_ IUnknown* pUnkOuter, _In_ REFIID riid, _COM_Outptr_ void** ppvObject) noexcept override;
        IFACEMETHODIMP LockServer(_In_ BOOL fLock) noexcept override;
    };
}