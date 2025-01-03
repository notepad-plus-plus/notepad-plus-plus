// Scintilla source code edit control
// ScintillaGTK.cxx - GTK+ specific subclass of ScintillaBase
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cmath>

#include <stdexcept>
#include <new>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <algorithm>
#include <memory>

#include <glib.h>
#include <gmodule.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if defined(GDK_WINDOWING_WAYLAND)
#include <gdk/gdkwayland.h>
#endif

#if defined(_WIN32)
// On Win32 use windows.h to access clipboard (rectangular format) and systems parameters
#undef NOMINMAX
#define NOMINMAX
#include <windows.h>
#endif

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
#include "ILoader.h"
#include "ILexer.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "Scintilla.h"
#include "ScintillaWidget.h"
#include "CharacterCategoryMap.h"
#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "CaseConvert.h"
#include "UniConversion.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"
#include "AutoComplete.h"
#include "ScintillaBase.h"

#include "Wrappers.h"
#include "ScintillaGTK.h"
#include "scintilla-marshal.h"
#include "ScintillaGTKAccessible.h"
#include "Converter.h"

#define IS_WIDGET_REALIZED(w) (gtk_widget_get_realized(GTK_WIDGET(w)))
#define IS_WIDGET_MAPPED(w) (gtk_widget_get_mapped(GTK_WIDGET(w)))

#define SC_INDICATOR_INPUT INDICATOR_IME
#define SC_INDICATOR_TARGET INDICATOR_IME+1
#define SC_INDICATOR_CONVERTED INDICATOR_IME+2
#define SC_INDICATOR_UNKNOWN INDICATOR_IME_MAX

using namespace Scintilla;
using namespace Scintilla::Internal;

// From PlatGTK.cxx
extern std::string UTF8FromLatin1(std::string_view text);
extern void Platform_Initialise();
extern void Platform_Finalise();

namespace {

enum {
	COMMAND_SIGNAL,
	NOTIFY_SIGNAL,
	LAST_SIGNAL
};

gint scintilla_signals[LAST_SIGNAL] = { 0 };

enum {
	TARGET_STRING,
	TARGET_TEXT,
	TARGET_COMPOUND_TEXT,
	TARGET_UTF8_STRING,
	TARGET_URI
};

const GtkTargetEntry clipboardCopyTargets[] = {
	{ (gchar *) "UTF8_STRING", 0, TARGET_UTF8_STRING },
	{ (gchar *) "STRING", 0, TARGET_STRING },
};
constexpr gint nClipboardCopyTargets = static_cast<gint>(std::size(clipboardCopyTargets));

const GtkTargetEntry clipboardPasteTargets[] = {
	{ (gchar *) "text/uri-list", 0, TARGET_URI },
	{ (gchar *) "UTF8_STRING", 0, TARGET_UTF8_STRING },
	{ (gchar *) "STRING", 0, TARGET_STRING },
};
constexpr gint nClipboardPasteTargets = static_cast<gint>(std::size(clipboardPasteTargets));

const GdkDragAction actionCopyOrMove = static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE);

GtkWidget *PWidget(const Window &w) noexcept {
	return static_cast<GtkWidget *>(w.GetID());
}

GdkWindow *PWindow(const Window &w) noexcept {
	GtkWidget *widget = static_cast<GtkWidget *>(w.GetID());
	return gtk_widget_get_window(widget);
}

void MapWidget(GtkWidget *widget) noexcept {
	if (widget &&
		gtk_widget_get_visible(GTK_WIDGET(widget)) &&
		!IS_WIDGET_MAPPED(widget)) {
		gtk_widget_map(widget);
	}
}

const guchar *DataOfGSD(GtkSelectionData *sd) noexcept {
	return gtk_selection_data_get_data(sd);
}

gint LengthOfGSD(GtkSelectionData *sd) noexcept {
	return gtk_selection_data_get_length(sd);
}

GdkAtom TypeOfGSD(GtkSelectionData *sd) noexcept {
	return gtk_selection_data_get_data_type(sd);
}

GdkAtom SelectionOfGSD(GtkSelectionData *sd) noexcept {
	return gtk_selection_data_get_selection(sd);
}

bool SettingGet(GtkSettings *settings, const gchar *name, gpointer value) noexcept {
	if (!settings) {
		return false;
	}
	if (!g_object_class_find_property(G_OBJECT_GET_CLASS(
		G_OBJECT(settings)), name)) {
		return false;
	}
	g_object_get(G_OBJECT(settings), name, value, nullptr);
	return true;
}

}

FontOptions::FontOptions(GtkWidget *widget) noexcept {
	UniquePangoContext pcontext(gtk_widget_create_pango_context(widget));
	PLATFORM_ASSERT(pcontext);
	const cairo_font_options_t *options = pango_cairo_context_get_font_options(pcontext.get());
	// options is owned by the PangoContext so must not be freed.
	if (options) {
		// options is NULL on Win32
		antialias = cairo_font_options_get_antialias(options);
		order = cairo_font_options_get_subpixel_order(options);
		hint = cairo_font_options_get_hint_style(options);
	}
}

bool FontOptions::operator==(const FontOptions &other) const noexcept {
	return antialias == other.antialias &&
		order == other.order &&
		hint == other.hint;
}

ScintillaGTK *ScintillaGTK::FromWidget(GtkWidget *widget) noexcept {
	ScintillaObject *scio = SCINTILLA(widget);
	return static_cast<ScintillaGTK *>(scio->pscin);
}

ScintillaGTK::ScintillaGTK(_ScintillaObject *sci_) :
	adjustmentv(nullptr), adjustmenth(nullptr),
	verticalScrollBarWidth(30), horizontalScrollBarHeight(30),
	buttonMouse(0),
	capturedMouse(false), dragWasDropped(false),
	lastKey(0), rectangularSelectionModifier(SCMOD_CTRL),
	parentClass(nullptr),
	atomSought(nullptr),
	preeditInitialized(false),
	im_context(nullptr),
	lastNonCommonScript(G_UNICODE_SCRIPT_INVALID_CODE),
	settings(nullptr),
	settingsHandlerId(0),
	lastWheelMouseTime(0),
	lastWheelMouseDirection(0),
	wheelMouseIntensity(0),
	smoothScrollY(0),
	smoothScrollX(0),
	rgnUpdate(nullptr),
	repaintFullWindow(false),
	styleIdleID(0),
	accessibilityEnabled(SC_ACCESSIBILITY_ENABLED),
	accessible(nullptr) {
	sci = sci_;
	wMain = GTK_WIDGET(sci);

	rectangularSelectionModifier = SCMOD_ALT;

#if PLAT_GTK_WIN32
	// There does not seem to be a real standard for indicating that the clipboard
	// contains a rectangular selection, so copy Developer Studio.
	cfColumnSelect = static_cast<CLIPFORMAT>(
				 ::RegisterClipboardFormatW(L"MSDEVColumnSelect"));

	// Get intellimouse parameters when running on win32; otherwise use
	// reasonable default
#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES   104
#endif
	::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &linesPerScroll, 0);
#else
	linesPerScroll = 4;
#endif
	primarySelection = false;

	Init();
}

ScintillaGTK::~ScintillaGTK() {
	if (styleIdleID) {
		g_source_remove(styleIdleID);
		styleIdleID = 0;
	}
	if (scrollBarIdleID) {
		g_source_remove(scrollBarIdleID);
		scrollBarIdleID = 0;
	}
	ClearPrimarySelection();
	wPreedit.Destroy();
	if (settingsHandlerId) {
		g_signal_handler_disconnect(settings, settingsHandlerId);
	}
}

void ScintillaGTK::RealizeThis(GtkWidget *widget) {
	//Platform::DebugPrintf("ScintillaGTK::realize this\n");
	gtk_widget_set_realized(widget, TRUE);
	GdkWindowAttr attrs {};
	attrs.window_type = GDK_WINDOW_CHILD;
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	attrs.x = allocation.x;
	attrs.y = allocation.y;
	attrs.width = allocation.width;
	attrs.height = allocation.height;
	attrs.wclass = GDK_INPUT_OUTPUT;
	attrs.visual = gtk_widget_get_visual(widget);
#if !GTK_CHECK_VERSION(3,0,0)
	attrs.colormap = gtk_widget_get_colormap(widget);
#endif
	attrs.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;
	GdkDisplay *pdisplay = gtk_widget_get_display(widget);
	GdkCursor *cursor = gdk_cursor_new_for_display(pdisplay, GDK_XTERM);
	attrs.cursor = cursor;
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_set_window(widget, gdk_window_new(gtk_widget_get_parent_window(widget), &attrs,
			      GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_CURSOR));
#if GTK_CHECK_VERSION(3,8,0)
	gtk_widget_register_window(widget, gtk_widget_get_window(widget));
#else
	gdk_window_set_user_data(gtk_widget_get_window(widget), widget);
#endif
#if !GTK_CHECK_VERSION(3,18,0)
	gtk_style_context_set_background(gtk_widget_get_style_context(widget),
					 gtk_widget_get_window(widget));
#endif
	gdk_window_show(gtk_widget_get_window(widget));
#else
	widget->window = gdk_window_new(gtk_widget_get_parent_window(widget), &attrs,
					GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP | GDK_WA_CURSOR);
	gdk_window_set_user_data(widget->window, widget);
	widget->style = gtk_style_attach(widget->style, widget->window);
	gdk_window_set_background(widget->window, &widget->style->bg[GTK_STATE_NORMAL]);
	gdk_window_show(widget->window);
#endif
	UnRefCursor(cursor);

	preeditInitialized = false;
	gtk_widget_realize(PWidget(wPreedit));
	gtk_widget_realize(PWidget(wPreeditDraw));

	im_context.reset(gtk_im_multicontext_new());
	g_signal_connect(G_OBJECT(im_context.get()), "commit",
			 G_CALLBACK(Commit), this);
	g_signal_connect(G_OBJECT(im_context.get()), "preedit_changed",
			 G_CALLBACK(PreeditChanged), this);
	g_signal_connect(G_OBJECT(im_context.get()), "retrieve-surrounding",
			 G_CALLBACK(RetrieveSurrounding), this);
	g_signal_connect(G_OBJECT(im_context.get()), "delete-surrounding",
			 G_CALLBACK(DeleteSurrounding), this);
	gtk_im_context_set_client_window(im_context.get(), WindowFromWidget(widget));

	GtkWidget *widtxt = PWidget(wText);	//	// No code inside the G_OBJECT macro
	g_signal_connect_after(G_OBJECT(widtxt), "style_set",
			       G_CALLBACK(ScintillaGTK::StyleSetText), nullptr);
	g_signal_connect_after(G_OBJECT(widtxt), "realize",
			       G_CALLBACK(ScintillaGTK::RealizeText), nullptr);
	gtk_widget_realize(widtxt);
	gtk_widget_realize(PWidget(scrollbarv));
	gtk_widget_realize(PWidget(scrollbarh));

	cursor = gdk_cursor_new_for_display(pdisplay, GDK_XTERM);
	gdk_window_set_cursor(PWindow(wText), cursor);
	UnRefCursor(cursor);

	cursor = gdk_cursor_new_for_display(pdisplay, GDK_LEFT_PTR);
	gdk_window_set_cursor(PWindow(scrollbarv), cursor);
	UnRefCursor(cursor);

	cursor = gdk_cursor_new_for_display(pdisplay, GDK_LEFT_PTR);
	gdk_window_set_cursor(PWindow(scrollbarh), cursor);
	UnRefCursor(cursor);

	using NotifyLambda = void (*)(GObject *, GParamSpec *, ScintillaGTK *);
	if (settings) {
		settingsHandlerId = g_signal_connect(settings, "notify::gtk-xft-dpi",
			G_CALLBACK(static_cast<NotifyLambda>([](GObject *, GParamSpec *, ScintillaGTK *sciThis) {
				sciThis->InvalidateStyleRedraw();
			})),
			this);
	}
}

void ScintillaGTK::Realize(GtkWidget *widget) {
	ScintillaGTK *sciThis = FromWidget(widget);
	sciThis->RealizeThis(widget);
}

void ScintillaGTK::UnRealizeThis(GtkWidget *widget) {
	try {
		if (IS_WIDGET_MAPPED(widget)) {
			gtk_widget_unmap(widget);
		}
		gtk_widget_set_realized(widget, FALSE);
		gtk_widget_unrealize(PWidget(wText));
		if (PWidget(scrollbarv))
			gtk_widget_unrealize(PWidget(scrollbarv));
		if (PWidget(scrollbarh))
			gtk_widget_unrealize(PWidget(scrollbarh));
		gtk_widget_unrealize(PWidget(wPreedit));
		gtk_widget_unrealize(PWidget(wPreeditDraw));
		im_context.reset();
		if (GTK_WIDGET_CLASS(parentClass)->unrealize)
			GTK_WIDGET_CLASS(parentClass)->unrealize(widget);

		Finalise();
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::UnRealize(GtkWidget *widget) {
	ScintillaGTK *sciThis = FromWidget(widget);
	sciThis->UnRealizeThis(widget);
}

void ScintillaGTK::MapThis() {
	try {
		//Platform::DebugPrintf("ScintillaGTK::map this\n");
		gtk_widget_set_mapped(PWidget(wMain), TRUE);
		MapWidget(PWidget(wText));
		MapWidget(PWidget(scrollbarh));
		MapWidget(PWidget(scrollbarv));
		wMain.SetCursor(Window::Cursor::arrow);
		scrollbarv.SetCursor(Window::Cursor::arrow);
		scrollbarh.SetCursor(Window::Cursor::arrow);
		SetClientRectangle();
		ChangeSize();
		gdk_window_show(PWindow(wMain));
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::Map(GtkWidget *widget) {
	ScintillaGTK *sciThis = FromWidget(widget);
	sciThis->MapThis();
}

void ScintillaGTK::UnMapThis() {
	try {
		//Platform::DebugPrintf("ScintillaGTK::unmap this\n");
		gtk_widget_set_mapped(PWidget(wMain), FALSE);
		DropGraphics();
		gdk_window_hide(PWindow(wMain));
		gtk_widget_unmap(PWidget(wText));
		if (PWidget(scrollbarh))
			gtk_widget_unmap(PWidget(scrollbarh));
		if (PWidget(scrollbarv))
			gtk_widget_unmap(PWidget(scrollbarv));
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::UnMap(GtkWidget *widget) {
	ScintillaGTK *sciThis = FromWidget(widget);
	sciThis->UnMapThis();
}

void ScintillaGTK::ForAll(GtkCallback callback, gpointer callback_data) {
	try {
		(*callback)(PWidget(wText), callback_data);
		if (PWidget(scrollbarv))
			(*callback)(PWidget(scrollbarv), callback_data);
		if (PWidget(scrollbarh))
			(*callback)(PWidget(scrollbarh), callback_data);
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::MainForAll(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data) {
	ScintillaGTK *sciThis = FromWidget(GTK_WIDGET(container));

	if (callback && include_internals) {
		sciThis->ForAll(callback, callback_data);
	}
}

namespace {

class PreEditString {
public:
	gchar *str;
	gint cursor_pos;
	PangoAttrList *attrs;
	gboolean validUTF8;
	glong uniStrLen;
	gunichar *uniStr;
	GUnicodeScript pscript;

	explicit PreEditString(GtkIMContext *im_context) noexcept {
		gtk_im_context_get_preedit_string(im_context, &str, &attrs, &cursor_pos);
		validUTF8 = g_utf8_validate(str, strlen(str), nullptr);
		uniStr = g_utf8_to_ucs4_fast(str, static_cast<glong>(strlen(str)), &uniStrLen);
		pscript = g_unichar_get_script(uniStr[0]);
	}
	// Deleted so PreEditString objects can not be copied.
	PreEditString(const PreEditString&) = delete;
	PreEditString(PreEditString&&) = delete;
	PreEditString&operator=(const PreEditString&) = delete;
	PreEditString&operator=(PreEditString&&) = delete;
	~PreEditString() {
		g_free(str);
		g_free(uniStr);
		pango_attr_list_unref(attrs);
	}
};

}

gint ScintillaGTK::FocusInThis(GtkWidget *) {
	try {
		SetFocusState(true);

		if (im_context) {
			gtk_im_context_focus_in(im_context.get());
			PreEditString pes(im_context.get());
			if (PWidget(wPreedit)) {
				if (!preeditInitialized) {
					GtkWidget *top = gtk_widget_get_toplevel(PWidget(wMain));
					gtk_window_set_transient_for(GTK_WINDOW(PWidget(wPreedit)), GTK_WINDOW(top));
					preeditInitialized = true;
				}

				if (strlen(pes.str) > 0) {
					gtk_widget_show(PWidget(wPreedit));
				} else {
					gtk_widget_hide(PWidget(wPreedit));
				}
			}
		}
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return FALSE;
}

gint ScintillaGTK::FocusIn(GtkWidget *widget, GdkEventFocus * /*event*/) {
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->FocusInThis(widget);
}

gint ScintillaGTK::FocusOutThis(GtkWidget *) {
	try {
		SetFocusState(false);

		if (PWidget(wPreedit))
			gtk_widget_hide(PWidget(wPreedit));
		if (im_context)
			gtk_im_context_focus_out(im_context.get());

	} catch (...) {
		errorStatus = Status::Failure;
	}
	return FALSE;
}

gint ScintillaGTK::FocusOut(GtkWidget *widget, GdkEventFocus * /*event*/) {
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->FocusOutThis(widget);
}

void ScintillaGTK::SizeRequest(GtkWidget *widget, GtkRequisition *requisition) {
	const ScintillaGTK *sciThis = FromWidget(widget);
	requisition->width = 1;
	requisition->height = 1;
	GtkRequisition child_requisition;
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_get_preferred_size(PWidget(sciThis->scrollbarh), nullptr, &child_requisition);
	gtk_widget_get_preferred_size(PWidget(sciThis->scrollbarv), nullptr, &child_requisition);
#else
	gtk_widget_size_request(PWidget(sciThis->scrollbarh), &child_requisition);
	gtk_widget_size_request(PWidget(sciThis->scrollbarv), &child_requisition);
#endif
}

#if GTK_CHECK_VERSION(3,0,0)

void ScintillaGTK::GetPreferredWidth(GtkWidget *widget, gint *minimalWidth, gint *naturalWidth) {
	GtkRequisition requisition;
	SizeRequest(widget, &requisition);
	*minimalWidth = *naturalWidth = requisition.width;
}

void ScintillaGTK::GetPreferredHeight(GtkWidget *widget, gint *minimalHeight, gint *naturalHeight) {
	GtkRequisition requisition;
	SizeRequest(widget, &requisition);
	*minimalHeight = *naturalHeight = requisition.height;
}

#endif

void ScintillaGTK::SizeAllocate(GtkWidget *widget, GtkAllocation *allocation) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		gtk_widget_set_allocation(widget, allocation);
		if (IS_WIDGET_REALIZED(widget))
			gdk_window_move_resize(WindowFromWidget(widget),
					       allocation->x,
					       allocation->y,
					       allocation->width,
					       allocation->height);

		sciThis->Resize(allocation->width, allocation->height);

	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
}

void ScintillaGTK::Init() {
	parentClass = static_cast<GtkWidgetClass *>(
			      g_type_class_ref(gtk_container_get_type()));

	gint maskSmooth = 0;
#if defined(GDK_WINDOWING_WAYLAND)
	GdkDisplay *pdisplay = gdk_display_get_default();
	if (GDK_IS_WAYLAND_DISPLAY(pdisplay)) {
		// On Wayland, touch pads only produce smooth scroll events
		maskSmooth = GDK_SMOOTH_SCROLL_MASK;
	}
#endif

	gtk_widget_set_can_focus(PWidget(wMain), TRUE);
	gtk_widget_set_sensitive(PWidget(wMain), TRUE);
	gtk_widget_set_events(PWidget(wMain),
			      GDK_EXPOSURE_MASK
			      | GDK_SCROLL_MASK
			      | maskSmooth
			      | GDK_STRUCTURE_MASK
			      | GDK_KEY_PRESS_MASK
			      | GDK_KEY_RELEASE_MASK
			      | GDK_FOCUS_CHANGE_MASK
			      | GDK_LEAVE_NOTIFY_MASK
			      | GDK_BUTTON_PRESS_MASK
			      | GDK_BUTTON_RELEASE_MASK
			      | GDK_POINTER_MOTION_MASK
			      | GDK_POINTER_MOTION_HINT_MASK);

	wText = gtk_drawing_area_new();
	gtk_widget_set_parent(PWidget(wText), PWidget(wMain));
	GtkWidget *widtxt = PWidget(wText);	// No code inside the G_OBJECT macro
	gtk_widget_show(widtxt);
#if GTK_CHECK_VERSION(3,0,0)
	g_signal_connect(G_OBJECT(widtxt), "draw",
			 G_CALLBACK(ScintillaGTK::DrawText), this);
#else
	g_signal_connect(G_OBJECT(widtxt), "expose_event",
			 G_CALLBACK(ScintillaGTK::ExposeText), this);
#endif
#if GTK_CHECK_VERSION(3,0,0)
	// we need a runtime check because we don't want double buffering when
	// running on >= 3.9.2
	if (gtk_check_version(3, 9, 2) != nullptr /* on < 3.9.2 */)
#endif
	{
#if !GTK_CHECK_VERSION(3,14,0)
		// Avoid background drawing flash/missing redraws
		gtk_widget_set_double_buffered(widtxt, FALSE);
#endif
	}
	gtk_widget_set_events(widtxt, GDK_EXPOSURE_MASK);
	gtk_widget_set_size_request(widtxt, 100, 100);
	adjustmentv = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 201.0, 1.0, 20.0, 20.0));
#if GTK_CHECK_VERSION(3,0,0)
	scrollbarv = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(adjustmentv));
#else
	scrollbarv = gtk_vscrollbar_new(GTK_ADJUSTMENT(adjustmentv));
#endif
	gtk_widget_set_can_focus(PWidget(scrollbarv), FALSE);
	g_signal_connect(G_OBJECT(adjustmentv), "value_changed",
			 G_CALLBACK(ScrollSignal), this);
	gtk_widget_set_parent(PWidget(scrollbarv), PWidget(wMain));
	gtk_widget_show(PWidget(scrollbarv));

	adjustmenth = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 101.0, 1.0, 20.0, 20.0));
#if GTK_CHECK_VERSION(3,0,0)
	scrollbarh = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT(adjustmenth));
#else
	scrollbarh = gtk_hscrollbar_new(GTK_ADJUSTMENT(adjustmenth));
#endif
	gtk_widget_set_can_focus(PWidget(scrollbarh), FALSE);
	g_signal_connect(G_OBJECT(adjustmenth), "value_changed",
			 G_CALLBACK(ScrollHSignal), this);
	gtk_widget_set_parent(PWidget(scrollbarh), PWidget(wMain));
	gtk_widget_show(PWidget(scrollbarh));

	gtk_widget_grab_focus(PWidget(wMain));

	gtk_drag_dest_set(GTK_WIDGET(PWidget(wMain)),
			  GTK_DEST_DEFAULT_ALL, clipboardPasteTargets, nClipboardPasteTargets,
			  actionCopyOrMove);

	/* create pre-edit window */
	wPreedit = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_type_hint(GTK_WINDOW(PWidget(wPreedit)), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
	wPreeditDraw = gtk_drawing_area_new();
	GtkWidget *predrw = PWidget(wPreeditDraw);      // No code inside the G_OBJECT macro
#if GTK_CHECK_VERSION(3,0,0)
	g_signal_connect(G_OBJECT(predrw), "draw",
			 G_CALLBACK(DrawPreedit), this);
#else
	g_signal_connect(G_OBJECT(predrw), "expose_event",
			 G_CALLBACK(ExposePreedit), this);
#endif
	gtk_container_add(GTK_CONTAINER(PWidget(wPreedit)), predrw);
	gtk_widget_show(predrw);

	settings = gtk_settings_get_default();

	// Set caret period based on GTK settings
	gboolean blinkOn = false;
	SettingGet(settings, "gtk-cursor-blink", &blinkOn);
	if (blinkOn) {
		gint value = 500;
		if (SettingGet(settings, "gtk-cursor-blink-time", &value)) {
			caret.period = static_cast<int>(value / 1.75);
		}
	} else {
		caret.period = 0;
	}

	for (size_t tr = static_cast<size_t>(TickReason::caret); tr <= static_cast<size_t>(TickReason::dwell); tr++) {
		timers[tr].reason = static_cast<TickReason>(tr);
		timers[tr].scintilla = this;
	}

	vs.indicators[SC_INDICATOR_UNKNOWN] = Indicator(IndicatorStyle::Hidden, colourIME);
	vs.indicators[SC_INDICATOR_INPUT] = Indicator(IndicatorStyle::Dots, colourIME);
	vs.indicators[SC_INDICATOR_CONVERTED] = Indicator(IndicatorStyle::CompositionThick, colourIME);
	vs.indicators[SC_INDICATOR_TARGET] = Indicator(IndicatorStyle::StraightBox, colourIME);

	fontOptionsPrevious = FontOptions(PWidget(wText));
}

void ScintillaGTK::Finalise() {
	for (size_t tr = static_cast<size_t>(TickReason::caret); tr <= static_cast<size_t>(TickReason::dwell); tr++) {
		FineTickerCancel(static_cast<TickReason>(tr));
	}
	if (accessible) {
		gtk_accessible_set_widget(GTK_ACCESSIBLE(accessible), nullptr);
		g_object_unref(accessible);
		accessible = nullptr;
	}

	ScintillaBase::Finalise();
}

bool ScintillaGTK::AbandonPaint() {
	if ((paintState == PaintState::painting) && !paintingAllText) {
		repaintFullWindow = true;
	}
	return false;
}

void ScintillaGTK::DisplayCursor(Window::Cursor c) {
	if (cursorMode == CursorShape::Normal)
		wText.SetCursor(c);
	else
		wText.SetCursor(static_cast<Window::Cursor>(cursorMode));
}

bool ScintillaGTK::DragThreshold(Point ptStart, Point ptNow) {
	return gtk_drag_check_threshold(GTK_WIDGET(PWidget(wMain)),
		static_cast<gint>(ptStart.x), static_cast<gint>(ptStart.y),
		static_cast<gint>(ptNow.x), static_cast<gint>(ptNow.y));
}

void ScintillaGTK::StartDrag() {
	PLATFORM_ASSERT(evbtn);
	dragWasDropped = false;
	inDragDrop = DragDrop::dragging;
	GtkTargetList *tl = gtk_target_list_new(clipboardCopyTargets, nClipboardCopyTargets);
#if GTK_CHECK_VERSION(3,10,0)
	gtk_drag_begin_with_coordinates(GTK_WIDGET(PWidget(wMain)),
					tl,
					actionCopyOrMove,
					buttonMouse,
					evbtn.get(),
					-1, -1);
#else
	gtk_drag_begin(GTK_WIDGET(PWidget(wMain)),
		       tl,
		       actionCopyOrMove,
		       buttonMouse,
		       evbtn.get());
#endif
}

namespace Scintilla::Internal {

std::string ConvertText(const char *s, size_t len, const char *charSetDest,
			const char *charSetSource, bool transliterations, bool silent) {
	// s is not const because of different versions of iconv disagreeing about const
	std::string destForm;
	Converter conv(charSetDest, charSetSource, transliterations);
	if (conv) {
		gsize outLeft = len*3+1;
		destForm = std::string(outLeft, '\0');
		// g_iconv does not actually write to its input argument so safe to cast away const
		char *pin = const_cast<char *>(s);
		gsize inLeft = len;
		char *putf = &destForm[0];
		char *pout = putf;
		const gsize conversions = conv.Convert(&pin, &inLeft, &pout, &outLeft);
		if (conversions == sizeFailure) {
			if (!silent) {
				if (len == 1)
					fprintf(stderr, "iconv %s->%s failed for %0x '%s'\n",
						charSetSource, charSetDest, static_cast<unsigned char>(*s), s);
				else
					fprintf(stderr, "iconv %s->%s failed for %s\n",
						charSetSource, charSetDest, s);
			}
			destForm = std::string();
		} else {
			destForm.resize(pout - putf);
		}
	} else {
		fprintf(stderr, "Can not iconv %s %s\n", charSetDest, charSetSource);
	}
	return destForm;
}
}

// Returns the target converted to UTF8.
// Return the length in bytes.
Sci::Position ScintillaGTK::TargetAsUTF8(char *text) const {
	const Sci::Position targetLength = targetRange.Length();
	if (IsUnicodeMode()) {
		if (text) {
			pdoc->GetCharRange(text, targetRange.start.Position(), targetLength);
		}
	} else {
		// Need to convert
		const char *charSetBuffer = CharacterSetID();
		if (*charSetBuffer) {
			std::string s = RangeText(targetRange.start.Position(), targetRange.end.Position());
			std::string tmputf = ConvertText(&s[0], targetLength, "UTF-8", charSetBuffer, false);
			if (text) {
				memcpy(text, tmputf.c_str(), tmputf.length());
			}
			return tmputf.length();
		} else {
			if (text) {
				pdoc->GetCharRange(text, targetRange.start.Position(), targetLength);
			}
		}
	}
	return targetLength;
}

// Translates a nul terminated UTF8 string into the document encoding.
// Return the length of the result in bytes.
Sci::Position ScintillaGTK::EncodedFromUTF8(const char *utf8, char *encoded) const {
	const Sci::Position inputLength = (lengthForEncode >= 0) ? lengthForEncode : strlen(utf8);
	if (IsUnicodeMode()) {
		if (encoded) {
			memcpy(encoded, utf8, inputLength);
		}
		return inputLength;
	} else {
		// Need to convert
		const char *charSetBuffer = CharacterSetID();
		if (*charSetBuffer) {
			std::string tmpEncoded = ConvertText(utf8, inputLength, charSetBuffer, "UTF-8", true);
			if (encoded) {
				memcpy(encoded, tmpEncoded.c_str(), tmpEncoded.length());
			}
			return tmpEncoded.length();
		} else {
			if (encoded) {
				memcpy(encoded, utf8, inputLength);
			}
			return inputLength;
		}
	}
	// Fail
	return 0;
}

bool ScintillaGTK::ValidCodePage(int codePage) const {
	return codePage == 0
	       || codePage == SC_CP_UTF8
	       || codePage == 932
	       || codePage == 936
	       || codePage == 949
	       || codePage == 950
	       || codePage == 1361;
}

std::string ScintillaGTK::UTF8FromEncoded(std::string_view encoded) const {
	if (IsUnicodeMode()) {
		return std::string(encoded);
	} else {
		const char *charSetBuffer = CharacterSetID();
		return ConvertText(encoded.data(), encoded.length(), "UTF-8", charSetBuffer, true);
	}
}

std::string ScintillaGTK::EncodedFromUTF8(std::string_view utf8) const {
	if (IsUnicodeMode()) {
		return std::string(utf8);
	} else {
		const char *charSetBuffer = CharacterSetID();
		return ConvertText(utf8.data(), utf8.length(), charSetBuffer, "UTF-8", true);
	}
}

sptr_t ScintillaGTK::WndProc(Message iMessage, uptr_t wParam, sptr_t lParam) {
	try {
		switch (iMessage) {

		case Message::GrabFocus:
			gtk_widget_grab_focus(PWidget(wMain));
			break;

		case Message::GetDirectFunction:
			return reinterpret_cast<sptr_t>(DirectFunction);

		case Message::GetDirectStatusFunction:
			return reinterpret_cast<sptr_t>(DirectStatusFunction);

		case Message::GetDirectPointer:
			return reinterpret_cast<sptr_t>(this);

		case Message::TargetAsUTF8:
			return TargetAsUTF8(CharPtrFromSPtr(lParam));

		case Message::EncodedFromUTF8:
			return EncodedFromUTF8(ConstCharPtrFromUPtr(wParam),
					       CharPtrFromSPtr(lParam));

		case Message::SetRectangularSelectionModifier:
			rectangularSelectionModifier = static_cast<int>(wParam);
			break;

		case Message::GetRectangularSelectionModifier:
			return rectangularSelectionModifier;

		case Message::SetReadOnly: {
				const sptr_t ret = ScintillaBase::WndProc(iMessage, wParam, lParam);
				if (accessible) {
					ScintillaGTKAccessible *sciAccessible = ScintillaGTKAccessible::FromAccessible(accessible);
					if (sciAccessible) {
						sciAccessible->NotifyReadOnly();
					}
				}
				return ret;
			}

		case Message::GetAccessibility:
			return accessibilityEnabled;

		case Message::SetAccessibility:
			accessibilityEnabled = static_cast<int>(wParam);
			if (accessible) {
				ScintillaGTKAccessible *sciAccessible = ScintillaGTKAccessible::FromAccessible(accessible);
				if (sciAccessible) {
					sciAccessible->SetAccessibility(accessibilityEnabled);
				}
			}
			break;

		default:
			return ScintillaBase::WndProc(iMessage, wParam, lParam);
		}
	} catch (std::bad_alloc &) {
		errorStatus = Status::BadAlloc;
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return 0;
}

sptr_t ScintillaGTK::DefWndProc(Message, uptr_t, sptr_t) {
	return 0;
}

bool ScintillaGTK::FineTickerRunning(TickReason reason) {
	return timers[static_cast<size_t>(reason)].timer != 0;
}

void ScintillaGTK::FineTickerStart(TickReason reason, int millis, int /* tolerance */) {
	FineTickerCancel(reason);
	const size_t reasonIndex = static_cast<size_t>(reason);
	timers[reasonIndex].timer = gdk_threads_add_timeout(millis, TimeOut, &timers[reasonIndex]);
}

void ScintillaGTK::FineTickerCancel(TickReason reason) {
	const size_t reasonIndex = static_cast<size_t>(reason);
	if (timers[reasonIndex].timer) {
		g_source_remove(timers[reasonIndex].timer);
		timers[reasonIndex].timer = 0;
	}
}

bool ScintillaGTK::SetIdle(bool on) {
	if (on) {
		// Start idler, if it's not running.
		if (!idler.state) {
			idler.state = true;
			idler.idlerID = GUINT_TO_POINTER(
						gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE, IdleCallback, this, nullptr));
		}
	} else {
		// Stop idler, if it's running
		if (idler.state) {
			idler.state = false;
			g_source_remove(GPOINTER_TO_UINT(idler.idlerID));
		}
	}
	return true;
}

void ScintillaGTK::SetMouseCapture(bool on) {
	if (mouseDownCaptures) {
		if (on) {
			gtk_grab_add(GTK_WIDGET(PWidget(wMain)));
		} else {
			gtk_grab_remove(GTK_WIDGET(PWidget(wMain)));
		}
	}
	capturedMouse = on;
}

bool ScintillaGTK::HaveMouseCapture() {
	return capturedMouse;
}

#if GTK_CHECK_VERSION(3,0,0)

namespace {

// Is crcTest completely in crcContainer?
bool CRectContains(const cairo_rectangle_t &crcContainer, const cairo_rectangle_t &crcTest) {
	return
		(crcTest.x >= crcContainer.x) && ((crcTest.x + crcTest.width) <= (crcContainer.x + crcContainer.width)) &&
		(crcTest.y >= crcContainer.y) && ((crcTest.y + crcTest.height) <= (crcContainer.y + crcContainer.height));
}

// Is crcTest completely in crcListContainer?
// May incorrectly return false if complex shape
bool CRectListContains(const cairo_rectangle_list_t *crcListContainer, const cairo_rectangle_t &crcTest) {
	for (int r=0; r<crcListContainer->num_rectangles; r++) {
		if (CRectContains(crcListContainer->rectangles[r], crcTest))
			return true;
	}
	return false;
}

}

#endif

bool ScintillaGTK::PaintContains(PRectangle rc) {
	// This allows optimization when a rectangle is completely in the update region.
	// It is OK to return false when too difficult to determine as that just performs extra drawing
	bool contains = true;
	if (paintState == PaintState::painting) {
		if (!rcPaint.Contains(rc)) {
			contains = false;
		} else if (rgnUpdate) {
#if GTK_CHECK_VERSION(3,0,0)
			cairo_rectangle_t grc = {rc.left, rc.top,
						 rc.right - rc.left, rc.bottom - rc.top
						};
			contains = CRectListContains(rgnUpdate, grc);
#else
			GdkRectangle grc = {static_cast<gint>(rc.left), static_cast<gint>(rc.top),
					    static_cast<gint>(rc.right - rc.left), static_cast<gint>(rc.bottom - rc.top)
					   };
			if (gdk_region_rect_in(rgnUpdate, &grc) != GDK_OVERLAP_RECTANGLE_IN) {
				contains = false;
			}
#endif
		}
	}
	return contains;
}

// Redraw all of text area. This paint will not be abandoned.
void ScintillaGTK::FullPaint() {
	wText.InvalidateAll();
}

void ScintillaGTK::SetClientRectangle() {
	rectangleClient = wMain.GetClientPosition();
}

PRectangle ScintillaGTK::GetClientRectangle() const {
	PRectangle rc = rectangleClient;
	if (verticalScrollBarVisible)
		rc.right -= verticalScrollBarWidth;
	if (horizontalScrollBarVisible && !Wrapping())
		rc.bottom -= horizontalScrollBarHeight;
	// Move to origin
	rc.right -= rc.left;
	rc.bottom -= rc.top;
	if (rc.bottom < 0)
		rc.bottom = 0;
	if (rc.right < 0)
		rc.right = 0;
	rc.left = 0;
	rc.top = 0;
	return rc;
}

void ScintillaGTK::ScrollText(Sci::Line linesToMove) {
	NotifyUpdateUI();

#if GTK_CHECK_VERSION(3,22,0)
	Redraw();
#else
	GtkWidget *wi = PWidget(wText);
	if (IS_WIDGET_REALIZED(wi)) {
		const Sci::Line diff = vs.lineHeight * -linesToMove;
		gdk_window_scroll(WindowFromWidget(wi), 0, static_cast<gint>(-diff));
		gdk_window_process_updates(WindowFromWidget(wi), FALSE);
	}
#endif
}

void ScintillaGTK::SetVerticalScrollPos() {
	DwellEnd(true);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustmentv), static_cast<gdouble>(topLine));
}

void ScintillaGTK::SetHorizontalScrollPos() {
	DwellEnd(true);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustmenth), xOffset);
}

bool ScintillaGTK::ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) {
	bool modified = false;
	const int pageScroll = static_cast<int>(LinesToScroll());

	if (gtk_adjustment_get_upper(adjustmentv) != (nMax + 1) ||
			gtk_adjustment_get_page_size(adjustmentv) != nPage ||
			gtk_adjustment_get_page_increment(adjustmentv) != pageScroll) {
		gtk_adjustment_set_upper(adjustmentv, nMax + 1.0);
		gtk_adjustment_set_page_size(adjustmentv, static_cast<gdouble>(nPage));
		gtk_adjustment_set_page_increment(adjustmentv, pageScroll);
#if !GTK_CHECK_VERSION(3,18,0)
		gtk_adjustment_changed(GTK_ADJUSTMENT(adjustmentv));
#endif
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustmentv), static_cast<gdouble>(topLine));
		modified = true;
	}

	const PRectangle rcText = GetTextRectangle();
	int horizEndPreferred = scrollWidth;
	if (horizEndPreferred < 0)
		horizEndPreferred = 0;
	const unsigned int pageWidth = static_cast<unsigned int>(rcText.Width());
	const unsigned int pageIncrement = pageWidth / 3;
	const unsigned int charWidth = static_cast<unsigned int>(vs.styles[STYLE_DEFAULT].aveCharWidth);
	if (gtk_adjustment_get_upper(adjustmenth) != horizEndPreferred ||
			gtk_adjustment_get_page_size(adjustmenth) != pageWidth ||
			gtk_adjustment_get_page_increment(adjustmenth) != pageIncrement ||
			gtk_adjustment_get_step_increment(adjustmenth) != charWidth) {
		gtk_adjustment_set_upper(adjustmenth, horizEndPreferred);
		gtk_adjustment_set_page_size(adjustmenth, pageWidth);
		gtk_adjustment_set_page_increment(adjustmenth, pageIncrement);
		gtk_adjustment_set_step_increment(adjustmenth, charWidth);
#if !GTK_CHECK_VERSION(3,18,0)
		gtk_adjustment_changed(GTK_ADJUSTMENT(adjustmenth));
#endif
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustmenth), xOffset);
		modified = true;
	}
	if (modified && (paintState == PaintState::painting)) {
		repaintFullWindow = true;
	}

	return modified;
}

void ScintillaGTK::ReconfigureScrollBars() {
	const PRectangle rc = wMain.GetClientPosition();
	Resize(static_cast<int>(rc.Width()), static_cast<int>(rc.Height()));
}

void ScintillaGTK::SetScrollBars() {
	if (scrollBarIdleID) {
		// Only allow one scroll bar change to be queued
		return;
	}
	constexpr gint priorityScrollBar = GDK_PRIORITY_REDRAW + 5;
	// On GTK, unlike other platforms, modifying scrollbars inside some events including
	// resizes causes problems. Deferring the modification to a lower priority (125) idle
	// event avoids the problems. This code did not always work when the priority was
	// higher than GTK's resize (GTK_PRIORITY_RESIZE=110) or redraw
	// (GDK_PRIORITY_REDRAW=120) idle tasks.
	scrollBarIdleID = gdk_threads_add_idle_full(priorityScrollBar,
		[](gpointer pSci) -> gboolean {
			ScintillaGTK *sciThis = static_cast<ScintillaGTK *>(pSci);
			sciThis->ChangeScrollBars();
			sciThis->scrollBarIdleID = 0;
			return FALSE;
		},
		this, nullptr);
}

void ScintillaGTK::NotifyChange() {
	g_signal_emit(G_OBJECT(sci), scintilla_signals[COMMAND_SIGNAL], 0,
		      Platform::LongFromTwoShorts(GetCtrlID(), SCEN_CHANGE), PWidget(wMain));
}

void ScintillaGTK::NotifyFocus(bool focus) {
	if (commandEvents)
		g_signal_emit(G_OBJECT(sci), scintilla_signals[COMMAND_SIGNAL], 0,
			      Platform::LongFromTwoShorts
			      (GetCtrlID(), focus ? SCEN_SETFOCUS : SCEN_KILLFOCUS), PWidget(wMain));
	Editor::NotifyFocus(focus);
}

void ScintillaGTK::NotifyParent(NotificationData scn) {
	scn.nmhdr.hwndFrom = PWidget(wMain);
	scn.nmhdr.idFrom = GetCtrlID();
	g_signal_emit(G_OBJECT(sci), scintilla_signals[NOTIFY_SIGNAL], 0,
		      GetCtrlID(), &scn);
}

void ScintillaGTK::NotifyKey(Keys key, KeyMod modifiers) {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::Key;
	scn.ch = static_cast<int>(key);
	scn.modifiers = modifiers;

	NotifyParent(scn);
}

void ScintillaGTK::NotifyURIDropped(const char *list) {
	NotificationData scn = {};
	scn.nmhdr.code = Notification::URIDropped;
	scn.text = list;

	NotifyParent(scn);
}

const char *CharacterSetID(CharacterSet characterSet);

const char *ScintillaGTK::CharacterSetID() const {
	return ::CharacterSetID(vs.styles[STYLE_DEFAULT].characterSet);
}

namespace {

class CaseFolderDBCS : public CaseFolderTable {
	const char *charSet;
public:
	explicit CaseFolderDBCS(const char *charSet_) noexcept : charSet(charSet_) {
	}
	size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) override {
		if ((lenMixed == 1) && (sizeFolded > 0)) {
			folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
			return 1;
		} else if (*charSet) {
			std::string sUTF8 = ConvertText(mixed, lenMixed,
							"UTF-8", charSet, false);
			if (!sUTF8.empty()) {
				UniqueStr mapped(g_utf8_casefold(sUTF8.c_str(), sUTF8.length()));
				size_t lenMapped = strlen(mapped.get());
				if (lenMapped < sizeFolded) {
					memcpy(folded, mapped.get(),  lenMapped);
				} else {
					folded[0] = '\0';
					lenMapped = 1;
				}
				return lenMapped;
			}
		}
		// Something failed so return a single NUL byte
		folded[0] = '\0';
		return 1;
	}
};

}

std::unique_ptr<CaseFolder> ScintillaGTK::CaseFolderForEncoding() {
	if (pdoc->dbcsCodePage == SC_CP_UTF8) {
		return std::make_unique<CaseFolderUnicode>();
	} else {
		const char *charSetBuffer = CharacterSetID();
		if (charSetBuffer) {
			if (pdoc->dbcsCodePage == 0) {
				std::unique_ptr<CaseFolderTable> pcf = std::make_unique<CaseFolderTable>();
				// Only for single byte encodings
				for (int i=0x80; i<0x100; i++) {
					char sCharacter[2] = "A";
					sCharacter[0] = i;
					// Silent as some bytes have no assigned character
					std::string sUTF8 = ConvertText(sCharacter, 1,
									"UTF-8", charSetBuffer, false, true);
					if (!sUTF8.empty()) {
						UniqueStr mapped(g_utf8_casefold(sUTF8.c_str(), sUTF8.length()));
						if (mapped) {
							std::string mappedBack = ConvertText(mapped.get(), strlen(mapped.get()),
											     charSetBuffer, "UTF-8", false, true);
							if ((mappedBack.length() == 1) && (mappedBack[0] != sCharacter[0])) {
								pcf->SetTranslation(sCharacter[0], mappedBack[0]);
							}
						}
					}
				}
				return pcf;
			} else {
				return std::make_unique<CaseFolderDBCS>(charSetBuffer);
			}
		}
		return nullptr;
	}
}

namespace {

struct CaseMapper {
	UniqueStr mapped;
	CaseMapper(const std::string &sUTF8, bool toUpperCase) noexcept {
		if (toUpperCase) {
			mapped.reset(g_utf8_strup(sUTF8.c_str(), sUTF8.length()));
		} else {
			mapped.reset(g_utf8_strdown(sUTF8.c_str(), sUTF8.length()));
		}
	}
};

}

std::string ScintillaGTK::CaseMapString(const std::string &s, CaseMapping caseMapping) {
	if (s.empty() || (caseMapping == CaseMapping::same))
		return s;

	if (IsUnicodeMode()) {
		std::string retMapped(s.length() * maxExpansionCaseConversion, 0);
		const size_t lenMapped = CaseConvertString(&retMapped[0], retMapped.length(), s.c_str(), s.length(),
					 (caseMapping == CaseMapping::upper) ? CaseConversion::upper : CaseConversion::lower);
		retMapped.resize(lenMapped);
		return retMapped;
	}

	const char *charSetBuffer = CharacterSetID();

	if (!*charSetBuffer) {
		CaseMapper mapper(s, caseMapping == CaseMapping::upper);
		return std::string(mapper.mapped.get());
	} else {
		// Change text to UTF-8
		std::string sUTF8 = ConvertText(s.c_str(), s.length(),
						"UTF-8", charSetBuffer, false);
		CaseMapper mapper(sUTF8, caseMapping == CaseMapping::upper);
		return ConvertText(mapper.mapped.get(), strlen(mapper.mapped.get()), charSetBuffer, "UTF-8", false);
	}
}

int ScintillaGTK::KeyDefault(Keys key, KeyMod modifiers) {
	// Pass up to container in case it is an accelerator
	NotifyKey(key, modifiers);
	return 0;
}

void ScintillaGTK::CopyToClipboard(const SelectionText &selectedText) {
	SelectionText *clipText = new SelectionText();
	clipText->Copy(selectedText);
	StoreOnClipboard(clipText);
}

void ScintillaGTK::Copy() {
	if (!sel.Empty()) {
		SelectionText *clipText = new SelectionText();
		CopySelectionRange(clipText);
		StoreOnClipboard(clipText);
#if PLAT_GTK_WIN32
		if (sel.IsRectangular()) {
			::OpenClipboard(NULL);
			::SetClipboardData(cfColumnSelect, 0);
			::CloseClipboard();
		}
#endif
	}
}

namespace {

// Helper class for the asynchronous paste not to risk calling in a destroyed ScintillaGTK

class SelectionReceiver : GObjectWatcher {
	ScintillaGTK *sci;

	void Destroyed() noexcept override {
		sci = nullptr;
	}

public:
	SelectionReceiver(ScintillaGTK *sci_) :
		GObjectWatcher(G_OBJECT(sci_->MainObject())),
		sci(sci_) {
	}

	static void ClipboardReceived(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer data) noexcept {
		SelectionReceiver *self = static_cast<SelectionReceiver *>(data);
		if (self->sci) {
			self->sci->ReceivedClipboard(clipboard, selection_data);
		}
		delete self;
	}
};

}

void ScintillaGTK::RequestSelection(GdkAtom atomSelection) {
	atomSought = atomUTF8;
	GtkClipboard *clipBoard =
		gtk_widget_get_clipboard(GTK_WIDGET(PWidget(wMain)), atomSelection);
	if (clipBoard) {
		gtk_clipboard_request_contents(clipBoard, atomSought,
					       SelectionReceiver::ClipboardReceived,
					       new SelectionReceiver(this));
	}
}

void ScintillaGTK::Paste() {
	RequestSelection(GDK_SELECTION_CLIPBOARD);
}

void ScintillaGTK::CreateCallTipWindow(PRectangle rc) {
	if (!ct.wCallTip.Created()) {
		ct.wCallTip = gtk_window_new(GTK_WINDOW_POPUP);
		gtk_window_set_type_hint(GTK_WINDOW(PWidget(ct.wCallTip)), GDK_WINDOW_TYPE_HINT_TOOLTIP);
		ct.wDraw = gtk_drawing_area_new();
		GtkWidget *widcdrw = PWidget(ct.wDraw);	//	// No code inside the G_OBJECT macro
		gtk_container_add(GTK_CONTAINER(PWidget(ct.wCallTip)), widcdrw);
#if GTK_CHECK_VERSION(3,0,0)
		g_signal_connect(G_OBJECT(widcdrw), "draw",
				 G_CALLBACK(ScintillaGTK::DrawCT), &ct);
#else
		g_signal_connect(G_OBJECT(widcdrw), "expose_event",
				 G_CALLBACK(ScintillaGTK::ExposeCT), &ct);
#endif
		g_signal_connect(G_OBJECT(widcdrw), "button_press_event",
				 G_CALLBACK(ScintillaGTK::PressCT), this);
		gtk_widget_set_events(widcdrw,
				      GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
		GtkWidget *top = gtk_widget_get_toplevel(PWidget(wMain));
		gtk_window_set_transient_for(GTK_WINDOW(PWidget(ct.wCallTip)), GTK_WINDOW(top));
	}
	const int width = static_cast<int>(rc.Width());
	const int height = static_cast<int>(rc.Height());
	gtk_widget_set_size_request(PWidget(ct.wDraw), width, height);
	ct.wDraw.Show();
	if (PWindow(ct.wCallTip)) {
		gdk_window_resize(PWindow(ct.wCallTip), width, height);
	}
}

void ScintillaGTK::AddToPopUp(const char *label, int cmd, bool enabled) {
	GtkWidget *menuItem;
	if (label[0])
		menuItem = gtk_menu_item_new_with_label(label);
	else
		menuItem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(popup.GetID()), menuItem);
	g_object_set_data(G_OBJECT(menuItem), "CmdNum", GINT_TO_POINTER(cmd));
	g_signal_connect(G_OBJECT(menuItem), "activate", G_CALLBACK(PopUpCB), this);

	if (cmd) {
		if (menuItem)
			gtk_widget_set_sensitive(menuItem, enabled);
	}
}

bool ScintillaGTK::OwnPrimarySelection() {
	return primarySelection;
}

void ScintillaGTK::ClearPrimarySelection() {
	if (primarySelection) {
		inClearSelection++;
		// Calls PrimaryClearSelection: primarySelection -> false
		gtk_clipboard_clear(gtk_clipboard_get(GDK_SELECTION_PRIMARY));
		inClearSelection--;
	}
}

void ScintillaGTK::PrimaryGetSelectionThis(GtkClipboard *clip, GtkSelectionData *selection_data, guint info) {
	try {
		if (SelectionOfGSD(selection_data) == GDK_SELECTION_PRIMARY) {
			if (primary.Empty()) {
				CopySelectionRange(&primary);
			}
			GetSelection(selection_data, info, &primary);
		}
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::PrimaryGetSelection(GtkClipboard *clip, GtkSelectionData *selection_data, guint info, gpointer pSci) {
	static_cast<ScintillaGTK *>(pSci)->PrimaryGetSelectionThis(clip, selection_data, info);
}

void ScintillaGTK::PrimaryClearSelectionThis(GtkClipboard *clip) {
	try {
		primarySelection = false;
		primary.Clear();
		if (!inClearSelection) {
			// Called because of another application or window claiming primary selection
			// so redraw to show selection in secondary colour.
			Redraw();
		}
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::PrimaryClearSelection(GtkClipboard *clip, gpointer pSci) {
	static_cast<ScintillaGTK *>(pSci)->PrimaryClearSelectionThis(clip);
}

void ScintillaGTK::ClaimSelection() {
	// X Windows has a 'primary selection' as well as the clipboard.
	// Whenever the user selects some text, we become the primary selection
	if (!sel.Empty()) {
		ClearPrimarySelection();
		if (gtk_clipboard_set_with_data(
			gtk_clipboard_get(GDK_SELECTION_PRIMARY),
			clipboardCopyTargets, nClipboardCopyTargets,
			PrimaryGetSelection,
			PrimaryClearSelection,
			this)) {
			primarySelection = true;
		}
	}
}

bool ScintillaGTK::IsStringAtom(GdkAtom type) {
	return (type == GDK_TARGET_STRING) || (type == atomUTF8) || (type == atomUTF8Mime);
}

// Detect rectangular text, convert line ends to current mode, convert from or to UTF-8
void ScintillaGTK::GetGtkSelectionText(GtkSelectionData *selectionData, SelectionText &selText) {
	const char *data = reinterpret_cast<const char *>(DataOfGSD(selectionData));
	int len = LengthOfGSD(selectionData);
	GdkAtom selectionTypeData = TypeOfGSD(selectionData);

	// Return empty string if selection is not a string
	if (!IsStringAtom(selectionTypeData)) {
		selText.Clear();
		return;
	}

	// Check for "\n\0" ending to string indicating that selection is rectangular
	bool isRectangular;
#if PLAT_GTK_WIN32
	isRectangular = ::IsClipboardFormatAvailable(cfColumnSelect) != 0;
#else
	isRectangular = ((len > 2) && (data[len - 1] == 0 && data[len - 2] == '\n'));
	if (isRectangular)
		len--;	// Forget the extra '\0'
#endif

#if PLAT_GTK_WIN32
	// Win32 includes an ending '\0' byte in 'len' for clipboard text from
	// external applications; ignore it.
	if ((len > 0) && (data[len - 1] == '\0'))
		len--;
#endif

	std::string dest(data, len);
	if (selectionTypeData == GDK_TARGET_STRING) {
		if (IsUnicodeMode()) {
			// Unknown encoding so assume in Latin1
			dest = UTF8FromLatin1(dest);
			selText.Copy(dest, CpUtf8, CharacterSet::Ansi, isRectangular, false);
		} else {
			// Assume buffer is in same encoding as selection
			selText.Copy(dest, pdoc->dbcsCodePage,
				     vs.styles[STYLE_DEFAULT].characterSet, isRectangular, false);
		}
	} else {	// UTF-8
		const char *charSetBuffer = CharacterSetID();
		if (!IsUnicodeMode() && *charSetBuffer) {
			// Convert to locale
			dest = ConvertText(dest.c_str(), dest.length(), charSetBuffer, "UTF-8", true);
			selText.Copy(dest, pdoc->dbcsCodePage,
				     vs.styles[STYLE_DEFAULT].characterSet, isRectangular, false);
		} else {
			selText.Copy(dest, CpUtf8, CharacterSet::Ansi, isRectangular, false);
		}
	}
}

void ScintillaGTK::InsertSelection(GtkClipboard *clipBoard, GtkSelectionData *selectionData) {
	const gint length = gtk_selection_data_get_length(selectionData);
	const GdkAtom selection = gtk_selection_data_get_selection(selectionData);
	if (length >= 0) {
		SelectionText selText;
		GetGtkSelectionText(selectionData, selText);

		UndoGroup ug(pdoc);
		if (selection == GDK_SELECTION_CLIPBOARD) {
			ClearSelection(multiPasteMode == MultiPaste::Each);
		}
		if (selection == GDK_SELECTION_PRIMARY) {
			SetSelection(posPrimary, posPrimary);
		}

		InsertPasteShape(selText.Data(), selText.Length(),
				 selText.rectangular ? PasteShape::rectangular : PasteShape::stream);
		EnsureCaretVisible();
	} else {
		if (selection == GDK_SELECTION_PRIMARY) {
			SetSelection(posPrimary, posPrimary);
		}
		GdkAtom target = gtk_selection_data_get_target(selectionData);
		if (target == atomUTF8) {
			// In case data is actually only stored as text/plain;charset=utf-8 not UTF8_STRING
			gtk_clipboard_request_contents(clipBoard, atomUTF8Mime,
					 SelectionReceiver::ClipboardReceived,
					 new SelectionReceiver(this)
			);
		}
	}
	Redraw();
}

GObject *ScintillaGTK::MainObject() const noexcept {
	return G_OBJECT(PWidget(wMain));
}

void ScintillaGTK::ReceivedClipboard(GtkClipboard *clipBoard, GtkSelectionData *selection_data) noexcept {
	try {
		InsertSelection(clipBoard, selection_data);
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::ReceivedSelection(GtkSelectionData *selection_data) {
	try {
		if ((SelectionOfGSD(selection_data) == GDK_SELECTION_CLIPBOARD) ||
				(SelectionOfGSD(selection_data) == GDK_SELECTION_PRIMARY)) {
			if ((atomSought == atomUTF8) && (LengthOfGSD(selection_data) <= 0)) {
				atomSought = atomString;
				gtk_selection_convert(GTK_WIDGET(PWidget(wMain)),
						      SelectionOfGSD(selection_data), atomSought, GDK_CURRENT_TIME);
			} else if ((LengthOfGSD(selection_data) > 0) && IsStringAtom(TypeOfGSD(selection_data))) {
				GtkClipboard *clipBoard = gtk_widget_get_clipboard(GTK_WIDGET(PWidget(wMain)), SelectionOfGSD(selection_data));
				InsertSelection(clipBoard, selection_data);
			}
		}
//	else fprintf(stderr, "Target non string %d %d\n", (int)(selection_data->type),
//		(int)(atomUTF8));
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::ReceivedDrop(GtkSelectionData *selection_data) {
	dragWasDropped = true;
	if (TypeOfGSD(selection_data) == atomUriList || TypeOfGSD(selection_data) == atomDROPFILES_DND) {
		const char *data = reinterpret_cast<const char *>(DataOfGSD(selection_data));
		std::vector<char> drop(data, data + LengthOfGSD(selection_data));
		drop.push_back('\0');
		NotifyURIDropped(&drop[0]);
	} else if (IsStringAtom(TypeOfGSD(selection_data))) {
		if (LengthOfGSD(selection_data) > 0) {
			SelectionText selText;
			GetGtkSelectionText(selection_data, selText);
			DropAt(posDrop, selText.Data(), selText.Length(), false, selText.rectangular);
		}
	} else if (LengthOfGSD(selection_data) > 0) {
		//~ fprintf(stderr, "ReceivedDrop other %p\n", static_cast<void *>(selection_data->type));
	}
	Redraw();
}



void ScintillaGTK::GetSelection(GtkSelectionData *selection_data, guint info, SelectionText *text) {
#if PLAT_GTK_WIN32
	// GDK on Win32 expands any \n into \r\n, so make a copy of
	// the clip text now with newlines converted to \n.  Use { } to hide symbols
	// from code below
	std::unique_ptr<SelectionText> newline_normalized;
	{
		std::string tmpstr = Document::TransformLineEnds(text->Data(), text->Length(), EndOfLine::Lf);
		newline_normalized = std::make_unique<SelectionText>();
		newline_normalized->Copy(tmpstr, CpUtf8, CharacterSet::Ansi, text->rectangular, false);
		text = newline_normalized.get();
	}
#endif

	// Convert text to utf8 if it isn't already
	std::unique_ptr<SelectionText> converted;
	if ((text->codePage != SC_CP_UTF8) && (info == TARGET_UTF8_STRING)) {
		const char *charSet = ::CharacterSetID(text->characterSet);
		if (*charSet) {
			std::string tmputf = ConvertText(text->Data(), text->Length(), "UTF-8", charSet, false);
			converted = std::make_unique<SelectionText>();
			converted->Copy(tmputf, CpUtf8, CharacterSet::Ansi, text->rectangular, false);
			text = converted.get();
		}
	}

	// Here is a somewhat evil kludge.
	// As I can not work out how to store data on the clipboard in multiple formats
	// and need some way to mark the clipping as being stream or rectangular,
	// the terminating \0 is included in the length for rectangular clippings.
	// All other tested applications behave benignly by ignoring the \0.
	// The #if is here because on Windows cfColumnSelect clip entry is used
	// instead as standard indicator of rectangularness (so no need to kludge)
	const char *textData = text->Data();
	gint len = static_cast<gint>(text->Length());
#if PLAT_GTK_WIN32 == 0
	if (text->rectangular)
		len++;
#endif

	if (info == TARGET_UTF8_STRING) {
		gtk_selection_data_set_text(selection_data, textData, len);
	} else {
		gtk_selection_data_set(selection_data,
				       static_cast<GdkAtom>(GDK_SELECTION_TYPE_STRING),
				       8, reinterpret_cast<const guchar *>(textData), len);
	}
}

void ScintillaGTK::StoreOnClipboard(SelectionText *clipText) {
	GtkClipboard *clipBoard =
		gtk_widget_get_clipboard(GTK_WIDGET(PWidget(wMain)), GDK_SELECTION_CLIPBOARD);
	if (clipBoard == nullptr) // Occurs if widget isn't in a toplevel
		return;

	if (gtk_clipboard_set_with_data(clipBoard, clipboardCopyTargets, nClipboardCopyTargets,
					ClipboardGetSelection, ClipboardClearSelection, clipText)) {
		gtk_clipboard_set_can_store(clipBoard, clipboardCopyTargets, nClipboardCopyTargets);
	}
}

void ScintillaGTK::ClipboardGetSelection(GtkClipboard *, GtkSelectionData *selection_data, guint info, void *data) {
	GetSelection(selection_data, info, static_cast<SelectionText *>(data));
}

void ScintillaGTK::ClipboardClearSelection(GtkClipboard *, void *data) {
	SelectionText *obj = static_cast<SelectionText *>(data);
	delete obj;
}

void ScintillaGTK::UnclaimSelection(GdkEventSelection *selection_event) {
	try {
		//Platform::DebugPrintf("UnclaimSelection\n");
		if (selection_event->selection == GDK_SELECTION_PRIMARY) {
			//Platform::DebugPrintf("UnclaimPrimarySelection\n");
			if (!OwnPrimarySelection()) {
				primary.Clear();
				primarySelection = false;
				FullPaint();
			}
		}
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::Resize(int width, int height) {
	//Platform::DebugPrintf("Resize %d %d\n", width, height);
	//printf("Resize %d %d\n", width, height);

	// GTK+ 3 warns when we allocate smaller than the minimum allocation,
	// so we use these variables to store the minimum scrollbar lengths.
	int minVScrollBarHeight, minHScrollBarWidth;

	// Not always needed, but some themes can have different sizes of scrollbars
#if GTK_CHECK_VERSION(3,0,0)
	GtkRequisition minimum, requisition;
	gtk_widget_get_preferred_size(PWidget(scrollbarv), &minimum, &requisition);
	minVScrollBarHeight = minimum.height;
	verticalScrollBarWidth = requisition.width;
	gtk_widget_get_preferred_size(PWidget(scrollbarh), &minimum, &requisition);
	minHScrollBarWidth = minimum.width;
	horizontalScrollBarHeight = requisition.height;
#else
	minVScrollBarHeight = minHScrollBarWidth = 1;
	verticalScrollBarWidth = GTK_WIDGET(PWidget(scrollbarv))->requisition.width;
	horizontalScrollBarHeight = GTK_WIDGET(PWidget(scrollbarh))->requisition.height;
#endif

	// These allocations should never produce negative sizes as they would wrap around to huge
	// unsigned numbers inside GTK+ causing warnings.
	const bool showSBHorizontal = horizontalScrollBarVisible && !Wrapping();

	GtkAllocation alloc = {};
	if (showSBHorizontal) {
		gtk_widget_show(GTK_WIDGET(PWidget(scrollbarh)));
		alloc.x = 0;
		alloc.y = height - horizontalScrollBarHeight;
		alloc.width = std::max(minHScrollBarWidth, width - verticalScrollBarWidth);
		alloc.height = horizontalScrollBarHeight;
		gtk_widget_size_allocate(GTK_WIDGET(PWidget(scrollbarh)), &alloc);
	} else {
		gtk_widget_hide(GTK_WIDGET(PWidget(scrollbarh)));
		horizontalScrollBarHeight = 0; // in case horizontalScrollBarVisible is true.
	}

	if (verticalScrollBarVisible) {
		gtk_widget_show(GTK_WIDGET(PWidget(scrollbarv)));
		alloc.x = width - verticalScrollBarWidth;
		alloc.y = 0;
		alloc.width = verticalScrollBarWidth;
		alloc.height = std::max(minVScrollBarHeight, height - horizontalScrollBarHeight);
		gtk_widget_size_allocate(GTK_WIDGET(PWidget(scrollbarv)), &alloc);
	} else {
		gtk_widget_hide(GTK_WIDGET(PWidget(scrollbarv)));
		verticalScrollBarWidth = 0;
	}
	SetClientRectangle();
	if (IS_WIDGET_MAPPED(PWidget(wMain))) {
		ChangeSize();
	} else {
		const PRectangle rcTextArea = GetTextRectangle();
		if (wrapWidth != rcTextArea.Width()) {
			wrapWidth = rcTextArea.Width();
			NeedWrapping();
		}
	}

	alloc.x = 0;
	alloc.y = 0;
	alloc.width = 1;
	alloc.height = 1;
#if GTK_CHECK_VERSION(3, 0, 0)
	// please GTK 3.20 and ask wText what size it wants, although we know it doesn't really need
	// anything special as it's ours.
	gtk_widget_get_preferred_size(PWidget(wText), &requisition, nullptr);
	alloc.width = requisition.width;
	alloc.height = requisition.height;
#endif
	alloc.width = std::max(alloc.width, width - verticalScrollBarWidth);
	alloc.height = std::max(alloc.height, height - horizontalScrollBarHeight);
	gtk_widget_size_allocate(GTK_WIDGET(PWidget(wText)), &alloc);
}

namespace {

void SetAdjustmentValue(GtkAdjustment *object, int value) noexcept {
	GtkAdjustment *adjustment = GTK_ADJUSTMENT(object);
	const int maxValue = static_cast<int>(
				     gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));

	if (value > maxValue)
		value = maxValue;
	if (value < 0)
		value = 0;
	gtk_adjustment_set_value(adjustment, value);
}

int modifierTranslated(int sciModifier) noexcept {
	switch (sciModifier) {
	case SCMOD_SHIFT:
		return GDK_SHIFT_MASK;
	case SCMOD_CTRL:
		return GDK_CONTROL_MASK;
	case SCMOD_ALT:
		return GDK_MOD1_MASK;
	case SCMOD_SUPER:
		return GDK_MOD4_MASK;
	default:
		return 0;
	}
}

Point PointOfEvent(const GdkEventButton *event) noexcept {
	// Use floor as want to round in the same direction (-infinity) so
	// there is no stickiness crossing 0.0.
	return Point(static_cast<XYPOSITION>(std::floor(event->x)), static_cast<XYPOSITION>(std::floor(event->y)));
}

}

gint ScintillaGTK::PressThis(GdkEventButton *event) {
	try {
		//Platform::DebugPrintf("Press %x time=%d state = %x button = %x\n",this,event->time, event->state, event->button);
		// Do not use GTK+ double click events as Scintilla has its own double click detection
		if (event->type != GDK_BUTTON_PRESS)
			return FALSE;

		evbtn.reset(gdk_event_copy(reinterpret_cast<GdkEvent *>(event)));
		buttonMouse = event->button;
		const Point pt = PointOfEvent(event);
		const PRectangle rcClient = GetClientRectangle();
		//Platform::DebugPrintf("Press %0d,%0d in %0d,%0d %0d,%0d\n",
		//	pt.x, pt.y, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
		if ((pt.x > rcClient.right) || (pt.y > rcClient.bottom)) {
			Platform::DebugPrintf("Bad location\n");
			return FALSE;
		}

		const bool shift = (event->state & GDK_SHIFT_MASK) != 0;
		bool ctrl = (event->state & GDK_CONTROL_MASK) != 0;
		// On X, instead of sending literal modifiers use the user specified
		// modifier, defaulting to control instead of alt.
		// This is because most X window managers grab alt + click for moving
		const bool alt = (event->state & modifierTranslated(rectangularSelectionModifier)) != 0;

		gtk_widget_grab_focus(PWidget(wMain));
		if (event->button == 1) {
#if PLAT_GTK_MACOSX
			const bool meta = ctrl;
			// GDK reports the Command modifier key as GDK_MOD2_MASK for button events,
			// not GDK_META_MASK like in key events.
			ctrl = (event->state & GDK_MOD2_MASK) != 0;
#else
			const bool meta = false;
#endif
			ButtonDownWithModifiers(pt, event->time, ModifierFlags(shift, ctrl, alt, meta));
		} else if (event->button == 2) {
			// Grab the primary selection if it exists
			posPrimary = SPositionFromLocation(pt, false, false, UserVirtualSpace());
			if (OwnPrimarySelection() && primary.Empty())
				CopySelectionRange(&primary);

			sel.Clear();
			RequestSelection(GDK_SELECTION_PRIMARY);
		} else if (event->button == 3) {
			if (!PointInSelection(pt))
				SetEmptySelection(PositionFromLocation(pt));
			if (ShouldDisplayPopup(pt)) {
				// PopUp menu
				// Convert to screen
				int ox = 0;
				int oy = 0;
				gdk_window_get_origin(PWindow(wMain), &ox, &oy);
				ContextMenu(Point(pt.x + ox, pt.y + oy));
			} else {
#if PLAT_GTK_MACOSX
				const bool meta = ctrl;
				// GDK reports the Command modifier key as GDK_MOD2_MASK for button events,
				// not GDK_META_MASK like in key events.
				ctrl = (event->state & GDK_MOD2_MASK) != 0;
#else
				const bool meta = false;
#endif
				RightButtonDownWithModifiers(pt, event->time, ModifierFlags(shift, ctrl, alt, meta));
				return FALSE;
			}
		} else if (event->button == 4) {
			// Wheel scrolling up (only GTK 1.x does it this way)
			if (ctrl)
				SetAdjustmentValue(adjustmenth, xOffset - 6);
			else
				SetAdjustmentValue(adjustmentv, static_cast<int>(topLine) - 3);
		} else if (event->button == 5) {
			// Wheel scrolling down (only GTK 1.x does it this way)
			if (ctrl)
				SetAdjustmentValue(adjustmenth, xOffset + 6);
			else
				SetAdjustmentValue(adjustmentv, static_cast<int>(topLine) + 3);
		}
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return TRUE;
}

gint ScintillaGTK::Press(GtkWidget *widget, GdkEventButton *event) {
	if (event->window != WindowFromWidget(widget))
		return FALSE;
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->PressThis(event);
}

gint ScintillaGTK::MouseRelease(GtkWidget *widget, GdkEventButton *event) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		//Platform::DebugPrintf("Release %x %d %d\n",sciThis,event->time,event->state);
		if (!sciThis->HaveMouseCapture())
			return FALSE;
		if (event->button == 1) {
			Point pt = PointOfEvent(event);
			//Platform::DebugPrintf("Up %x %x %d %d %d\n",
			//	sciThis,event->window,event->time, pt.x, pt.y);
			if (event->window != PWindow(sciThis->wMain))
				// If mouse released on scroll bar then the position is relative to the
				// scrollbar, not the drawing window so just repeat the most recent point.
				pt = sciThis->ptMouseLast;
			const KeyMod modifiers = ModifierFlags(
						      (event->state & GDK_SHIFT_MASK) != 0,
						      (event->state & GDK_CONTROL_MASK) != 0,
						      (event->state & modifierTranslated(sciThis->rectangularSelectionModifier)) != 0);
			sciThis->ButtonUpWithModifiers(pt, event->time, modifiers);
		}
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
	return FALSE;
}

// win32gtk and GTK >= 2 use SCROLL_* events instead of passing the
// button4/5/6/7 events to the GTK app
gint ScintillaGTK::ScrollEvent(GtkWidget *widget, GdkEventScroll *event) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {

		if (widget == nullptr || event == nullptr)
			return FALSE;

#if defined(GDK_WINDOWING_WAYLAND)
		if (event->direction == GDK_SCROLL_SMOOTH && GDK_IS_WAYLAND_WINDOW(event->window)) {
			const int smoothScrollFactor = 4;
			sciThis->smoothScrollY += event->delta_y * smoothScrollFactor;
			sciThis->smoothScrollX += event->delta_x * smoothScrollFactor;;
			if (ABS(sciThis->smoothScrollY) >= 1.0) {
				const int scrollLines = std::trunc(sciThis->smoothScrollY);
				sciThis->ScrollTo(sciThis->topLine + scrollLines);
				sciThis->smoothScrollY -= scrollLines;
			}
			if (ABS(sciThis->smoothScrollX) >= 1.0) {
				const int scrollPixels = std::trunc(sciThis->smoothScrollX);
				sciThis->HorizontalScrollTo(sciThis->xOffset + scrollPixels);
				sciThis->smoothScrollX -= scrollPixels;
			}
			return TRUE;
		}
#endif

		// Compute amount and direction to scroll (even tho on win32 there is
		// intensity of scrolling info in the native message, gtk doesn't
		// support this so we simulate similarly adaptive scrolling)
		// Note that this is disabled on macOS (Darwin) with the X11 backend
		// where the X11 server already has an adaptive scrolling algorithm
		// that fights with this one
		int cLineScroll;
#if (defined(__APPLE__) || defined(PLAT_GTK_WIN32)) && !defined(GDK_WINDOWING_QUARTZ)
		cLineScroll = sciThis->linesPerScroll;
		if (cLineScroll == 0)
			cLineScroll = 4;
		sciThis->wheelMouseIntensity = cLineScroll;
#else
		const gint64 curTime = g_get_monotonic_time();
		const gint64 timeDelta = curTime - sciThis->lastWheelMouseTime;
		if ((event->direction == sciThis->lastWheelMouseDirection) && (timeDelta < 250000)) {
			if (sciThis->wheelMouseIntensity < 12)
				sciThis->wheelMouseIntensity++;
			cLineScroll = sciThis->wheelMouseIntensity;
		} else {
			cLineScroll = sciThis->linesPerScroll;
			if (cLineScroll == 0)
				cLineScroll = 4;
			sciThis->wheelMouseIntensity = cLineScroll;
		}
		sciThis->lastWheelMouseTime = curTime;
#endif
		if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_LEFT) {
			cLineScroll *= -1;
		}
		sciThis->lastWheelMouseDirection = event->direction;

		// Note:  Unpatched versions of win32gtk don't set the 'state' value so
		// only regular scrolling is supported there.  Also, unpatched win32gtk
		// issues spurious button 2 mouse events during wheeling, which can cause
		// problems (a patch for both was submitted by archaeopteryx.com on 13Jun2001)

#if GTK_CHECK_VERSION(3,4,0)
		// Smooth scrolling not supported
		if (event->direction == GDK_SCROLL_SMOOTH) {
			return FALSE;
		}
#endif

		// Horizontal scrolling
		if (event->direction == GDK_SCROLL_LEFT || event->direction == GDK_SCROLL_RIGHT || event->state & GDK_SHIFT_MASK) {
			int hScroll = gtk_adjustment_get_step_increment(sciThis->adjustmenth);
			hScroll *= cLineScroll; // scroll by this many characters
			sciThis->HorizontalScrollTo(sciThis->xOffset + hScroll);

			// Text font size zoom
		} else if (event->state & GDK_CONTROL_MASK) {
			if (cLineScroll < 0) {
				sciThis->KeyCommand(Message::ZoomIn);
			} else {
				sciThis->KeyCommand(Message::ZoomOut);
			}

			// Regular scrolling
		} else {
			sciThis->ScrollTo(sciThis->topLine + cLineScroll);
		}
		return TRUE;
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
	return FALSE;
}

gint ScintillaGTK::Motion(GtkWidget *widget, GdkEventMotion *event) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		//Platform::DebugPrintf("Motion %x %d\n",sciThis,event->time);
		if (event->window != WindowFromWidget(widget))
			return FALSE;
		int x = 0;
		int y = 0;
		GdkModifierType state {};
		if (event->is_hint) {
#if GTK_CHECK_VERSION(3,0,0)
			gdk_window_get_device_position(event->window,
						       event->device, &x, &y, &state);
#else
			gdk_window_get_pointer(event->window, &x, &y, &state);
#endif
		} else {
			x = static_cast<int>(event->x);
			y = static_cast<int>(event->y);
			state = static_cast<GdkModifierType>(event->state);
		}
		//Platform::DebugPrintf("Move %x %x %d %c %d %d\n",
		//	sciThis,event->window,event->time,event->is_hint? 'h' :'.', x, y);
		const Point pt(static_cast<XYPOSITION>(x), static_cast<XYPOSITION>(y));
		const KeyMod modifiers = ModifierFlags(
					      (event->state & GDK_SHIFT_MASK) != 0,
					      (event->state & GDK_CONTROL_MASK) != 0,
					      (event->state & modifierTranslated(sciThis->rectangularSelectionModifier)) != 0);
		sciThis->ButtonMoveWithModifiers(pt, event->time, modifiers);
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
	return FALSE;
}

namespace {

// Map the keypad keys to their equivalent functions
int KeyTranslate(int keyIn) noexcept {
	switch (keyIn) {
#if GTK_CHECK_VERSION(3,0,0)
	case GDK_KEY_ISO_Left_Tab:
		return SCK_TAB;
	case GDK_KEY_KP_Down:
		return SCK_DOWN;
	case GDK_KEY_KP_Up:
		return SCK_UP;
	case GDK_KEY_KP_Left:
		return SCK_LEFT;
	case GDK_KEY_KP_Right:
		return SCK_RIGHT;
	case GDK_KEY_KP_Home:
		return SCK_HOME;
	case GDK_KEY_KP_End:
		return SCK_END;
	case GDK_KEY_KP_Page_Up:
		return SCK_PRIOR;
	case GDK_KEY_KP_Page_Down:
		return SCK_NEXT;
	case GDK_KEY_KP_Delete:
		return SCK_DELETE;
	case GDK_KEY_KP_Insert:
		return SCK_INSERT;
	case GDK_KEY_KP_Enter:
		return SCK_RETURN;

	case GDK_KEY_Down:
		return SCK_DOWN;
	case GDK_KEY_Up:
		return SCK_UP;
	case GDK_KEY_Left:
		return SCK_LEFT;
	case GDK_KEY_Right:
		return SCK_RIGHT;
	case GDK_KEY_Home:
		return SCK_HOME;
	case GDK_KEY_End:
		return SCK_END;
	case GDK_KEY_Page_Up:
		return SCK_PRIOR;
	case GDK_KEY_Page_Down:
		return SCK_NEXT;
	case GDK_KEY_Delete:
		return SCK_DELETE;
	case GDK_KEY_Insert:
		return SCK_INSERT;
	case GDK_KEY_Escape:
		return SCK_ESCAPE;
	case GDK_KEY_BackSpace:
		return SCK_BACK;
	case GDK_KEY_Tab:
		return SCK_TAB;
	case GDK_KEY_Return:
		return SCK_RETURN;
	case GDK_KEY_KP_Add:
		return SCK_ADD;
	case GDK_KEY_KP_Subtract:
		return SCK_SUBTRACT;
	case GDK_KEY_KP_Divide:
		return SCK_DIVIDE;
	case GDK_KEY_Super_L:
		return SCK_WIN;
	case GDK_KEY_Super_R:
		return SCK_RWIN;
	case GDK_KEY_Menu:
		return SCK_MENU;

#else

	case GDK_ISO_Left_Tab:
		return SCK_TAB;
	case GDK_KP_Down:
		return SCK_DOWN;
	case GDK_KP_Up:
		return SCK_UP;
	case GDK_KP_Left:
		return SCK_LEFT;
	case GDK_KP_Right:
		return SCK_RIGHT;
	case GDK_KP_Home:
		return SCK_HOME;
	case GDK_KP_End:
		return SCK_END;
	case GDK_KP_Page_Up:
		return SCK_PRIOR;
	case GDK_KP_Page_Down:
		return SCK_NEXT;
	case GDK_KP_Delete:
		return SCK_DELETE;
	case GDK_KP_Insert:
		return SCK_INSERT;
	case GDK_KP_Enter:
		return SCK_RETURN;

	case GDK_Down:
		return SCK_DOWN;
	case GDK_Up:
		return SCK_UP;
	case GDK_Left:
		return SCK_LEFT;
	case GDK_Right:
		return SCK_RIGHT;
	case GDK_Home:
		return SCK_HOME;
	case GDK_End:
		return SCK_END;
	case GDK_Page_Up:
		return SCK_PRIOR;
	case GDK_Page_Down:
		return SCK_NEXT;
	case GDK_Delete:
		return SCK_DELETE;
	case GDK_Insert:
		return SCK_INSERT;
	case GDK_Escape:
		return SCK_ESCAPE;
	case GDK_BackSpace:
		return SCK_BACK;
	case GDK_Tab:
		return SCK_TAB;
	case GDK_Return:
		return SCK_RETURN;
	case GDK_KP_Add:
		return SCK_ADD;
	case GDK_KP_Subtract:
		return SCK_SUBTRACT;
	case GDK_KP_Divide:
		return SCK_DIVIDE;
	case GDK_Super_L:
		return SCK_WIN;
	case GDK_Super_R:
		return SCK_RWIN;
	case GDK_Menu:
		return SCK_MENU;
#endif
	default:
		return keyIn;
	}
}

}

gboolean ScintillaGTK::KeyThis(GdkEventKey *event) {
	try {
		//fprintf(stderr, "SC-key: %d %x [%s]\n",
		//	event->keyval, event->state, (event->length > 0) ? event->string : "empty");
		if (gtk_im_context_filter_keypress(im_context.get(), event)) {
			return 1;
		}
		if (!event->keyval) {
			return true;
		}

		const bool shift = (event->state & GDK_SHIFT_MASK) != 0;
		bool ctrl = (event->state & GDK_CONTROL_MASK) != 0;
		const bool alt = (event->state & GDK_MOD1_MASK) != 0;
		const bool super = (event->state & GDK_MOD4_MASK) != 0;
		guint key = event->keyval;
		if ((ctrl || alt) && (key < 128))
			key = toupper(key);
#if GTK_CHECK_VERSION(3,0,0)
		else if (!ctrl && (key >= GDK_KEY_KP_Multiply && key <= GDK_KEY_KP_9))
#else
		else if (!ctrl && (key >= GDK_KP_Multiply && key <= GDK_KP_9))
#endif
			key &= 0x7F;
		// Hack for keys over 256 and below command keys but makes Hungarian work.
		// This will have to change for Unicode
		else if (key >= 0xFE00)
			key = KeyTranslate(key);

		bool consumed = false;
#if !(PLAT_GTK_MACOSX)
		const bool meta = false;
#else
		const bool meta = ctrl;
		ctrl = (event->state & GDK_META_MASK) != 0;
#endif
		const bool added = KeyDownWithModifiers(static_cast<Keys>(key), ModifierFlags(shift, ctrl, alt, meta, super), &consumed) != 0;
		if (!consumed)
			consumed = added;
		//fprintf(stderr, "SK-key: %d %x %x\n",event->keyval, event->state, consumed);
		if (event->keyval == 0xffffff && event->length > 0) {
			ClearSelection();
			const Sci::Position lengthInserted = pdoc->InsertString(CurrentPosition(), event->string, strlen(event->string));
			if (lengthInserted > 0) {
				MovePositionTo(CurrentPosition() + lengthInserted);
			}
		}
		return consumed;
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return FALSE;
}

gboolean ScintillaGTK::KeyPress(GtkWidget *widget, GdkEventKey *event) {
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->KeyThis(event);
}

gboolean ScintillaGTK::KeyRelease(GtkWidget *widget, GdkEventKey *event) {
	//Platform::DebugPrintf("SC-keyrel: %d %x %3s\n",event->keyval, event->state, event->string);
	ScintillaGTK *sciThis = FromWidget(widget);
	if (gtk_im_context_filter_keypress(sciThis->im_context.get(), event)) {
		return TRUE;
	}
	return FALSE;
}

#if GTK_CHECK_VERSION(3,0,0)

gboolean ScintillaGTK::DrawPreeditThis(GtkWidget *, cairo_t *cr) {
	try {
		PreEditString pes(im_context.get());
		UniquePangoLayout layout(gtk_widget_create_pango_layout(PWidget(wText), pes.str));
		pango_layout_set_attributes(layout.get(), pes.attrs);

		cairo_move_to(cr, 0, 0);
		pango_cairo_show_layout(cr, layout.get());
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return TRUE;
}

gboolean ScintillaGTK::DrawPreedit(GtkWidget *widget, cairo_t *cr, ScintillaGTK *sciThis) {
	return sciThis->DrawPreeditThis(widget, cr);
}

#else

gboolean ScintillaGTK::ExposePreeditThis(GtkWidget *widget, GdkEventExpose *) {
	try {
		PreEditString pes(im_context.get());
		UniquePangoLayout layout(gtk_widget_create_pango_layout(PWidget(wText), pes.str));
		pango_layout_set_attributes(layout.get(), pes.attrs);

		UniqueCairo context(gdk_cairo_create(WindowFromWidget(widget)));
		cairo_move_to(context.get(), 0, 0);
		pango_cairo_show_layout(context.get(), layout.get());
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return TRUE;
}

gboolean ScintillaGTK::ExposePreedit(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis) {
	return sciThis->ExposePreeditThis(widget, ose);
}

#endif

bool ScintillaGTK::KoreanIME() {
	PreEditString pes(im_context.get());
	if (pes.pscript != G_UNICODE_SCRIPT_COMMON)
		lastNonCommonScript = pes.pscript;
	return lastNonCommonScript == G_UNICODE_SCRIPT_HANGUL;
}

void ScintillaGTK::MoveImeCarets(Sci::Position pos) {
	// Move carets relatively by bytes
	for (size_t r=0; r<sel.Count(); r++) {
		const Sci::Position positionInsert = sel.Range(r).Start().Position();
		sel.Range(r) = SelectionRange(positionInsert + pos);
	}
}

void ScintillaGTK::DrawImeIndicator(int indicator, Sci::Position len) {
	// Emulate the visual style of IME characters with indicators.
	// Draw an indicator on the character before caret by the character bytes of len
	// so it should be called after InsertCharacter().
	// It does not affect caret positions.
	if (indicator < 8 || indicator > INDICATOR_MAX) {
		return;
	}
	pdoc->DecorationSetCurrentIndicator(indicator);
	for (size_t r=0; r<sel.Count(); r++) {
		const Sci::Position positionInsert = sel.Range(r).Start().Position();
		pdoc->DecorationFillRange(positionInsert - len, 1, len);
	}
}

namespace {

std::vector<int> MapImeIndicators(PangoAttrList *attrs, const char *u8Str) {
	// Map input style to scintilla ime indicator.
	// Attrs position points between UTF-8 bytes.
	// Indicator index to be returned is character based though.
	const glong charactersLen = g_utf8_strlen(u8Str, strlen(u8Str));
	std::vector<int> indicator(charactersLen, SC_INDICATOR_UNKNOWN);

	PangoAttrIterator *iterunderline = pango_attr_list_get_iterator(attrs);
	if (iterunderline) {
		do {
			const PangoAttribute  *attrunderline = pango_attr_iterator_get(iterunderline, PANGO_ATTR_UNDERLINE);
			if (attrunderline) {
				const glong start = g_utf8_strlen(u8Str, attrunderline->start_index);
				const glong end = g_utf8_strlen(u8Str, attrunderline->end_index);
				const int ulinevalue = reinterpret_cast<const PangoAttrInt *>(attrunderline)->value;
				const PangoUnderline uline = static_cast<PangoUnderline>(ulinevalue);
				for (glong i=start; i < end; ++i) {
					switch (uline) {
					case PANGO_UNDERLINE_NONE:
						indicator[i] = SC_INDICATOR_UNKNOWN;
						break;
					case PANGO_UNDERLINE_SINGLE: // normal input
						indicator[i] = SC_INDICATOR_INPUT;
						break;
					case PANGO_UNDERLINE_DOUBLE:
					case PANGO_UNDERLINE_LOW:
					case PANGO_UNDERLINE_ERROR:
					default:
						break;
					}
				}
			}
		} while (pango_attr_iterator_next(iterunderline));
		pango_attr_iterator_destroy(iterunderline);
	}

	PangoAttrIterator *itercolor = pango_attr_list_get_iterator(attrs);
	if (itercolor) {
		do {
			const PangoAttribute *backcolor = pango_attr_iterator_get(itercolor, PANGO_ATTR_BACKGROUND);
			if (backcolor) {
				const glong start = g_utf8_strlen(u8Str, backcolor->start_index);
				const glong end = g_utf8_strlen(u8Str, backcolor->end_index);
				for (glong i=start; i < end; ++i) {
					indicator[i] = SC_INDICATOR_TARGET;  // target converted
				}
			}
		} while (pango_attr_iterator_next(itercolor));
		pango_attr_iterator_destroy(itercolor);
	}
	return indicator;
}

}

void ScintillaGTK::SetCandidateWindowPos() {
	// Composition box accompanies candidate box.
	const Point pt = PointMainCaret();
	GdkRectangle imeBox {};
	imeBox.x = static_cast<gint>(pt.x);
	imeBox.y = static_cast<gint>(pt.y + std::max(4, vs.lineHeight/4));
	// prevent overlapping with current line
	imeBox.height = vs.lineHeight;
	gtk_im_context_set_cursor_location(im_context.get(), &imeBox);
}

void ScintillaGTK::CommitThis(char *commitStr) {
	try {
		//~ fprintf(stderr, "Commit '%s'\n", commitStr);
		view.imeCaretBlockOverride = false;

		if (pdoc->TentativeActive()) {
			pdoc->TentativeUndo();
		}

		const char *charSetSource = CharacterSetID();

		glong uniStrLen = 0;
		gunichar *uniStr = g_utf8_to_ucs4_fast(commitStr, static_cast<glong>(strlen(commitStr)), &uniStrLen);
		for (glong i = 0; i < uniStrLen; i++) {
			gchar u8Char[UTF8MaxBytes+2] = {0};
			const gint u8CharLen = g_unichar_to_utf8(uniStr[i], u8Char);
			std::string docChar = u8Char;
			if (!IsUnicodeMode())
				docChar = ConvertText(u8Char, u8CharLen, charSetSource, "UTF-8", true);

			InsertCharacter(docChar, CharacterSource::DirectInput);
		}
		g_free(uniStr);
		ShowCaretAtCurrentPosition();
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::Commit(GtkIMContext *, char  *str, ScintillaGTK *sciThis) {
	sciThis->CommitThis(str);
}

void ScintillaGTK::PreeditChangedInlineThis() {
	// Copy & paste by johnsonj with a lot of helps of Neil
	// Great thanks for my foreruners, jiniya and BLUEnLIVE
	try {
		if (pdoc->IsReadOnly() || SelectionContainsProtected()) {
			gtk_im_context_reset(im_context.get());
			return;
		}

		view.imeCaretBlockOverride = false; // If backspace.

		bool initialCompose = false;
		if (pdoc->TentativeActive()) {
			pdoc->TentativeUndo();
		} else {
			// No tentative undo means start of this composition so
			// fill in any virtual spaces.
			initialCompose = true;
		}

		PreEditString preeditStr(im_context.get());
		const char *charSetSource = CharacterSetID();

		if (!preeditStr.validUTF8 || (charSetSource == nullptr)) {
			ShowCaretAtCurrentPosition();
			return;
		}

		if (preeditStr.uniStrLen == 0) {
			ShowCaretAtCurrentPosition();
			return;
		}

		if (initialCompose) {
			ClearBeforeTentativeStart();
		}

		SetCandidateWindowPos();
		pdoc->TentativeStart(); // TentativeActive() from now on

		std::vector<int> indicator = MapImeIndicators(preeditStr.attrs, preeditStr.str);

		for (glong i = 0; i < preeditStr.uniStrLen; i++) {
			gchar u8Char[UTF8MaxBytes+2] = {0};
			const gint u8CharLen = g_unichar_to_utf8(preeditStr.uniStr[i], u8Char);
			std::string docChar = u8Char;
			if (!IsUnicodeMode())
				docChar = ConvertText(u8Char, u8CharLen, charSetSource, "UTF-8", true);

			InsertCharacter(docChar, CharacterSource::TentativeInput);

			DrawImeIndicator(indicator[i], docChar.size());
		}

		// Move caret to ime cursor position.
		const int imeEndToImeCaretU32 = preeditStr.cursor_pos - preeditStr.uniStrLen;
		const Sci::Position imeCaretPosDoc = pdoc->GetRelativePosition(CurrentPosition(), imeEndToImeCaretU32);

		MoveImeCarets(- CurrentPosition() + imeCaretPosDoc);

		if (KoreanIME()) {
#if !PLAT_GTK_WIN32
			if (preeditStr.cursor_pos > 0) {
				int oneCharBefore = pdoc->GetRelativePosition(CurrentPosition(), -1);
				MoveImeCarets(- CurrentPosition() + oneCharBefore);
			}
#endif
			view.imeCaretBlockOverride = true;
		}

		EnsureCaretVisible();
		ShowCaretAtCurrentPosition();
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::PreeditChangedWindowedThis() {
	try {
		PreEditString pes(im_context.get());
		if (strlen(pes.str) > 0) {
			SetCandidateWindowPos();

			UniquePangoLayout layout(gtk_widget_create_pango_layout(PWidget(wText), pes.str));
			pango_layout_set_attributes(layout.get(), pes.attrs);

			gint w, h;
			pango_layout_get_pixel_size(layout.get(), &w, &h);

			gint x, y;
			gdk_window_get_origin(PWindow(wText), &x, &y);

			Point pt = PointMainCaret();
			if (pt.x < 0)
				pt.x = 0;
			if (pt.y < 0)
				pt.y = 0;

			gtk_window_move(GTK_WINDOW(PWidget(wPreedit)), x + static_cast<gint>(pt.x), y + static_cast<gint>(pt.y));
			gtk_window_resize(GTK_WINDOW(PWidget(wPreedit)), w, h);
			gtk_widget_show(PWidget(wPreedit));
			gtk_widget_queue_draw_area(PWidget(wPreeditDraw), 0, 0, w, h);
		} else {
			gtk_widget_hide(PWidget(wPreedit));
		}
	} catch (...) {
		errorStatus = Status::Failure;
	}
}

void ScintillaGTK::PreeditChanged(GtkIMContext *, ScintillaGTK *sciThis) {
	if ((sciThis->imeInteraction == IMEInteraction::Inline) || (sciThis->KoreanIME())) {
		sciThis->PreeditChangedInlineThis();
	} else {
		sciThis->PreeditChangedWindowedThis();
	}
}

bool ScintillaGTK::RetrieveSurroundingThis(GtkIMContext *context) {
	try {
		const Sci::Position pos = CurrentPosition();
		const int line = pdoc->LineFromPosition(pos);
		const Sci::Position startByte = pdoc->LineStart(line);
		const Sci::Position endByte = pdoc->LineEnd(line);

		std::string utf8Text;
		gint cursorIndex; // index of the cursor inside utf8Text, in bytes
		const char *charSetBuffer;

		if (IsUnicodeMode() || ! *(charSetBuffer = CharacterSetID())) {
			utf8Text = RangeText(startByte, endByte);
			cursorIndex = pos - startByte;
		} else {
			// Need to convert
			std::string tmpbuf = RangeText(startByte, pos);
			utf8Text = ConvertText(&tmpbuf[0], tmpbuf.length(), "UTF-8", charSetBuffer, false);
			cursorIndex = utf8Text.length();
			if (endByte > pos) {
				tmpbuf = RangeText(pos, endByte);
				utf8Text += ConvertText(&tmpbuf[0], tmpbuf.length(), "UTF-8", charSetBuffer, false);
			}
		}

		gtk_im_context_set_surrounding(context, &utf8Text[0], utf8Text.length(), cursorIndex);

		return true;
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return false;
}

gboolean ScintillaGTK::RetrieveSurrounding(GtkIMContext *context, ScintillaGTK *sciThis) {
	return sciThis->RetrieveSurroundingThis(context);
}

bool ScintillaGTK::DeleteSurroundingThis(GtkIMContext *, gint characterOffset, gint characterCount) {
	try {
		const Sci::Position startByte = pdoc->GetRelativePosition(CurrentPosition(), characterOffset);
		if (startByte == INVALID_POSITION)
			return false;

		const Sci::Position endByte = pdoc->GetRelativePosition(startByte, characterCount);
		if (endByte == INVALID_POSITION)
			return false;

		return pdoc->DeleteChars(startByte, endByte - startByte);
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return false;
}

gboolean ScintillaGTK::DeleteSurrounding(GtkIMContext *context, gint characterOffset, gint characterCount, ScintillaGTK *sciThis) {
	return sciThis->DeleteSurroundingThis(context, characterOffset, characterCount);
}

void ScintillaGTK::StyleSetText(GtkWidget *widget, GtkStyle *, void *) {
	RealizeText(widget, nullptr);
}

void ScintillaGTK::RealizeText(GtkWidget *widget, void *) {
	// Set NULL background to avoid automatic clearing so Scintilla responsible for all drawing
	if (WindowFromWidget(widget)) {
#if GTK_CHECK_VERSION(3,22,0)
		// Appears unnecessary
#elif GTK_CHECK_VERSION(3,0,0)
		gdk_window_set_background_pattern(WindowFromWidget(widget), nullptr);
#else
		gdk_window_set_back_pixmap(WindowFromWidget(widget), nullptr, FALSE);
#endif
	}
}

static GObjectClass *scintilla_class_parent_class;

void ScintillaGTK::Dispose(GObject *object) {
	try {
		ScintillaObject *scio = SCINTILLA(object);
		ScintillaGTK *sciThis = static_cast<ScintillaGTK *>(scio->pscin);

		if (PWidget(sciThis->scrollbarv)) {
			gtk_widget_unparent(PWidget(sciThis->scrollbarv));
			sciThis->scrollbarv = nullptr;
		}

		if (PWidget(sciThis->scrollbarh)) {
			gtk_widget_unparent(PWidget(sciThis->scrollbarh));
			sciThis->scrollbarh = nullptr;
		}

		scintilla_class_parent_class->dispose(object);
	} catch (...) {
		// Its dying so nowhere to save the status
	}
}

void ScintillaGTK::Destroy(GObject *object) {
	try {
		ScintillaObject *scio = SCINTILLA(object);

		// This avoids a double destruction
		if (!scio->pscin)
			return;
		ScintillaGTK *sciThis = static_cast<ScintillaGTK *>(scio->pscin);
		//Platform::DebugPrintf("Destroying %x %x\n", sciThis, object);
		sciThis->Finalise();

		delete sciThis;
		scio->pscin = nullptr;
		scintilla_class_parent_class->finalize(object);
	} catch (...) {
		// Its dead so nowhere to save the status
	}
}

void ScintillaGTK::CheckForFontOptionChange() {
	const FontOptions fontOptionsNow(PWidget(wText));
	if (!(fontOptionsNow == fontOptionsPrevious)) {
		// Clear position caches
		InvalidateStyleData();
	}
	fontOptionsPrevious = fontOptionsNow;
}

#if GTK_CHECK_VERSION(3,0,0)

gboolean ScintillaGTK::DrawTextThis(cairo_t *cr) {
	try {
		CheckForFontOptionChange();

		paintState = PaintState::painting;
		repaintFullWindow = false;

		rcPaint = GetClientRectangle();

		cairo_rectangle_list_t *oldRgnUpdate = rgnUpdate;
		rgnUpdate = cairo_copy_clip_rectangle_list(cr);
		if (rgnUpdate && rgnUpdate->status != CAIRO_STATUS_SUCCESS) {
			// If not successful then ignore
			fprintf(stderr, "DrawTextThis failed to copy update region %d [%d]\n", rgnUpdate->status, rgnUpdate->num_rectangles);
			cairo_rectangle_list_destroy(rgnUpdate);
			rgnUpdate = nullptr;
		}

		double x1, y1, x2, y2;
		cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
		rcPaint.left = x1;
		rcPaint.top = y1;
		rcPaint.right = x2;
		rcPaint.bottom = y2;
		PRectangle rcClient = GetClientRectangle();
		paintingAllText = rcPaint.Contains(rcClient);
		std::unique_ptr<Surface> surfaceWindow(Surface::Allocate(Technology::Default));
		surfaceWindow->Init(cr, PWidget(wText));
		Paint(surfaceWindow.get(), rcPaint);
		surfaceWindow->Release();
		if ((paintState == PaintState::abandoned) || repaintFullWindow) {
			// Painting area was insufficient to cover new styling or brace highlight positions
			FullPaint();
		}
		paintState = PaintState::notPainting;
		repaintFullWindow = false;

		if (rgnUpdate) {
			cairo_rectangle_list_destroy(rgnUpdate);
		}
		rgnUpdate = oldRgnUpdate;
		paintState = PaintState::notPainting;
	} catch (...) {
		errorStatus = Status::Failure;
	}

	return FALSE;
}

gboolean ScintillaGTK::DrawText(GtkWidget *, cairo_t *cr, ScintillaGTK *sciThis) {
	return sciThis->DrawTextThis(cr);
}

gboolean ScintillaGTK::DrawThis(cairo_t *cr) {
	try {
#ifdef GTK_STYLE_CLASS_SCROLLBARS_JUNCTION /* GTK >= 3.4 */
		// if both scrollbars are visible, paint the little square on the bottom right corner
		if (verticalScrollBarVisible && horizontalScrollBarVisible && !Wrapping()) {
			GtkStyleContext *styleContext = gtk_widget_get_style_context(PWidget(wMain));
			PRectangle rc = GetClientRectangle();

			gtk_style_context_save(styleContext);
			gtk_style_context_add_class(styleContext, GTK_STYLE_CLASS_SCROLLBARS_JUNCTION);

			gtk_render_background(styleContext, cr, rc.right, rc.bottom,
					      verticalScrollBarWidth, horizontalScrollBarHeight);
			gtk_render_frame(styleContext, cr, rc.right, rc.bottom,
					 verticalScrollBarWidth, horizontalScrollBarHeight);

			gtk_style_context_restore(styleContext);
		}
#endif

		gtk_container_propagate_draw(
			GTK_CONTAINER(PWidget(wMain)), PWidget(scrollbarh), cr);
		gtk_container_propagate_draw(
			GTK_CONTAINER(PWidget(wMain)), PWidget(scrollbarv), cr);
// Starting from the following version, the expose event are not propagated
// for double buffered non native windows, so we need to call it ourselves
// or keep the default handler
#if GTK_CHECK_VERSION(3,0,0)
		// we want to forward on any >= 3.9.2 runtime
		if (gtk_check_version(3, 9, 2) == nullptr) {
			gtk_container_propagate_draw(
				GTK_CONTAINER(PWidget(wMain)), PWidget(wText), cr);
		}
#endif
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return FALSE;
}

gboolean ScintillaGTK::DrawMain(GtkWidget *widget, cairo_t *cr) {
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->DrawThis(cr);
}

#else

gboolean ScintillaGTK::ExposeTextThis(GtkWidget * /*widget*/, GdkEventExpose *ose) {
	try {
		CheckForFontOptionChange();

		paintState = PaintState::painting;

		rcPaint = PRectangle::FromInts(
				  ose->area.x,
				  ose->area.y,
				  ose->area.x + ose->area.width,
				  ose->area.y + ose->area.height);

		GdkRegion *oldRgnUpdate = rgnUpdate;
		rgnUpdate = gdk_region_copy(ose->region);
		const PRectangle rcClient = GetClientRectangle();
		paintingAllText = rcPaint.Contains(rcClient);
		{
			std::unique_ptr<Surface> surfaceWindow(Surface::Allocate(Technology::Default));
			UniqueCairo cr(gdk_cairo_create(PWindow(wText)));
			surfaceWindow->Init(cr.get(), PWidget(wText));
			Paint(surfaceWindow.get(), rcPaint);
		}
		if ((paintState == PaintState::abandoned) || repaintFullWindow) {
			// Painting area was insufficient to cover new styling or brace highlight positions
			FullPaint();
		}
		paintState = PaintState::notPainting;
		repaintFullWindow = false;

		if (rgnUpdate) {
			gdk_region_destroy(rgnUpdate);
		}
		rgnUpdate = oldRgnUpdate;
	} catch (...) {
		errorStatus = Status::Failure;
	}

	return FALSE;
}

gboolean ScintillaGTK::ExposeText(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis) {
	return sciThis->ExposeTextThis(widget, ose);
}

gboolean ScintillaGTK::ExposeMain(GtkWidget *widget, GdkEventExpose *ose) {
	ScintillaGTK *sciThis = FromWidget(widget);
	//Platform::DebugPrintf("Expose Main %0d,%0d %0d,%0d\n",
	//ose->area.x, ose->area.y, ose->area.width, ose->area.height);
	return sciThis->Expose(widget, ose);
}

gboolean ScintillaGTK::Expose(GtkWidget *, GdkEventExpose *ose) {
	try {
		//fprintf(stderr, "Expose %0d,%0d %0d,%0d\n",
		//ose->area.x, ose->area.y, ose->area.width, ose->area.height);

		// The text is painted in ExposeText
		gtk_container_propagate_expose(
			GTK_CONTAINER(PWidget(wMain)), PWidget(scrollbarh), ose);
		gtk_container_propagate_expose(
			GTK_CONTAINER(PWidget(wMain)), PWidget(scrollbarv), ose);

	} catch (...) {
		errorStatus = Status::Failure;
	}
	return FALSE;
}

#endif

void ScintillaGTK::ScrollSignal(GtkAdjustment *adj, ScintillaGTK *sciThis) {
	try {
		sciThis->ScrollTo(static_cast<int>(gtk_adjustment_get_value(adj)), false);
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
}

void ScintillaGTK::ScrollHSignal(GtkAdjustment *adj, ScintillaGTK *sciThis) {
	try {
		sciThis->HorizontalScrollTo(static_cast<int>(gtk_adjustment_get_value(adj)));
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
}

void ScintillaGTK::SelectionReceived(GtkWidget *widget,
				     GtkSelectionData *selection_data, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	//Platform::DebugPrintf("Selection received\n");
	sciThis->ReceivedSelection(selection_data);
}

void ScintillaGTK::SelectionGet(GtkWidget *widget,
				GtkSelectionData *selection_data, guint info, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		//Platform::DebugPrintf("Selection get\n");
		if (SelectionOfGSD(selection_data) == GDK_SELECTION_PRIMARY) {
			if (sciThis->primary.Empty()) {
				sciThis->CopySelectionRange(&sciThis->primary);
			}
			sciThis->GetSelection(selection_data, info, &sciThis->primary);
		}
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
}

gint ScintillaGTK::SelectionClear(GtkWidget *widget, GdkEventSelection *selection_event) {
	ScintillaGTK *sciThis = FromWidget(widget);
	//Platform::DebugPrintf("Selection clear\n");
	sciThis->UnclaimSelection(selection_event);
	if (GTK_WIDGET_CLASS(sciThis->parentClass)->selection_clear_event) {
		return GTK_WIDGET_CLASS(sciThis->parentClass)->selection_clear_event(widget, selection_event);
	}
	return TRUE;
}

gboolean ScintillaGTK::DragMotionThis(GdkDragContext *context,
				      gint x, gint y, guint dragtime) {
	try {
		const Point npt = Point::FromInts(x, y);
		SetDragPosition(SPositionFromLocation(npt, false, false, UserVirtualSpace()));
		GdkDragAction preferredAction = gdk_drag_context_get_suggested_action(context);
		const GdkDragAction actions = gdk_drag_context_get_actions(context);
		const SelectionPosition pos = SPositionFromLocation(npt);
		if ((inDragDrop == DragDrop::dragging) && (PositionInSelection(pos.Position()))) {
			// Avoid dragging selection onto itself as that produces a move
			// with no real effect but which creates undo actions.
			preferredAction = static_cast<GdkDragAction>(0);
		} else if (actions == actionCopyOrMove) {
			preferredAction = GDK_ACTION_MOVE;
		}
		gdk_drag_status(context, preferredAction, dragtime);
	} catch (...) {
		errorStatus = Status::Failure;
	}
	return FALSE;
}

gboolean ScintillaGTK::DragMotion(GtkWidget *widget, GdkDragContext *context,
				  gint x, gint y, guint dragtime) {
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->DragMotionThis(context, x, y, dragtime);
}

void ScintillaGTK::DragLeave(GtkWidget *widget, GdkDragContext * /*context*/, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		sciThis->SetDragPosition(SelectionPosition(Sci::invalidPosition));
		//Platform::DebugPrintf("DragLeave %x\n", sciThis);
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
}

void ScintillaGTK::DragEnd(GtkWidget *widget, GdkDragContext * /*context*/) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		// If drag did not result in drop here or elsewhere
		if (!sciThis->dragWasDropped)
			sciThis->SetEmptySelection(sciThis->posDrag);
		sciThis->SetDragPosition(SelectionPosition(Sci::invalidPosition));
		//Platform::DebugPrintf("DragEnd %x %d\n", sciThis, sciThis->dragWasDropped);
		sciThis->inDragDrop = DragDrop::none;
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
}

gboolean ScintillaGTK::Drop(GtkWidget *widget, GdkDragContext * /*context*/,
			    gint, gint, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		//Platform::DebugPrintf("Drop %x\n", sciThis);
		sciThis->SetDragPosition(SelectionPosition(Sci::invalidPosition));
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
	return FALSE;
}

void ScintillaGTK::DragDataReceived(GtkWidget *widget, GdkDragContext * /*context*/,
				    gint, gint, GtkSelectionData *selection_data, guint /*info*/, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		sciThis->ReceivedDrop(selection_data);
		sciThis->SetDragPosition(SelectionPosition(Sci::invalidPosition));
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
}

void ScintillaGTK::DragDataGet(GtkWidget *widget, GdkDragContext *context,
			       GtkSelectionData *selection_data, guint info, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		sciThis->dragWasDropped = true;
		if (!sciThis->sel.Empty()) {
			sciThis->GetSelection(selection_data, info, &sciThis->drag);
		}
		const GdkDragAction action = gdk_drag_context_get_selected_action(context);
		if (action == GDK_ACTION_MOVE) {
			for (size_t r=0; r<sciThis->sel.Count(); r++) {
				if (sciThis->posDrop >= sciThis->sel.Range(r).Start()) {
					if (sciThis->posDrop > sciThis->sel.Range(r).End()) {
						sciThis->posDrop.Add(-sciThis->sel.Range(r).Length());
					} else {
						sciThis->posDrop.Add(-SelectionRange(sciThis->posDrop, sciThis->sel.Range(r).Start()).Length());
					}
				}
			}
			sciThis->ClearSelection();
		}
		sciThis->SetDragPosition(SelectionPosition(Sci::invalidPosition));
	} catch (...) {
		sciThis->errorStatus = Status::Failure;
	}
}

int ScintillaGTK::TimeOut(gpointer ptt) {
	TimeThunk *tt = static_cast<TimeThunk *>(ptt);
	tt->scintilla->TickFor(tt->reason);
	return 1;
}

gboolean ScintillaGTK::IdleCallback(gpointer pSci) {
	ScintillaGTK *sciThis = static_cast<ScintillaGTK *>(pSci);
	// Idler will be automatically stopped, if there is nothing
	// to do while idle.
	const bool ret = sciThis->Idle();
	if (!ret) {
		// FIXME: This will remove the idler from GTK, we don't want to
		// remove it as it is removed automatically when this function
		// returns false (although, it should be harmless).
		sciThis->SetIdle(false);
	}
	return ret;
}

gboolean ScintillaGTK::StyleIdle(gpointer pSci) {
	ScintillaGTK *sciThis = static_cast<ScintillaGTK *>(pSci);
	sciThis->IdleWork();
	// Idler will be automatically stopped
	return FALSE;
}

void ScintillaGTK::IdleWork() {
	Editor::IdleWork();
	styleIdleID = 0;
}

void ScintillaGTK::QueueIdleWork(WorkItems items, Sci::Position upTo) {
	Editor::QueueIdleWork(items, upTo);
	if (!styleIdleID) {
		// Only allow one style needed to be queued
		styleIdleID = gdk_threads_add_idle_full(G_PRIORITY_HIGH_IDLE, StyleIdle, this, nullptr);
	}
}

void ScintillaGTK::SetDocPointer(Document *document) {
	Document *oldDoc = nullptr;
	ScintillaGTKAccessible *sciAccessible = nullptr;
	if (accessible) {
		sciAccessible = ScintillaGTKAccessible::FromAccessible(accessible);
		if (sciAccessible && pdoc) {
			oldDoc = pdoc;
			oldDoc->AddRef();
		}
	}

	Editor::SetDocPointer(document);

	if (sciAccessible) {
		// the accessible needs have the old Document, but also the new one active
		sciAccessible->ChangeDocument(oldDoc, pdoc);
	}
	if (oldDoc) {
		oldDoc->Release();
	}
}

void ScintillaGTK::PopUpCB(GtkMenuItem *menuItem, ScintillaGTK *sciThis) {
	guint const action = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuItem), "CmdNum"));
	if (action) {
		sciThis->Command(action);
	}
}

gboolean ScintillaGTK::PressCT(GtkWidget *widget, GdkEventButton *event, ScintillaGTK *sciThis) {
	try {
		if (event->window != WindowFromWidget(widget))
			return FALSE;
		if (event->type != GDK_BUTTON_PRESS)
			return FALSE;
		const Point pt = PointOfEvent(event);
		sciThis->ct.MouseClick(pt);
		sciThis->CallTipClick();
	} catch (...) {
	}
	return TRUE;
}

#if GTK_CHECK_VERSION(3,0,0)

gboolean ScintillaGTK::DrawCT(GtkWidget *widget, cairo_t *cr, CallTip *ctip) {
	try {
		std::unique_ptr<Surface> surfaceWindow(Surface::Allocate(Technology::Default));
		surfaceWindow->Init(cr, widget);
		surfaceWindow->SetMode(SurfaceMode(ctip->codePage, false));
		ctip->PaintCT(surfaceWindow.get());
		surfaceWindow->Release();
	} catch (...) {
		// No pointer back to Scintilla to save status
	}
	return TRUE;
}

#else

gboolean ScintillaGTK::ExposeCT(GtkWidget *widget, GdkEventExpose * /*ose*/, CallTip *ctip) {
	try {
		std::unique_ptr<Surface> surfaceWindow(Surface::Allocate(Technology::Default));
		UniqueCairo cr(gdk_cairo_create(WindowFromWidget(widget)));
		surfaceWindow->Init(cr.get(), widget);
		surfaceWindow->SetMode(SurfaceMode(ctip->codePage, false));
		ctip->PaintCT(surfaceWindow.get());
	} catch (...) {
		// No pointer back to Scintilla to save status
	}
	return TRUE;
}

#endif

AtkObject *ScintillaGTK::GetAccessibleThis(GtkWidget *widget) {
	return ScintillaGTKAccessible::WidgetGetAccessibleImpl(widget, &accessible, scintilla_class_parent_class);
}

AtkObject *ScintillaGTK::GetAccessible(GtkWidget *widget) {
	return FromWidget(widget)->GetAccessibleThis(widget);
}

sptr_t ScintillaGTK::DirectFunction(
	sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	ScintillaGTK *sci = reinterpret_cast<ScintillaGTK *>(ptr);
	return sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
}

sptr_t ScintillaGTK::DirectStatusFunction(
	sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam, int *pStatus) {
	ScintillaGTK *sci = reinterpret_cast<ScintillaGTK *>(ptr);
	const sptr_t returnValue = sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
	*pStatus = static_cast<int>(sci->errorStatus);
	return returnValue;
}

/* legacy name for scintilla_object_send_message */
sptr_t scintilla_send_message(ScintillaObject *sci, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	ScintillaGTK *psci = static_cast<ScintillaGTK *>(sci->pscin);
	return psci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
}

gintptr scintilla_object_send_message(ScintillaObject *sci, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	return scintilla_send_message(sci, iMessage, wParam, lParam);
}

static void scintilla_class_init(ScintillaClass *klass);
static void scintilla_init(ScintillaObject *sci);

/* legacy name for scintilla_object_get_type */
GType scintilla_get_type() {
	static GType scintilla_type = 0;
	try {

		if (!scintilla_type) {
			scintilla_type = g_type_from_name("ScintillaObject");
			if (!scintilla_type) {
				static GTypeInfo scintilla_info = {
					(guint16) sizeof(ScintillaObjectClass),
					nullptr, //(GBaseInitFunc)
					nullptr, //(GBaseFinalizeFunc)
					(GClassInitFunc) scintilla_class_init,
					nullptr, //(GClassFinalizeFunc)
					nullptr, //gconstpointer data
					(guint16) sizeof(ScintillaObject),
					0, //n_preallocs
					(GInstanceInitFunc) scintilla_init,
					nullptr //(GTypeValueTable*)
				};
				scintilla_type = g_type_register_static(
							 GTK_TYPE_CONTAINER, "ScintillaObject", &scintilla_info, (GTypeFlags) 0);
			}
		}

	} catch (...) {
	}
	return scintilla_type;
}

GType scintilla_object_get_type() {
	return scintilla_get_type();
}

void ScintillaGTK::ClassInit(OBJECT_CLASS *object_class, GtkWidgetClass *widget_class, GtkContainerClass *container_class) {
	Platform_Initialise();
	atomUTF8 = gdk_atom_intern("UTF8_STRING", FALSE);
	atomUTF8Mime = gdk_atom_intern("text/plain;charset=utf-8", FALSE);
	atomString = GDK_SELECTION_TYPE_STRING;
	atomUriList = gdk_atom_intern("text/uri-list", FALSE);
	atomDROPFILES_DND = gdk_atom_intern("DROPFILES_DND", FALSE);

	// Define default signal handlers for the class:  Could move more
	// of the signal handlers here (those that currently attached to wDraw
	// in Init() may require coordinate translation?)

	object_class->dispose = Dispose;
	object_class->finalize = Destroy;
#if GTK_CHECK_VERSION(3,0,0)
	widget_class->get_preferred_width = GetPreferredWidth;
	widget_class->get_preferred_height = GetPreferredHeight;
#else
	widget_class->size_request = SizeRequest;
#endif
	widget_class->size_allocate = SizeAllocate;
#if GTK_CHECK_VERSION(3,0,0)
	widget_class->draw = DrawMain;
#else
	widget_class->expose_event = ExposeMain;
#endif
	widget_class->motion_notify_event = Motion;
	widget_class->button_press_event = Press;
	widget_class->button_release_event = MouseRelease;
	widget_class->scroll_event = ScrollEvent;
	widget_class->key_press_event = KeyPress;
	widget_class->key_release_event = KeyRelease;
	widget_class->focus_in_event = FocusIn;
	widget_class->focus_out_event = FocusOut;
	widget_class->selection_received = SelectionReceived;
	widget_class->selection_get = SelectionGet;
	widget_class->selection_clear_event = SelectionClear;

	widget_class->drag_data_received = DragDataReceived;
	widget_class->drag_motion = DragMotion;
	widget_class->drag_leave = DragLeave;
	widget_class->drag_end = DragEnd;
	widget_class->drag_drop = Drop;
	widget_class->drag_data_get = DragDataGet;

	widget_class->realize = Realize;
	widget_class->unrealize = UnRealize;
	widget_class->map = Map;
	widget_class->unmap = UnMap;

	widget_class->get_accessible = GetAccessible;

	container_class->forall = MainForAll;
}

static void scintilla_class_init(ScintillaClass *klass) {
	try {
		OBJECT_CLASS *object_class = reinterpret_cast<OBJECT_CLASS *>(klass);
		GtkWidgetClass *widget_class = reinterpret_cast<GtkWidgetClass *>(klass);
		GtkContainerClass *container_class = reinterpret_cast<GtkContainerClass *>(klass);

		const GSignalFlags sigflags = static_cast<GSignalFlags>(G_SIGNAL_ACTION | G_SIGNAL_RUN_LAST);
		scintilla_signals[COMMAND_SIGNAL] = g_signal_new(
				"command",
				G_TYPE_FROM_CLASS(object_class),
				sigflags,
				G_STRUCT_OFFSET(ScintillaClass, command),
				nullptr, //(GSignalAccumulator)
				nullptr, //(gpointer)
				scintilla_marshal_VOID__INT_OBJECT,
				G_TYPE_NONE,
				2, G_TYPE_INT, GTK_TYPE_WIDGET);

		scintilla_signals[NOTIFY_SIGNAL] = g_signal_new(
				SCINTILLA_NOTIFY,
				G_TYPE_FROM_CLASS(object_class),
				sigflags,
				G_STRUCT_OFFSET(ScintillaClass, notify),
				nullptr, //(GSignalAccumulator)
				nullptr, //(gpointer)
				scintilla_marshal_VOID__INT_BOXED,
				G_TYPE_NONE,
				2, G_TYPE_INT, SCINTILLA_TYPE_NOTIFICATION);

		klass->command = nullptr;
		klass->notify = nullptr;
		scintilla_class_parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
		ScintillaGTK::ClassInit(object_class, widget_class, container_class);
	} catch (...) {
	}
}

static void scintilla_init(ScintillaObject *sci) {
	try {
		gtk_widget_set_can_focus(GTK_WIDGET(sci), TRUE);
		sci->pscin = new ScintillaGTK(sci);
	} catch (...) {
	}
}

/* legacy name for scintilla_object_new */
GtkWidget *scintilla_new() {
	GtkWidget *widget = GTK_WIDGET(g_object_new(scintilla_get_type(), nullptr));
	gtk_widget_set_direction(widget, GTK_TEXT_DIR_LTR);

	return widget;
}

GtkWidget *scintilla_object_new() {
	return scintilla_new();
}

void scintilla_set_id(ScintillaObject *sci, uptr_t id) {
	ScintillaGTK *psci = static_cast<ScintillaGTK *>(sci->pscin);
	psci->ctrlID = static_cast<int>(id);
}

void scintilla_release_resources(void) {
	try {
		Platform_Finalise();
	} catch (...) {
	}
}

/* Define a dummy boxed type because g-ir-scanner is unable to
 * recognize gpointer-derived types. Note that SCNotificaiton
 * is always allocated on stack so copying is not appropriate. */
static void *copy_(void *src) { return src; }
static void free_(void *) { }

GType scnotification_get_type(void) {
	static gsize type_id = 0;
	if (g_once_init_enter(&type_id)) {
		const gsize id = (gsize) g_boxed_type_register_static(
					 g_intern_static_string("SCNotification"),
					 (GBoxedCopyFunc) copy_,
					 (GBoxedFreeFunc) free_);
		g_once_init_leave(&type_id, id);
	}
	return (GType) type_id;
}
