#pragma once
// Win32 Shim: COM smart pointer helpers for macOS
// Minimal stubs for _com_ptr_t and _com_IIID

#include "windef.h"

// __uuidof emulation for macOS
// On Windows, __uuidof(T) returns the IID of the COM interface.
// We provide a template-based emulation.
template<typename T> struct __uuidof_helper;

// Specializations are provided in shobjidl.h via static GUIDs.
// For the general case, return IID_NULL.
#ifndef __uuidof
#define __uuidof(x) __uuidof_helper<std::remove_pointer_t<std::decay_t<decltype(x)>>>::iid()
#endif

// _com_IIID template - binds an interface type to its IID
template<class T, const GUID* piid>
struct _com_IIID {
	typedef T Interface;
	static const GUID& GetIID() { return *piid; }
};

// _com_ptr_t - simplified COM smart pointer
// On macOS, COM objects won't actually exist, so this is a no-op wrapper.
template<class IIID>
class _com_ptr_t {
public:
	typedef typename IIID::Interface Interface;

	_com_ptr_t() : _p(nullptr) {}
	_com_ptr_t(std::nullptr_t) : _p(nullptr) {}
	_com_ptr_t(Interface* p) : _p(p) { if (_p) _p->AddRef(); }
	_com_ptr_t(const _com_ptr_t& other) : _p(other._p) { if (_p) _p->AddRef(); }
	~_com_ptr_t() { if (_p) _p->Release(); }

	_com_ptr_t& operator=(Interface* p) {
		if (_p) _p->Release();
		_p = p;
		if (_p) _p->AddRef();
		return *this;
	}
	_com_ptr_t& operator=(const _com_ptr_t& other) {
		if (this != &other) {
			if (_p) _p->Release();
			_p = other._p;
			if (_p) _p->AddRef();
		}
		return *this;
	}
	_com_ptr_t& operator=(std::nullptr_t) {
		if (_p) _p->Release();
		_p = nullptr;
		return *this;
	}

	Interface* operator->() const { return _p; }
	Interface& operator*() const { return *_p; }
	Interface** operator&() { if (_p) { _p->Release(); _p = nullptr; } return &_p; }
	operator Interface*() const { return _p; }
	operator bool() const { return _p != nullptr; }
	bool operator!() const { return _p == nullptr; }

	Interface* GetInterfacePtr() const { return _p; }

	HRESULT CreateInstance(const CLSID& clsid, IUnknown* pOuter = nullptr, DWORD dwClsContext = CLSCTX_INPROC_SERVER) {
		return CoCreateInstance(clsid, pOuter, dwClsContext, IIID::GetIID(), reinterpret_cast<void**>(&_p));
	}

	template<class Q>
	HRESULT QueryInterface(const IID& iid, Q** pp) const {
		if (!_p) return E_POINTER;
		return _p->QueryInterface(iid, reinterpret_cast<void**>(pp));
	}

private:
	Interface* _p;
};
