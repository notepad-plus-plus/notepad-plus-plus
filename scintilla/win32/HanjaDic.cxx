// Scintilla source code edit control
/** @file HanjaDic.cxx
 ** Korean Hanja Dictionary
 ** Convert between Korean Hanja and Hangul by COM interface.
 **/
// Copyright 2015 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <windows.h>

#include "UniConversion.h"
#include "HanjaDic.h"

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

namespace HanjaDict {

interface IRadical;
interface IHanja;
interface IStrokes;

typedef enum { HANJA_UNKNOWN = 0, HANJA_K0 = 1, HANJA_K1 = 2, HANJA_OTHER = 3 } HANJA_TYPE;

interface IHanjaDic : IUnknown {
	STDMETHOD(OpenMainDic)();
	STDMETHOD(CloseMainDic)();
	STDMETHOD(GetHanjaWords)(BSTR bstrHangul, SAFEARRAY* ppsaHanja, VARIANT_BOOL* pfFound);
	STDMETHOD(GetHanjaChars)(unsigned short wchHangul, BSTR* pbstrHanjaChars, VARIANT_BOOL* pfFound);
	STDMETHOD(HanjaToHangul)(BSTR bstrHanja, BSTR* pbstrHangul);
	STDMETHOD(GetHanjaType)(unsigned short wchHanja, HANJA_TYPE* pHanjaType);
	STDMETHOD(GetHanjaSense)(unsigned short wchHanja, BSTR* pbstrSense);
	STDMETHOD(GetRadicalID)(short SeqNumOfRadical, short* pRadicalID, unsigned short* pwchRadical);
	STDMETHOD(GetRadical)(short nRadicalID, IRadical** ppIRadical);
	STDMETHOD(RadicalIDToHanja)(short nRadicalID, unsigned short* pwchRadical);
	STDMETHOD(GetHanja)(unsigned short wchHanja, IHanja** ppIHanja);
	STDMETHOD(GetStrokes)(short nStrokes, IStrokes** ppIStrokes);
	STDMETHOD(OpenDefaultCustomDic)();
	STDMETHOD(OpenCustomDic)(BSTR bstrPath, long* plUdr);
	STDMETHOD(CloseDefaultCustomDic)();
	STDMETHOD(CloseCustomDic)(long lUdr);
	STDMETHOD(CloseAllCustomDics)();
	STDMETHOD(GetDefaultCustomHanjaWords)(BSTR bstrHangul, SAFEARRAY** ppsaHanja, VARIANT_BOOL* pfFound);
	STDMETHOD(GetCustomHanjaWords)(long lUdr, BSTR bstrHangul, SAFEARRAY** ppsaHanja, VARIANT_BOOL* pfFound);
	STDMETHOD(PutDefaultCustomHanjaWord)(BSTR bstrHangul, BSTR bstrHanja);
	STDMETHOD(PutCustomHanjaWord)(long lUdr, BSTR bstrHangul, BSTR bstrHanja);
	STDMETHOD(MaxNumOfRadicals)(short* pVal);
	STDMETHOD(MaxNumOfStrokes)(short* pVal);
	STDMETHOD(DefaultCustomDic)(long* pVal);
	STDMETHOD(DefaultCustomDic)(long pVal);
	STDMETHOD(MaxHanjaType)(HANJA_TYPE* pHanjaType);
	STDMETHOD(MaxHanjaType)(HANJA_TYPE pHanjaType);
};

extern "C" const GUID __declspec(selectany) IID_IHanjaDic =
{ 0xad75f3ac, 0x18cd, 0x48c6, { 0xa2, 0x7d, 0xf1, 0xe9, 0xa7, 0xdc, 0xe4, 0x32 } };

class HanjaDic {
private:
	HRESULT hr;
	CLSID CLSID_HanjaDic;

public:
	IHanjaDic *HJinterface;

	HanjaDic() : HJinterface(NULL) {
		hr = CLSIDFromProgID(OLESTR("mshjdic.hanjadic"), &CLSID_HanjaDic);
		if (SUCCEEDED(hr)) {
			hr = CoCreateInstance(CLSID_HanjaDic, NULL,
					CLSCTX_INPROC_SERVER, IID_IHanjaDic,
					(LPVOID *)& HJinterface);
			if (SUCCEEDED(hr)) {
				hr = HJinterface->OpenMainDic();
			}
		}
	}

	~HanjaDic() {
		if (SUCCEEDED(hr)) {
			hr = HJinterface->CloseMainDic();
			HJinterface->Release();
		}
	}

	bool HJdictAvailable() {
		return SUCCEEDED(hr);
	}

	bool IsHanja(int hanja) {
		HANJA_TYPE hanjaType;
		hr = HJinterface->GetHanjaType(static_cast<unsigned short>(hanja), &hanjaType);
		if (SUCCEEDED(hr)) {
			return (hanjaType > 0);
		}
		return false;
	}
};

int GetHangulOfHanja(wchar_t *inout) {
	// Convert every hanja to hangul.
	// Return the number of characters converted.
	int changed = 0;
	HanjaDic dict;
	if (dict.HJdictAvailable()) {
		const size_t len = wcslen(inout);
		wchar_t conv[UTF8MaxBytes] = {0};
		BSTR bstrHangul = SysAllocString(conv);
		for (size_t i=0; i<len; i++) {
			if (dict.IsHanja(static_cast<int>(inout[i]))) { // Pass hanja only!
				conv[0] = inout[i];
				BSTR bstrHanja = SysAllocString(conv);
				HRESULT hr = dict.HJinterface->HanjaToHangul(bstrHanja, &bstrHangul);
				if (SUCCEEDED(hr)) {
					inout[i] = static_cast<wchar_t>(bstrHangul[0]);
					changed += 1;
				}
				SysFreeString(bstrHanja);
			}
		}
		SysFreeString(bstrHangul);
	}
	return changed;
}

}
#ifdef SCI_NAMESPACE
}
#endif
