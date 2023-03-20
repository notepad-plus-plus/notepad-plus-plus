#include "pch.h"
#include "ExplorerCommandBase.h"

using namespace NppShell::CommandHandlers;

const EXPCMDFLAGS ExplorerCommandBase::Flags()
{
    return ECF_DEFAULT;
}

const EXPCMDSTATE ExplorerCommandBase::State(IShellItemArray* psiItemArray)
{
    UNREFERENCED_PARAMETER(psiItemArray);

    return ECS_ENABLED;
}

IFACEMETHODIMP ExplorerCommandBase::GetTitle(IShellItemArray* psiItemArray, LPWSTR* ppszName)
{
    UNREFERENCED_PARAMETER(psiItemArray);

    wstring title = Title();
    SHStrDup(title.data(), ppszName);

    return S_OK;
}

IFACEMETHODIMP ExplorerCommandBase::GetIcon(IShellItemArray* psiItemArray, LPWSTR* ppszIcon)
{
    UNREFERENCED_PARAMETER(psiItemArray);

    wstring icon = Icon();
    SHStrDup(icon.data(), ppszIcon);

    return S_OK;
}

IFACEMETHODIMP ExplorerCommandBase::GetToolTip(IShellItemArray* psiItemArray, LPWSTR* ppszInfotip)
{
    UNREFERENCED_PARAMETER(psiItemArray);
    UNREFERENCED_PARAMETER(ppszInfotip);

    return E_NOTIMPL;
}

IFACEMETHODIMP ExplorerCommandBase::GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState)
{
    UNREFERENCED_PARAMETER(fOkToBeSlow);

    *pCmdState = State(psiItemArray);
    return S_OK;
}

IFACEMETHODIMP ExplorerCommandBase::GetFlags(EXPCMDFLAGS* flags)
{
    *flags = Flags();
    return S_OK;
}

IFACEMETHODIMP ExplorerCommandBase::GetCanonicalName(GUID* pguidCommandName) {
    *pguidCommandName = GUID_NULL;
    return S_OK;
}

IFACEMETHODIMP ExplorerCommandBase::EnumSubCommands(IEnumExplorerCommand** ppEnum)
{
    *ppEnum = nullptr;
    return E_NOTIMPL;
}