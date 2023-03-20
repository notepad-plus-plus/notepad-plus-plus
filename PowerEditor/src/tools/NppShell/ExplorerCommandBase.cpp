#include "pch.h"
#include "ExplorerCommandBase.h"

using namespace NppShell::CommandHandlers;

const EXPCMDFLAGS ExplorerCommandBase::Flags()
{
    return ECF_DEFAULT;
}

const EXPCMDSTATE ExplorerCommandBase::State(_In_opt_ IShellItemArray* selection)
{
    UNREFERENCED_PARAMETER(selection);

    return ECS_ENABLED;
}

IFACEMETHODIMP ExplorerCommandBase::GetTitle(_In_opt_ IShellItemArray* items, _Outptr_result_nullonfailure_ PWSTR* name)
{
    UNREFERENCED_PARAMETER(items);

    *name = nullptr;
    auto str = wil::make_cotaskmem_string_nothrow(Title().c_str());
    RETURN_IF_NULL_ALLOC(str);
    *name = str.release();
    return S_OK;
}

IFACEMETHODIMP ExplorerCommandBase::GetIcon(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* icon)
{
    *icon = nullptr;
    auto str = wil::make_cotaskmem_string_nothrow(Icon().c_str());
    RETURN_IF_NULL_ALLOC(str);
    *icon = str.release();
    return S_OK;
}

IFACEMETHODIMP ExplorerCommandBase::GetToolTip(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* infoTip)
{
    *infoTip = nullptr;
    return E_NOTIMPL;
}

IFACEMETHODIMP ExplorerCommandBase::GetState(_In_opt_ IShellItemArray* selection, _In_ BOOL okToBeSlow, _Out_ EXPCMDSTATE* cmdState)
{
    UNREFERENCED_PARAMETER(okToBeSlow);

    *cmdState = State(selection);
    return S_OK;
}

IFACEMETHODIMP ExplorerCommandBase::GetFlags(_Out_ EXPCMDFLAGS* flags)
{
    *flags = Flags();
    return S_OK;
}

IFACEMETHODIMP ExplorerCommandBase::GetCanonicalName(_Out_ GUID* guidCommandName)
{
    *guidCommandName = GUID_NULL;
    return E_NOTIMPL;
}

IFACEMETHODIMP ExplorerCommandBase::EnumSubCommands(_COM_Outptr_ IEnumExplorerCommand** enumCommands)
{
    *enumCommands = nullptr;
    return E_NOTIMPL;
}