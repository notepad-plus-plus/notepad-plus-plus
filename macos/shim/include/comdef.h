#pragma once
// Win32 Shim: COM definitions for macOS
// Minimal stubs for _com_error and related types

#include "windef.h"
#include <stdexcept>
#include <string>

// _com_error - simplified COM error class
class _com_error {
public:
	_com_error(HRESULT hr) : _hr(hr) {}
	HRESULT Error() const { return _hr; }
	const wchar_t* ErrorMessage() const { return L"COM Error"; }
private:
	HRESULT _hr;
};
