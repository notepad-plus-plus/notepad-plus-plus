// Windows regression tests for bidirectional drawing and hit testing.
// Uses an off-screen Scintilla control: no desktop window is shown.
#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "Scintilla.h"
#include "../src/BidiClass.h"

namespace {
int assertions = 0;
void Check(bool condition, const char *message) {
	++assertions;
	if (!condition) {
		std::fprintf(stderr, "FAIL: %s\n", message);
		std::exit(1);
	}
}

struct Control {
	static constexpr int width = 640;
	static constexpr int height = 200;
	HWND window = CreateWindowExW(0, L"Scintilla", L"", WS_POPUP,
		0, 0, width, height, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
	HDC dc = CreateCompatibleDC(nullptr);
	HBITMAP bitmap = nullptr;
	HGDIOBJ previous = nullptr;
	uint32_t *pixels = nullptr;
	Control() {
		Check(window != nullptr, "create test control");
		BITMAPINFO info {};
		info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info.bmiHeader.biWidth = width;
		info.bmiHeader.biHeight = -height;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS,
			reinterpret_cast<void **>(&pixels), nullptr, 0);
		previous = SelectObject(dc, bitmap);
		Send(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITEDC);
		Check(Send(SCI_GETTECHNOLOGY) == SC_TECHNOLOGY_DIRECTWRITEDC, "DirectWrite available");
		Send(SCI_SETCODEPAGE, SC_CP_UTF8);
		Send(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<LPARAM>("Segoe UI"));
		Send(SCI_STYLESETSIZE, STYLE_DEFAULT, 16);
		Send(SCI_STYLESETFORE, STYLE_DEFAULT, RGB(0, 0, 0));
		Send(SCI_STYLESETBACK, STYLE_DEFAULT, RGB(255, 255, 255));
		Send(SCI_STYLECLEARALL);
		Send(SCI_SETFONTQUALITY, SC_EFF_QUALITY_NON_ANTIALIASED);
		Send(SCI_SETMARGINLEFT, 0, 11);
		Send(SCI_SETMARGINRIGHT, 0, 7);
		for (int margin = 0; margin < 5; margin++)
			Send(SCI_SETMARGINWIDTHN, margin, margin == 0 ? 29 : 0);
		Send(SCI_SETCARETSTYLE, CARETSTYLE_INVISIBLE);
		Send(SCI_SETSELBACK, TRUE, RGB(0, 120, 255));
		Send(SCI_SETSELFORE, FALSE);
		Send(SCI_SETSELEOLFILLED, FALSE);
		for (int element = SC_ELEMENT_SELECTION_TEXT; element <= SC_ELEMENT_SELECTION_INACTIVE_ADDITIONAL_BACK; element++)
			Send(SCI_SETELEMENTCOLOUR, element, (element & 1) ?
				static_cast<LPARAM>(RGB(0, 120, 255) | 0xff000000UL) : 0xff000000UL);
	}
	~Control() {
		DestroyWindow(window);
		SelectObject(dc, previous);
		DeleteObject(bitmap);
		DeleteDC(dc);
	}
	LRESULT Send(UINT message, WPARAM w = 0, LPARAM l = 0) {
		return SendMessageW(window, message, w, l);
	}
	std::vector<uint32_t> Paint() {
		std::fill_n(pixels, width * height, 0xff00ff);
		Send(WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
		GdiFlush();
		return {pixels, pixels + width * height};
	}
};

bool Ink(uint32_t pixel) {
	return (pixel & 0xffffff) == 0;
}

std::vector<int> Boundaries(const std::string &text) {
	std::vector<int> boundaries;
	for (int i = 0; i < static_cast<int>(text.size()); i++)
		if ((static_cast<unsigned char>(text[i]) & 0xc0) != 0x80)
			boundaries.push_back(i);
	boundaries.push_back(static_cast<int>(text.size()));
	return boundaries;
}

void SelectionDoesNotReorder(Control &control, const std::string &text, int direction, int phases) {
	control.Send(SCI_SETBIDIRECTIONAL, direction);
	control.Send(SCI_SETPHASESDRAW, phases);
	control.Send(SCI_SETWRAPMODE, SC_WRAP_NONE);
	control.Send(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(text.c_str()));
	control.Send(SCI_SETSEL, 0, 0);
	const std::vector<uint32_t> original = control.Paint();
	Check(std::any_of(original.begin(), original.end(), Ink), "test paints text");
	// Artificial syntax-style boundaries must not reshape the line either.
	control.Send(SCI_STARTSTYLING, 0);
	for (size_t position = 0; position < text.size();) {
		size_t end = position + 1;
		while (end < text.size() && (static_cast<unsigned char>(text[end]) & 0xc0) == 0x80)
			++end;
		control.Send(SCI_SETSTYLING, end - position, (position & 1) ? 1 : 0);
		position = end;
	}
	const std::vector<uint32_t> styled = control.Paint();
	Check(std::equal(original.begin(), original.end(), styled.begin(),
		[](uint32_t a, uint32_t b) { return Ink(a) == Ink(b); }),
		"syntax-style boundaries preserve glyph placement");
	// Mouse selections stop at shaped cluster boundaries (including niqqud
	// and emoji). Programmatic selections may split a cluster deliberately.
	std::vector<int> boundaries;
	for (int x = 40; x < Control::width - 7; x++) {
		const int position = static_cast<int>(control.Send(SCI_CHARPOSITIONFROMPOINTCLOSE, x, 1));
		if (position >= 0 && position < static_cast<int>(text.size()))
			boundaries.push_back(position);
	}
	boundaries.push_back(static_cast<int>(text.size()));
	std::sort(boundaries.begin(), boundaries.end());
	boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
	int left = Control::width;
	int right = 0;
	for (int boundary : boundaries) {
		const int x = static_cast<int>(control.Send(SCI_POINTXFROMPOSITION, 0, boundary));
		left = std::min(left, x);
		right = std::max(right, x);
	}
	const int y = static_cast<int>(control.Send(SCI_TEXTHEIGHT, 0)) - 1;
	std::vector<int> characters(Control::width, -1);
	for (int x = std::max(41, left + 2); x < std::min(Control::width - 8, right - 1); x++) {
		const LRESULT character = control.Send(SCI_CHARPOSITIONFROMPOINTCLOSE, x, y);
		if (character >= 0 && character < static_cast<LRESULT>(text.size()) &&
			control.Send(SCI_CHARPOSITIONFROMPOINTCLOSE, x - 1, y) == character &&
			control.Send(SCI_CHARPOSITIONFROMPOINTCLOSE, x + 1, y) == character)
			characters[x] = static_cast<int>(character);
	}
	for (size_t a = 0; a + 1 < boundaries.size(); a++) {
		for (size_t b = a + 1; b < boundaries.size(); b++) {
			if (a != 0 && b + 1 != boundaries.size() && b > a + 2)
				continue;
			control.Send(SCI_SETSEL, boundaries[a], boundaries[b]);
			std::vector<char> selectedText(boundaries[b] - boundaries[a] + 1);
			control.Send(SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(selectedText.data()));
			Check(std::string(selectedText.data()) == text.substr(boundaries[a], boundaries[b] - boundaries[a]),
				"selected text preserves logical order for copying");
			const std::vector<uint32_t> selected = control.Paint();
			for (size_t pixel = 0; pixel < original.size(); pixel++) {
				if (Ink(original[pixel]) != Ink(selected[pixel])) {
					std::fprintf(stderr, "direction=%d phases=%d range=%d..%d pixel=%zu text=%s\n",
						direction, phases, boundaries[a], boundaries[b], pixel, text.c_str());
					Check(false, "selection must preserve glyph placement");
				}
			}
			Check(true, "selection preserves glyph placement");
			for (int x = 41; x < Control::width - 8; x++) {
				if (x <= left + 1 || x >= right - 1)
					continue;
				const int character = characters[x];
				if (character < 0 || character >= static_cast<LRESULT>(text.size()))
					continue;
				// Background rectangles snap to pixels, while DirectWrite hit tests
				// use fractional coordinates. Exclude the one-pixel boundary band.
				const bool expected = character >= boundaries[a] && character < boundaries[b];
				const uint32_t pixel = selected[y * Control::width + x] & 0xffffff;
				if (pixel != 0xffffff && pixel != 0x0078ff)
					continue;
				if ((pixel == 0x0078ff) != expected) {
					std::fprintf(stderr, "direction=%d phases=%d range=%d..%d point=%d,%d char=%lld pixel=%06x text=%s\n",
						direction, phases, boundaries[a], boundaries[b], x, y,
						static_cast<long long>(character), pixel, text.c_str());
					Check(false, "selection background agrees with mouse character");
				}
				Check(true, "selection background agrees with mouse character");
			}
		}
	}
	std::printf("PASS direction=%d phases=%d text=%s\n", direction, phases, text.c_str());
	std::fflush(stdout);
}

void WrappedSelection(Control &control) {
	SetWindowPos(control.window, nullptr, 0, 0, 230, Control::height, SWP_NOACTIVATE | SWP_NOZORDER);
	for (int direction : {SC_BIDIRECTIONAL_L2R, SC_BIDIRECTIONAL_R2L}) {
		control.Send(SCI_SETBIDIRECTIONAL, direction);
		control.Send(SCI_SETWRAPMODE, SC_WRAP_WORD);
		control.Send(SCI_SETWRAPSTARTINDENT, 2);
		const std::string text = u8"abc אבג def 123 שלום world אבג xyz 456 שלום\r\nאבג abc\r\n\r\n";
		control.Send(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(text.c_str()));
		control.Send(SCI_SETSEL, 0, 0);
		const auto original = control.Paint();
		Check(control.Send(SCI_WRAPCOUNT, 0) > 1, "text actually wraps");
		for (int end : Boundaries(text)) {
			control.Send(SCI_SETSEL, 0, end);
			const auto selected = control.Paint();
			Check(std::equal(original.begin(), original.end(), selected.begin(),
				[](uint32_t a, uint32_t b) { return Ink(a) == Ink(b); }),
				"wrapped selection preserves glyph positions");
		}
	}
	SetWindowPos(control.window, nullptr, 0, 0, Control::width, Control::height, SWP_NOACTIVATE | SWP_NOZORDER);
	control.Send(SCI_SETWRAPMODE, SC_WRAP_NONE);
	std::printf("PASS wrapped multiline selections\n");
}

void AbsoluteVisualOrder(Control &control) {
	// Each non-space glyph has a distinct colour. Check the actual pixels
	// against a known visual order, independently of the editor's hit tests.
	struct VisualCase {
		std::string text;
		std::vector<int> order;
	};
	const std::vector<VisualCase> cases {
		{u8"כתוב נכון ABC אני עכשיו בודק אם",
			{30, 29, 27, 26, 25, 24, 22, 21, 20, 19, 18, 16, 15, 14,
			10, 11, 12, 8, 7, 6, 5, 3, 2, 1, 0}},
		{u8"abc אבג def", {0, 1, 2, 6, 5, 4, 8, 9, 10}},
		{u8"אבג abc דהו", {10, 9, 8, 4, 5, 6, 2, 1, 0}},
		{u8" 123 אבג ABC", {9, 10, 11, 7, 6, 5, 1, 2, 3}},
	};
	for (const char *font : {"Segoe UI", "Consolas", "Courier New"}) {
		for (const VisualCase &sample : cases) {
			const std::string &text = sample.text;
			const auto boundaries = Boundaries(text);
			control.Send(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<LPARAM>(font));
			control.Send(SCI_STYLECLEARALL);
			for (int direction : {SC_BIDIRECTIONAL_L2R, SC_BIDIRECTIONAL_R2L}) {
				for (int phases : {SC_PHASES_ONE, SC_PHASES_TWO, SC_PHASES_MULTIPLE}) {
					control.Send(SCI_SETBIDIRECTIONAL, direction);
					control.Send(SCI_SETPHASESDRAW, phases);
					control.Send(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(text.c_str()));
					control.Send(SCI_SETSEL, 0, 0);
					control.Send(SCI_STARTSTYLING, 0);
					std::vector<uint32_t> colours;
					for (size_t i = 0; i + 1 < boundaries.size(); ++i) {
						const int red = 60 + static_cast<int>(i) * 5;
						control.Send(SCI_STYLESETFORE, i + 1, RGB(red, 32, 48));
						colours.push_back((red << 16) | (32 << 8) | 48);
						control.Send(SCI_SETSTYLING, boundaries[i + 1] - boundaries[i], i + 1);
					}
					const auto painted = control.Paint();
					std::vector<double> centres(colours.size(), 0);
					std::vector<int> counts(colours.size(), 0);
					for (size_t pixel = 0; pixel < painted.size(); ++pixel) {
						const auto match = std::find(colours.begin(), colours.end(), painted[pixel] & 0xffffff);
						if (match != colours.end()) {
							const size_t i = match - colours.begin();
							centres[i] += pixel % Control::width;
							++counts[i];
						}
					}
					// These are code-point indices in left-to-right visual order.
					double previousX = -1;
					for (int i : sample.order) {
						Check(counts[i] > 0, "coloured glyph actually drawn");
						const double x = centres[i] / counts[i];
						if (x <= previousX) {
							std::fprintf(stderr, "font=%s direction=%d phases=%d glyph=%d text=%s\n",
								font, direction, phases, i, text.c_str());
							Check(false, "first strong character determines painted paragraph order");
						}
						Check(true, "painted mixed-script glyphs follow known visual order");
						previousX = x;
					}
				}
			}
		}
		std::printf("PASS absolute painted glyph order font=%s\n", font);
	}
	control.Send(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<LPARAM>("Segoe UI"));
	control.Send(SCI_STYLECLEARALL);
}

void FirstStrongDirection() {
	using Scintilla::Internal::ParagraphIsRightToLeft;
	for (bool fallback : {false, true}) {
		for (const std::string &text : {u8"כתוב נכון ABC אני עכשיו בודק אם",
			u8" 123 אבג ABC", u8"😀 אבג ABC", u8"\u05B0אבג ABC",
			u8"\u0661\u0662 العربية ABC", u8"\U0001E900 abc",
			u8"\u2066abc\u2069 אבג", u8"\u2068abc\u2069 אבג",
			u8"\u2066a\u2067אבג\u2069b\u2069 אבג", u8"\u200Fabc"}) {
			Check(ParagraphIsRightToLeft(text, fallback), "Hebrew/Arabic first strong determines RTL");
		}
		for (const std::string &text : {u8"abc אבג def", u8" 123 abc אבג", u8"😀 abc אבג",
			u8"\U00010400 אבג", u8"\u2067אבג\u2069 abc", u8"\u200Eאבג"}) {
			Check(!ParagraphIsRightToLeft(text, fallback), "Latin first strong determines LTR");
		}
		for (const std::string &text : {u8"", u8" 123 (456) 😀", u8"\u05B0\u0301",
			u8"\u2066abc", u8"\u2066abc\u2069", u8"\u2069 123"}) {
			Check(ParagraphIsRightToLeft(text, fallback) == fallback, "no outside strong character uses fallback");
		}
	}
	std::printf("PASS first-strong Unicode paragraph direction\n");
}

void WrappedParagraphDirection(Control &control) {
	const std::string text = u8"אבג one two three four five six אבג seven eight nine אבג ten eleven";
	const auto boundaries = Boundaries(text);
	SetWindowPos(control.window, nullptr, 0, 0, 230, Control::height, SWP_NOACTIVATE | SWP_NOZORDER);
	control.Send(SCI_SETWRAPMODE, SC_WRAP_WORD);
	control.Send(SCI_SETWRAPSTARTINDENT, 2);
	std::vector<int> referenceX, referenceY;
	bool startsWithLatin = false;
	for (int direction : {SC_BIDIRECTIONAL_R2L, SC_BIDIRECTIONAL_L2R}) {
		control.Send(SCI_SETBIDIRECTIONAL, direction);
		control.Send(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(text.c_str()));
		control.Send(SCI_SETSEL, 0, 0);
		control.Paint();
		Check(control.Send(SCI_WRAPCOUNT, 0) > 1, "RTL paragraph wraps");
		std::vector<int> xs, ys;
		for (int boundary : boundaries) {
			xs.push_back(static_cast<int>(control.Send(SCI_POINTXFROMPOSITION, 0, boundary)));
			ys.push_back(static_cast<int>(control.Send(SCI_POINTYFROMPOSITION, 0, boundary)));
		}
		const auto rawX = xs;
		for (size_t index = 0; index < boundaries.size(); ++index) {
			int minimum = Control::width;
			for (size_t other = 0; other < boundaries.size(); ++other) {
				if (ys[other] == ys[index]) minimum = std::min(minimum, rawX[other]);
			}
			xs[index] -= minimum;
		}
		if (referenceX.empty()) {
			referenceX = xs;
			referenceY = ys;
			for (size_t index = 1; index + 1 < boundaries.size(); ++index) {
				if (ys[index] != ys[index - 1] && text[boundaries[index]] >= 'a' && text[boundaries[index]] <= 'z') {
					startsWithLatin = true;
				}
			}
		} else {
			for (size_t index = 0; index < boundaries.size(); ++index) {
				Check(ys[index] == referenceY[index] && std::abs(xs[index] - referenceX[index]) <= 1,
					"wrapped rows retain full paragraph direction independently of alignment");
			}
		}
	}
	Check(startsWithLatin, "wrapped RTL paragraph includes a row beginning with Latin");
	SetWindowPos(control.window, nullptr, 0, 0, Control::width, Control::height, SWP_NOACTIVATE | SWP_NOZORDER);
	control.Send(SCI_SETWRAPMODE, SC_WRAP_NONE);
	std::printf("PASS full paragraph direction across wrapped rows\n");
}
}

int main(int argc, char **) {
	Check(Scintilla_RegisterClasses(GetModuleHandleW(nullptr)) != 0, "register Scintilla");
	{
		Control control;
		FirstStrongDirection();
		AbsoluteVisualOrder(control);
		const std::vector<std::string> samples {
			u8"abc אבג def",
			u8"אבג abc דהו",
			u8"abc אבג 123 (def)",
			u8"אבג\tabc 123",
			u8"abc שָׁלוֹם 😀 def",
			u8"כתוב נכון ABC אני עכשיו בודק אם",
			u8" 123 אבג ABC"
		};
		if (argc == 1) {
			for (int direction : {SC_BIDIRECTIONAL_L2R, SC_BIDIRECTIONAL_R2L}) {
				for (int phases : {SC_PHASES_ONE, SC_PHASES_TWO, SC_PHASES_MULTIPLE}) {
					for (const std::string &sample : samples)
						SelectionDoesNotReorder(control, sample, direction, phases);
				}
			}
		}
		WrappedSelection(control);
		WrappedParagraphDirection(control);
		control.Send(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(""));
		control.Send(SCI_SETBIDIRECTIONAL, SC_BIDIRECTIONAL_R2L);
		control.Send(SCI_SETXOFFSET, 0);
		Check(control.Send(SCI_POINTXFROMPOSITION, 0, 0) == Control::width - 8,
			"empty RTL line caret is at text area's right edge");
		Check((GetWindowLongPtrW(control.window, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) == 0,
			"native bidi does not mirror the Windows control");
		for (int direction : {SC_BIDIRECTIONAL_L2R, SC_BIDIRECTIONAL_R2L}) {
			control.Send(SCI_SETBIDIRECTIONAL, direction);
			control.Send(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(u8"אבג"));
			control.Paint();
			control.Send(SCI_SETEMPTYSELECTION, 0);
			for (int expected : {2, 4, 6}) {
				const LRESULT oldX = control.Send(SCI_POINTXFROMPOSITION, 0, control.Send(SCI_GETCURRENTPOS));
				control.Send(SCI_CHARLEFT);
				Check(control.Send(SCI_GETCURRENTPOS) == expected, "left arrow follows Hebrew clusters visually");
				Check(control.Send(SCI_POINTXFROMPOSITION, 0, expected) < oldX, "left arrow moves left on screen");
			}
			for (int expected : {4, 2, 0}) {
				control.Send(SCI_CHARRIGHT);
				Check(control.Send(SCI_GETCURRENTPOS) == expected, "right arrow follows Hebrew clusters visually");
			}
			control.Send(SCI_CHARRIGHT);
			Check(control.Send(SCI_GETCURRENTPOS) == 0, "right arrow does not bounce at visual row edge");
		}
	}
	Scintilla_ReleaseResources();
	std::printf("PASS: %d native bidi assertions\n", assertions);
	return 0;
}
