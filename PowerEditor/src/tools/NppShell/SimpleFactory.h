#pragma once
#include "pch.h"

namespace NppShell::Factories
{
	template<class T>
	struct SimpleFactory : winrt::implements<SimpleFactory<T>, IClassFactory>
	{
		IFACEMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvObject) override try
		{
			*ppvObject = nullptr;

			if (!pUnkOuter)
			{
				return winrt::make<T>().as(riid, ppvObject);
			}
			else
			{
				return CLASS_E_NOAGGREGATION;
			}
		}
		catch (...)
		{
			return winrt::to_hresult();
		}

		IFACEMETHODIMP LockServer(BOOL) noexcept override
		{
			return S_OK;
		}
	};
}