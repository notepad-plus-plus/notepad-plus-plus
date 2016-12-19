// Scintilla source code edit control
// ScintillaGTK.cxx - GTK+ specific subclass of ScintillaBase
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>

#include <stdexcept>
#include <new>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include <glib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#if defined(__WIN32__) || defined(_MSC_VER)
#include <windows.h>
#endif

#include "Platform.h"

#include "ILexer.h"
#include "Scintilla.h"
#include "ScintillaWidget.h"
#ifdef SCI_LEXER
#include "SciLexer.h"
#endif
#include "StringCopy.h"
#ifdef SCI_LEXER
#include "LexerModule.h"
#endif
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "CaseConvert.h"
#include "UniConversion.h"
#include "UnicodeFromUTF8.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"
#include "AutoComplete.h"
#include "ScintillaBase.h"

#ifdef SCI_LEXER
#include "ExternalLexer.h"
#endif

#include "scintilla-marshal.h"

#include "Converter.h"

#if defined(__clang__)
// Clang 3.0 incorrectly displays  sentinel warnings. Fixed by clang 3.1.
#pragma GCC diagnostic ignored "-Wsentinel"
#endif

#if GTK_CHECK_VERSION(2,20,0)
#define IS_WIDGET_REALIZED(w) (gtk_widget_get_realized(GTK_WIDGET(w)))
#define IS_WIDGET_MAPPED(w) (gtk_widget_get_mapped(GTK_WIDGET(w)))
#define IS_WIDGET_VISIBLE(w) (gtk_widget_get_visible(GTK_WIDGET(w)))
#else
#define IS_WIDGET_REALIZED(w) (GTK_WIDGET_REALIZED(w))
#define IS_WIDGET_MAPPED(w) (GTK_WIDGET_MAPPED(w))
#define IS_WIDGET_VISIBLE(w) (GTK_WIDGET_VISIBLE(w))
#endif

#define SC_INDICATOR_INPUT INDIC_IME
#define SC_INDICATOR_TARGET INDIC_IME+1
#define SC_INDICATOR_CONVERTED INDIC_IME+2
#define SC_INDICATOR_UNKNOWN INDIC_IME_MAX

static GdkWindow *WindowFromWidget(GtkWidget *w) {
#if GTK_CHECK_VERSION(3,0,0)
	return gtk_widget_get_window(w);
#else
	return w->window;
#endif
}

#ifdef _MSC_VER
// Constant conditional expressions are because of GTK+ headers
#pragma warning(disable: 4127)
// Ignore unreferenced local functions in GTK+ headers
#pragma warning(disable: 4505)
#endif

#define OBJECT_CLASS GObjectClass

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

static GdkWindow *PWindow(const Window &w) {
	GtkWidget *widget = reinterpret_cast<GtkWidget *>(w.GetID());
#if GTK_CHECK_VERSION(3,0,0)
	return gtk_widget_get_window(widget);
#else
	return widget->window;
#endif
}

extern std::string UTF8FromLatin1(const char *s, int len);

class ScintillaGTK : public ScintillaBase {
	_ScintillaObject *sci;
	Window wText;
	Window scrollbarv;
	Window scrollbarh;
	GtkAdjustment *adjustmentv;
	GtkAdjustment *adjustmenth;
	int verticalScrollBarWidth;
	int horizontalScrollBarHeight;

	SelectionText primary;

	GdkEventButton *evbtn;
	bool capturedMouse;
	bool dragWasDropped;
	int lastKey;
	int rectangularSelectionModifier;

	GtkWidgetClass *parentClass;

	static GdkAtom atomClipboard;
	static GdkAtom atomUTF8;
	static GdkAtom atomString;
	static GdkAtom atomUriList;
	static GdkAtom atomDROPFILES_DND;
	GdkAtom atomSought;

#if PLAT_GTK_WIN32
	CLIPFORMAT cfColumnSelect;
#endif

	Window wPreedit;
	Window wPreeditDraw;
	GtkIMContext *im_context;
	PangoScript lastNonCommonScript;

	// Wheel mouse support
	unsigned int linesPerScroll;
	GTimeVal lastWheelMouseTime;
	gint lastWheelMouseDirection;
	gint wheelMouseIntensity;

#if GTK_CHECK_VERSION(3,0,0)
	cairo_rectangle_list_t *rgnUpdate;
#else
	GdkRegion *rgnUpdate;
#endif
	bool repaintFullWindow;

	// Private so ScintillaGTK objects can not be copied
	ScintillaGTK(const ScintillaGTK &);
	ScintillaGTK &operator=(const ScintillaGTK &);

public:
	explicit ScintillaGTK(_ScintillaObject *sci_);
	virtual ~ScintillaGTK();
	static void ClassInit(OBJECT_CLASS* object_class, GtkWidgetClass *widget_class, GtkContainerClass *container_class);
private:
	virtual void Initialise();
	virtual void Finalise();
	virtual bool AbandonPaint();
	virtual void DisplayCursor(Window::Cursor c);
	virtual bool DragThreshold(Point ptStart, Point ptNow);
	virtual void StartDrag();
	int TargetAsUTF8(char *text);
	int EncodedFromUTF8(char *utf8, char *encoded) const;
	virtual bool ValidCodePage(int codePage) const;
public: 	// Public for scintilla_send_message
	virtual sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
private:
	virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	struct TimeThunk {
		TickReason reason;
		ScintillaGTK *scintilla;
		guint timer;
		TimeThunk() : reason(tickCaret), scintilla(NULL), timer(0) {}
	};
	TimeThunk timers[tickDwell+1];
	virtual bool FineTickerAvailable();
	virtual bool FineTickerRunning(TickReason reason);
	virtual void FineTickerStart(TickReason reason, int millis, int tolerance);
	virtual void FineTickerCancel(TickReason reason);
	virtual bool SetIdle(bool on);
	virtual void SetMouseCapture(bool on);
	virtual bool HaveMouseCapture();
	virtual bool PaintContains(PRectangle rc);
	void FullPaint();
	virtual PRectangle GetClientRectangle() const;
	virtual void ScrollText(int linesToMove);
	virtual void SetVerticalScrollPos();
	virtual void SetHorizontalScrollPos();
	virtual bool ModifyScrollBars(int nMax, int nPage);
	void ReconfigureScrollBars();
	virtual void NotifyChange();
	virtual void NotifyFocus(bool focus);
	virtual void NotifyParent(SCNotification scn);
	void NotifyKey(int key, int modifiers);
	void NotifyURIDropped(const char *list);
	const char *CharacterSetID() const;
	virtual CaseFolder *CaseFolderForEncoding();
	virtual std::string CaseMapString(const std::string &s, int caseMapping);
	virtual int KeyDefault(int key, int modifiers);
	virtual void CopyToClipboard(const SelectionText &selectedText);
	virtual void Copy();
	virtual void Paste();
	virtual void CreateCallTipWindow(PRectangle rc);
	virtual void AddToPopUp(const char *label, int cmd = 0, bool enabled = true);
	bool OwnPrimarySelection();
	virtual void ClaimSelection();
	void GetGtkSelectionText(GtkSelectionData *selectionData, SelectionText &selText);
	void ReceivedSelection(GtkSelectionData *selection_data);
	void ReceivedDrop(GtkSelectionData *selection_data);
	static void GetSelection(GtkSelectionData *selection_data, guint info, SelectionText *selected);
	void StoreOnClipboard(SelectionText *clipText);
	static void ClipboardGetSelection(GtkClipboard* clip, GtkSelectionData *selection_data, guint info, void *data);
	static void ClipboardClearSelection(GtkClipboard* clip, void *data);

	void UnclaimSelection(GdkEventSelection *selection_event);
	void Resize(int width, int height);

	// Callback functions
	void RealizeThis(GtkWidget *widget);
	static void Realize(GtkWidget *widget);
	void UnRealizeThis(GtkWidget *widget);
	static void UnRealize(GtkWidget *widget);
	void MapThis();
	static void Map(GtkWidget *widget);
	void UnMapThis();
	static void UnMap(GtkWidget *widget);
	gint FocusInThis(GtkWidget *widget);
	static gint FocusIn(GtkWidget *widget, GdkEventFocus *event);
	gint FocusOutThis(GtkWidget *widget);
	static gint FocusOut(GtkWidget *widget, GdkEventFocus *event);
	static void SizeRequest(GtkWidget *widget, GtkRequisition *requisition);
#if GTK_CHECK_VERSION(3,0,0)
	static void GetPreferredWidth(GtkWidget *widget, gint *minimalWidth, gint *naturalWidth);
	static void GetPreferredHeight(GtkWidget *widget, gint *minimalHeight, gint *naturalHeight);
#endif
	static void SizeAllocate(GtkWidget *widget, GtkAllocation *allocation);
#if GTK_CHECK_VERSION(3,0,0)
	gboolean DrawTextThis(cairo_t *cr);
	static gboolean DrawText(GtkWidget *widget, cairo_t *cr, ScintillaGTK *sciThis);
	gboolean DrawThis(cairo_t *cr);
	static gboolean DrawMain(GtkWidget *widget, cairo_t *cr);
#else
	gboolean ExposeTextThis(GtkWidget *widget, GdkEventExpose *ose);
	static gboolean ExposeText(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis);
	gboolean Expose(GtkWidget *widget, GdkEventExpose *ose);
	static gboolean ExposeMain(GtkWidget *widget, GdkEventExpose *ose);
#endif
	static void Draw(GtkWidget *widget, GdkRectangle *area);
	void ForAll(GtkCallback callback, gpointer callback_data);
	static void MainForAll(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data);

	static void ScrollSignal(GtkAdjustment *adj, ScintillaGTK *sciThis);
	static void ScrollHSignal(GtkAdjustment *adj, ScintillaGTK *sciThis);
	gint PressThis(GdkEventButton *event);
	static gint Press(GtkWidget *widget, GdkEventButton *event);
	static gint MouseRelease(GtkWidget *widget, GdkEventButton *event);
	static gint ScrollEvent(GtkWidget *widget, GdkEventScroll *event);
	static gint Motion(GtkWidget *widget, GdkEventMotion *event);
	gboolean KeyThis(GdkEventKey *event);
	static gboolean KeyPress(GtkWidget *widget, GdkEventKey *event);
	static gboolean KeyRelease(GtkWidget *widget, GdkEventKey *event);
#if GTK_CHECK_VERSION(3,0,0)
	gboolean DrawPreeditThis(GtkWidget *widget, cairo_t *cr);
	static gboolean DrawPreedit(GtkWidget *widget, cairo_t *cr, ScintillaGTK *sciThis);
#else
	gboolean ExposePreeditThis(GtkWidget *widget, GdkEventExpose *ose);
	static gboolean ExposePreedit(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis);
#endif

	bool KoreanIME();
	void CommitThis(char *str);
	static void Commit(GtkIMContext *context, char *str, ScintillaGTK *sciThis);
	void PreeditChangedInlineThis();
	void PreeditChangedWindowedThis();
	static void PreeditChanged(GtkIMContext *context, ScintillaGTK *sciThis);
	void MoveImeCarets(int pos);
	void DrawImeIndicator(int indicator, int len);
	static void GetImeUnderlines(PangoAttrList *attrs, bool *normalInput);
	static void GetImeBackgrounds(PangoAttrList *attrs, bool *targetInput);
	void SetCandidateWindowPos();

	static void StyleSetText(GtkWidget *widget, GtkStyle *previous, void*);
	static void RealizeText(GtkWidget *widget, void*);
	static void Destroy(GObject *object);
	static void SelectionReceived(GtkWidget *widget, GtkSelectionData *selection_data,
	                              guint time);
	static void ClipboardReceived(GtkClipboard *clipboard, GtkSelectionData *selection_data, 
	                              gpointer data);
	static void SelectionGet(GtkWidget *widget, GtkSelectionData *selection_data,
	                         guint info, guint time);
	static gint SelectionClear(GtkWidget *widget, GdkEventSelection *selection_event);
	gboolean DragMotionThis(GdkDragContext *context, gint x, gint y, guint dragtime);
	static gboolean DragMotion(GtkWidget *widget, GdkDragContext *context,
	                           gint x, gint y, guint dragtime);
	static void DragLeave(GtkWidget *widget, GdkDragContext *context,
	                      guint time);
	static void DragEnd(GtkWidget *widget, GdkDragContext *context);
	static gboolean Drop(GtkWidget *widget, GdkDragContext *context,
	                     gint x, gint y, guint time);
	static void DragDataReceived(GtkWidget *widget, GdkDragContext *context,
	                             gint x, gint y, GtkSelectionData *selection_data, guint info, guint time);
	static void DragDataGet(GtkWidget *widget, GdkDragContext *context,
	                        GtkSelectionData *selection_data, guint info, guint time);
	static gboolean TimeOut(TimeThunk *tt);
	static gboolean IdleCallback(ScintillaGTK *sciThis);
	static gboolean StyleIdle(ScintillaGTK *sciThis);
	virtual void QueueIdleWork(WorkNeeded::workItems items, int upTo);
	static void PopUpCB(GtkMenuItem *menuItem, ScintillaGTK *sciThis);

#if GTK_CHECK_VERSION(3,0,0)
	static gboolean DrawCT(GtkWidget *widget, cairo_t *cr, CallTip *ctip);
#else
	static gboolean ExposeCT(GtkWidget *widget, GdkEventExpose *ose, CallTip *ct);
#endif
	static gboolean PressCT(GtkWidget *widget, GdkEventButton *event, ScintillaGTK *sciThis);

	static sptr_t DirectFunction(sptr_t ptr,
	                             unsigned int iMessage, uptr_t wParam, sptr_t lParam);
};

enum {
    COMMAND_SIGNAL,
    NOTIFY_SIGNAL,
    LAST_SIGNAL
};

static gint scintilla_signals[LAST_SIGNAL] = { 0 };

enum {
    TARGET_STRING,
    TARGET_TEXT,
    TARGET_COMPOUND_TEXT,
    TARGET_UTF8_STRING,
    TARGET_URI
};

GdkAtom ScintillaGTK::atomClipboard = 0;
GdkAtom ScintillaGTK::atomUTF8 = 0;
GdkAtom ScintillaGTK::atomString = 0;
GdkAtom ScintillaGTK::atomUriList = 0;
GdkAtom ScintillaGTK::atomDROPFILES_DND = 0;

static const GtkTargetEntry clipboardCopyTargets[] = {
	{ (gchar *) "UTF8_STRING", 0, TARGET_UTF8_STRING },
	{ (gchar *) "STRING", 0, TARGET_STRING },
};
static const gint nClipboardCopyTargets = ELEMENTS(clipboardCopyTargets);

static const GtkTargetEntry clipboardPasteTargets[] = {
	{ (gchar *) "text/uri-list", 0, TARGET_URI },
	{ (gchar *) "UTF8_STRING", 0, TARGET_UTF8_STRING },
	{ (gchar *) "STRING", 0, TARGET_STRING },
};
static const gint nClipboardPasteTargets = ELEMENTS(clipboardPasteTargets);

static GtkWidget *PWidget(Window &w) {
	return reinterpret_cast<GtkWidget *>(w.GetID());
}

static ScintillaGTK *ScintillaFromWidget(GtkWidget *widget) {
	ScintillaObject *scio = reinterpret_cast<ScintillaObject *>(widget);
	return reinterpret_cast<ScintillaGTK *>(scio->pscin);
}

ScintillaGTK::ScintillaGTK(_ScintillaObject *sci_) :
		adjustmentv(0), adjustmenth(0),
		verticalScrollBarWidth(30), horizontalScrollBarHeight(30),
		evbtn(0), capturedMouse(false), dragWasDropped(false),
		lastKey(0), rectangularSelectionModifier(SCMOD_CTRL), parentClass(0),
		im_context(NULL), lastNonCommonScript(PANGO_SCRIPT_INVALID_CODE),
		lastWheelMouseDirection(0),
		wheelMouseIntensity(0),
		rgnUpdate(0),
		repaintFullWindow(false) {
	sci = sci_;
	wMain = GTK_WIDGET(sci);

#if PLAT_GTK_WIN32
	rectangularSelectionModifier = SCMOD_ALT;
#else
	rectangularSelectionModifier = SCMOD_CTRL;
#endif

#if PLAT_GTK_WIN32
	// There does not seem to be a real standard for indicating that the clipboard
	// contains a rectangular selection, so copy Developer Studio.
	cfColumnSelect = static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat("MSDEVColumnSelect"));

	// Get intellimouse parameters when running on win32; otherwise use
	// reasonable default
#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES   104
#endif
	::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &linesPerScroll, 0);
#else
	linesPerScroll = 4;
#endif
	lastWheelMouseTime.tv_sec = 0;
	lastWheelMouseTime.tv_usec = 0;

	Initialise();
}

ScintillaGTK::~ScintillaGTK() {
	g_idle_remove_by_data(this);
	if (evbtn) {
		gdk_event_free(reinterpret_cast<GdkEvent *>(evbtn));
		evbtn = 0;
	}
	wPreedit.Destroy();
}

static void UnRefCursor(GdkCursor *cursor) {
#if GTK_CHECK_VERSION(3,0,0)
	g_object_unref(cursor);
#else
	gdk_cursor_unref(cursor);
#endif
}

void ScintillaGTK::RealizeThis(GtkWidget *widget) {
	//Platform::DebugPrintf("ScintillaGTK::realize this\n");
#if GTK_CHECK_VERSION(2,20,0)
	gtk_widget_set_realized(widget, TRUE);
#else
	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
#endif
	GdkWindowAttr attrs;
	attrs.window_type = GDK_WINDOW_CHILD;
	GtkAllocation allocation;
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_get_allocation(widget, &allocation);
#else
	allocation = widget->allocation;
#endif
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
	GdkCursor *cursor = gdk_cursor_new(GDK_XTERM);
	attrs.cursor = cursor;
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_set_window(widget, gdk_window_new(gtk_widget_get_parent_window(widget), &attrs,
		GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_CURSOR));
	gdk_window_set_user_data(gtk_widget_get_window(widget), widget);
	GtkStyleContext *styleContext = gtk_widget_get_style_context(widget);
	if (styleContext) {
		GdkRGBA colourBackWidget;
		gtk_style_context_get_background_color(styleContext, GTK_STATE_FLAG_NORMAL, &colourBackWidget);
		gdk_window_set_background_rgba(gtk_widget_get_window(widget), &colourBackWidget);
	}
	gdk_window_show(gtk_widget_get_window(widget));
	UnRefCursor(cursor);
#else
	widget->window = gdk_window_new(gtk_widget_get_parent_window(widget), &attrs,
		GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP | GDK_WA_CURSOR);
	gdk_window_set_user_data(widget->window, widget);
	widget->style = gtk_style_attach(widget->style, widget->window);
	gdk_window_set_background(widget->window, &widget->style->bg[GTK_STATE_NORMAL]);
	gdk_window_show(widget->window);
	UnRefCursor(cursor);
#endif
	gtk_widget_realize(PWidget(wPreedit));
	gtk_widget_realize(PWidget(wPreeditDraw));

	im_context = gtk_im_multicontext_new();
	g_signal_connect(G_OBJECT(im_context), "commit",
		G_CALLBACK(Commit), this);
	g_signal_connect(G_OBJECT(im_context), "preedit_changed",
		G_CALLBACK(PreeditChanged), this);
	gtk_im_context_set_client_window(im_context, WindowFromWidget(widget));
	GtkWidget *widtxt = PWidget(wText);	//	// No code inside the G_OBJECT macro
	g_signal_connect_after(G_OBJECT(widtxt), "style_set",
		G_CALLBACK(ScintillaGTK::StyleSetText), NULL);
	g_signal_connect_after(G_OBJECT(widtxt), "realize",
		G_CALLBACK(ScintillaGTK::RealizeText), NULL);
	gtk_widget_realize(widtxt);
	gtk_widget_realize(PWidget(scrollbarv));
	gtk_widget_realize(PWidget(scrollbarh));

	cursor = gdk_cursor_new(GDK_XTERM);
	gdk_window_set_cursor(PWindow(wText), cursor);
	UnRefCursor(cursor);

	cursor = gdk_cursor_new(GDK_LEFT_PTR);
	gdk_window_set_cursor(PWindow(scrollbarv), cursor);
	UnRefCursor(cursor);

	cursor = gdk_cursor_new(GDK_LEFT_PTR);
	gdk_window_set_cursor(PWindow(scrollbarh), cursor);
	UnRefCursor(cursor);

	gtk_selection_add_targets(widget, GDK_SELECTION_PRIMARY,
	                          clipboardCopyTargets, nClipboardCopyTargets);
}

void ScintillaGTK::Realize(GtkWidget *widget) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	sciThis->RealizeThis(widget);
}

void ScintillaGTK::UnRealizeThis(GtkWidget *widget) {
	try {
		gtk_selection_clear_targets(widget, GDK_SELECTION_PRIMARY);

		if (IS_WIDGET_MAPPED(widget)) {
			gtk_widget_unmap(widget);
		}
#if GTK_CHECK_VERSION(2,20,0)
		gtk_widget_set_realized(widget, FALSE);
#else
		GTK_WIDGET_UNSET_FLAGS(widget, GTK_REALIZED);
#endif
		gtk_widget_unrealize(PWidget(wText));
		gtk_widget_unrealize(PWidget(scrollbarv));
		gtk_widget_unrealize(PWidget(scrollbarh));
		gtk_widget_unrealize(PWidget(wPreedit));
		gtk_widget_unrealize(PWidget(wPreeditDraw));
		g_object_unref(im_context);
		im_context = NULL;
		if (GTK_WIDGET_CLASS(parentClass)->unrealize)
			GTK_WIDGET_CLASS(parentClass)->unrealize(widget);

		Finalise();
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::UnRealize(GtkWidget *widget) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	sciThis->UnRealizeThis(widget);
}

static void MapWidget(GtkWidget *widget) {
	if (widget &&
	        IS_WIDGET_VISIBLE(widget) &&
	        !IS_WIDGET_MAPPED(widget)) {
		gtk_widget_map(widget);
	}
}

void ScintillaGTK::MapThis() {
	try {
		//Platform::DebugPrintf("ScintillaGTK::map this\n");
#if GTK_CHECK_VERSION(2,20,0)
		gtk_widget_set_mapped(PWidget(wMain), TRUE);
#else
		GTK_WIDGET_SET_FLAGS(PWidget(wMain), GTK_MAPPED);
#endif
		MapWidget(PWidget(wText));
		MapWidget(PWidget(scrollbarh));
		MapWidget(PWidget(scrollbarv));
		wMain.SetCursor(Window::cursorArrow);
		scrollbarv.SetCursor(Window::cursorArrow);
		scrollbarh.SetCursor(Window::cursorArrow);
		ChangeSize();
		gdk_window_show(PWindow(wMain));
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::Map(GtkWidget *widget) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	sciThis->MapThis();
}

void ScintillaGTK::UnMapThis() {
	try {
		//Platform::DebugPrintf("ScintillaGTK::unmap this\n");
#if GTK_CHECK_VERSION(2,20,0)
		gtk_widget_set_mapped(PWidget(wMain), FALSE);
#else
		GTK_WIDGET_UNSET_FLAGS(PWidget(wMain), GTK_MAPPED);
#endif
		DropGraphics(false);
		gdk_window_hide(PWindow(wMain));
		gtk_widget_unmap(PWidget(wText));
		gtk_widget_unmap(PWidget(scrollbarh));
		gtk_widget_unmap(PWidget(scrollbarv));
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::UnMap(GtkWidget *widget) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	sciThis->UnMapThis();
}

void ScintillaGTK::ForAll(GtkCallback callback, gpointer callback_data) {
	try {
		(*callback) (PWidget(wText), callback_data);
		(*callback) (PWidget(scrollbarv), callback_data);
		(*callback) (PWidget(scrollbarh), callback_data);
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::MainForAll(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data) {
	ScintillaGTK *sciThis = ScintillaFromWidget((GtkWidget *)container);

	if (callback != NULL && include_internals) {
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
	PangoScript pscript;

	explicit PreEditString(GtkIMContext *im_context) {
		gtk_im_context_get_preedit_string(im_context, &str, &attrs, &cursor_pos);
		validUTF8 = g_utf8_validate(str, strlen(str), NULL);
		uniStr = g_utf8_to_ucs4_fast(str, strlen(str), &uniStrLen);
		pscript = pango_script_for_unichar(uniStr[0]);
	}
	~PreEditString() {
		g_free(str);
		g_free(uniStr);
		pango_attr_list_unref(attrs);
	}
};

}

gint ScintillaGTK::FocusInThis(GtkWidget *widget) {
	try {
		SetFocusState(true);
		if (im_context != NULL) {
			PreEditString pes(im_context);
			if (PWidget(wPreedit) != NULL) {
				if (strlen(pes.str) > 0) {
					gtk_widget_show(PWidget(wPreedit));
				} else {
					gtk_widget_hide(PWidget(wPreedit));
				}
			}
			gtk_im_context_focus_in(im_context);
		}

	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gint ScintillaGTK::FocusIn(GtkWidget *widget, GdkEventFocus * /*event*/) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	return sciThis->FocusInThis(widget);
}

gint ScintillaGTK::FocusOutThis(GtkWidget *widget) {
	try {
		SetFocusState(false);

		if (PWidget(wPreedit) != NULL)
			gtk_widget_hide(PWidget(wPreedit));
		if (im_context != NULL)
			gtk_im_context_focus_out(im_context);

	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gint ScintillaGTK::FocusOut(GtkWidget *widget, GdkEventFocus * /*event*/) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	return sciThis->FocusOutThis(widget);
}

void ScintillaGTK::SizeRequest(GtkWidget *widget, GtkRequisition *requisition) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	requisition->width = 1;
	requisition->height = 1;
	GtkRequisition child_requisition;
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_get_preferred_size(PWidget(sciThis->scrollbarh), NULL, &child_requisition);
	gtk_widget_get_preferred_size(PWidget(sciThis->scrollbarv), NULL, &child_requisition);
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
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {
#if GTK_CHECK_VERSION(2,20,0)
		gtk_widget_set_allocation(widget, allocation);
#else
		widget->allocation = *allocation;
#endif
		if (IS_WIDGET_REALIZED(widget))
			gdk_window_move_resize(WindowFromWidget(widget),
			        allocation->x,
			        allocation->y,
			        allocation->width,
			        allocation->height);

		sciThis->Resize(allocation->width, allocation->height);

	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::Initialise() {
	//Platform::DebugPrintf("ScintillaGTK::Initialise\n");
	parentClass = reinterpret_cast<GtkWidgetClass *>(
	                  g_type_class_ref(gtk_container_get_type()));

#if GTK_CHECK_VERSION(2,20,0)
	gtk_widget_set_can_focus(PWidget(wMain), TRUE);
	gtk_widget_set_sensitive(PWidget(wMain), TRUE);
#else
	GTK_WIDGET_SET_FLAGS(PWidget(wMain), GTK_CAN_FOCUS);
	GTK_WIDGET_SET_FLAGS(GTK_WIDGET(PWidget(wMain)), GTK_SENSITIVE);
#endif
	gtk_widget_set_events(PWidget(wMain),
	                      GDK_EXPOSURE_MASK
	                      | GDK_SCROLL_MASK
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
	if (gtk_check_version(3,9,2) != NULL /* on < 3.9.2 */)
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
#if GTK_CHECK_VERSION(2,20,0)
	gtk_widget_set_can_focus(PWidget(scrollbarv), FALSE);
#else
	GTK_WIDGET_UNSET_FLAGS(PWidget(scrollbarv), GTK_CAN_FOCUS);
#endif
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
#if GTK_CHECK_VERSION(2,20,0)
	gtk_widget_set_can_focus(PWidget(scrollbarh), FALSE);
#else
	GTK_WIDGET_UNSET_FLAGS(PWidget(scrollbarh), GTK_CAN_FOCUS);
#endif
	g_signal_connect(G_OBJECT(adjustmenth), "value_changed",
			   G_CALLBACK(ScrollHSignal), this);
	gtk_widget_set_parent(PWidget(scrollbarh), PWidget(wMain));
	gtk_widget_show(PWidget(scrollbarh));

	gtk_widget_grab_focus(PWidget(wMain));

	gtk_drag_dest_set(GTK_WIDGET(PWidget(wMain)),
	                  GTK_DEST_DEFAULT_ALL, clipboardPasteTargets, nClipboardPasteTargets,
	                  static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE));

	/* create pre-edit window */
	wPreedit = gtk_window_new(GTK_WINDOW_POPUP);
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

	// Set caret period based on GTK settings
	gboolean blinkOn = false;
	if (g_object_class_find_property(G_OBJECT_GET_CLASS(
			G_OBJECT(gtk_settings_get_default())), "gtk-cursor-blink")) {
		g_object_get(G_OBJECT(
			gtk_settings_get_default()), "gtk-cursor-blink", &blinkOn, NULL);
	}
	if (blinkOn &&
		g_object_class_find_property(G_OBJECT_GET_CLASS(
			G_OBJECT(gtk_settings_get_default())), "gtk-cursor-blink-time")) {
		gint value;
		g_object_get(G_OBJECT(
			gtk_settings_get_default()), "gtk-cursor-blink-time", &value, NULL);
		caret.period = gint(value / 1.75);
	} else {
		caret.period = 0;
	}

	for (TickReason tr = tickCaret; tr <= tickDwell; tr = static_cast<TickReason>(tr + 1)) {
		timers[tr].reason = tr;
		timers[tr].scintilla = this;
	}
	vs.indicators[SC_INDICATOR_UNKNOWN] = Indicator(INDIC_HIDDEN, ColourDesired(0, 0, 0xff));
	vs.indicators[SC_INDICATOR_INPUT] = Indicator(INDIC_DOTS, ColourDesired(0, 0, 0xff));
	vs.indicators[SC_INDICATOR_CONVERTED] = Indicator(INDIC_COMPOSITIONTHICK, ColourDesired(0, 0, 0xff));
	vs.indicators[SC_INDICATOR_TARGET] = Indicator(INDIC_STRAIGHTBOX, ColourDesired(0, 0, 0xff));
}

void ScintillaGTK::Finalise() {
	for (TickReason tr = tickCaret; tr <= tickDwell; tr = static_cast<TickReason>(tr + 1)) {
		FineTickerCancel(tr);
	}
	ScintillaBase::Finalise();
}

bool ScintillaGTK::AbandonPaint() {
	if ((paintState == painting) && !paintingAllText) {
		repaintFullWindow = true;
	}
	return false;
}

void ScintillaGTK::DisplayCursor(Window::Cursor c) {
	if (cursorMode == SC_CURSORNORMAL)
		wText.SetCursor(c);
	else
		wText.SetCursor(static_cast<Window::Cursor>(cursorMode));
}

bool ScintillaGTK::DragThreshold(Point ptStart, Point ptNow) {
	return gtk_drag_check_threshold(GTK_WIDGET(PWidget(wMain)),
		ptStart.x, ptStart.y, ptNow.x, ptNow.y);
}

void ScintillaGTK::StartDrag() {
	PLATFORM_ASSERT(evbtn != 0);
	dragWasDropped = false;
	inDragDrop = ddDragging;
	GtkTargetList *tl = gtk_target_list_new(clipboardCopyTargets, nClipboardCopyTargets);
#if GTK_CHECK_VERSION(3,10,0)
	gtk_drag_begin_with_coordinates(GTK_WIDGET(PWidget(wMain)),
	               tl,
	               static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE),
	               evbtn->button,
	               reinterpret_cast<GdkEvent *>(evbtn),
			-1, -1);
#else
	gtk_drag_begin(GTK_WIDGET(PWidget(wMain)),
	               tl,
	               static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE),
	               evbtn->button,
	               reinterpret_cast<GdkEvent *>(evbtn));
#endif
}

static std::string ConvertText(const char *s, size_t len, const char *charSetDest,
	const char *charSetSource, bool transliterations, bool silent=false) {
	// s is not const because of different versions of iconv disagreeing about const
	std::string destForm;
	Converter conv(charSetDest, charSetSource, transliterations);
	if (conv) {
		size_t outLeft = len*3+1;
		destForm = std::string(outLeft, '\0');
		// g_iconv does not actually write to its input argument so safe to cast away const
		char *pin = const_cast<char *>(s);
		size_t inLeft = len;
		char *putf = &destForm[0];
		char *pout = putf;
		size_t conversions = conv.Convert(&pin, &inLeft, &pout, &outLeft);
		if (conversions == ((size_t)(-1))) {
			if (!silent) {
				if (len == 1)
					fprintf(stderr, "iconv %s->%s failed for %0x '%s'\n",
						charSetSource, charSetDest, (unsigned char)(*s), s);
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

// Returns the target converted to UTF8.
// Return the length in bytes.
int ScintillaGTK::TargetAsUTF8(char *text) {
	int targetLength = targetEnd - targetStart;
	if (IsUnicodeMode()) {
		if (text) {
			pdoc->GetCharRange(text, targetStart, targetLength);
		}
	} else {
		// Need to convert
		const char *charSetBuffer = CharacterSetID();
		if (*charSetBuffer) {
			std::string s = RangeText(targetStart, targetEnd);
			std::string tmputf = ConvertText(&s[0], targetLength, "UTF-8", charSetBuffer, false);
			if (text) {
				memcpy(text, tmputf.c_str(), tmputf.length());
			}
			return tmputf.length();
		} else {
			if (text) {
				pdoc->GetCharRange(text, targetStart, targetLength);
			}
		}
	}
	return targetLength;
}

// Translates a nul terminated UTF8 string into the document encoding.
// Return the length of the result in bytes.
int ScintillaGTK::EncodedFromUTF8(char *utf8, char *encoded) const {
	int inputLength = (lengthForEncode >= 0) ? lengthForEncode : strlen(utf8);
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

sptr_t ScintillaGTK::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	try {
		switch (iMessage) {

		case SCI_GRABFOCUS:
			gtk_widget_grab_focus(PWidget(wMain));
			break;

		case SCI_GETDIRECTFUNCTION:
			return reinterpret_cast<sptr_t>(DirectFunction);

		case SCI_GETDIRECTPOINTER:
			return reinterpret_cast<sptr_t>(this);

#ifdef SCI_LEXER
		case SCI_LOADLEXERLIBRARY:
			LexerManager::GetInstance()->Load(reinterpret_cast<const char*>(lParam));
			break;
#endif
		case SCI_TARGETASUTF8:
			return TargetAsUTF8(reinterpret_cast<char*>(lParam));

		case SCI_ENCODEDFROMUTF8:
			return EncodedFromUTF8(reinterpret_cast<char*>(wParam),
			        reinterpret_cast<char*>(lParam));

		case SCI_SETRECTANGULARSELECTIONMODIFIER:
			rectangularSelectionModifier = wParam;
			break;

		case SCI_GETRECTANGULARSELECTIONMODIFIER:
			return rectangularSelectionModifier;

		default:
			return ScintillaBase::WndProc(iMessage, wParam, lParam);
		}
	} catch (std::bad_alloc&) {
		errorStatus = SC_STATUS_BADALLOC;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return 0l;
}

sptr_t ScintillaGTK::DefWndProc(unsigned int, uptr_t, sptr_t) {
	return 0;
}

/**
* Report that this Editor subclass has a working implementation of FineTickerStart.
*/
bool ScintillaGTK::FineTickerAvailable() {
	return true;
}

bool ScintillaGTK::FineTickerRunning(TickReason reason) {
	return timers[reason].timer != 0;
}

void ScintillaGTK::FineTickerStart(TickReason reason, int millis, int /* tolerance */) {
	FineTickerCancel(reason);
	timers[reason].timer = g_timeout_add(millis, reinterpret_cast<GSourceFunc>(TimeOut), &timers[reason]);
}

void ScintillaGTK::FineTickerCancel(TickReason reason) {
	if (timers[reason].timer) {
		g_source_remove(timers[reason].timer);
		timers[reason].timer = 0;
	}
}

bool ScintillaGTK::SetIdle(bool on) {
	if (on) {
		// Start idler, if it's not running.
		if (!idler.state) {
			idler.state = true;
			idler.idlerID = reinterpret_cast<IdlerID>(
				g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
					reinterpret_cast<GSourceFunc>(IdleCallback), this, NULL));
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

// Is crcTest completely in crcContainer?
static bool CRectContains(const cairo_rectangle_t &crcContainer, const cairo_rectangle_t &crcTest) {
	return
		(crcTest.x >= crcContainer.x) && ((crcTest.x + crcTest.width) <= (crcContainer.x + crcContainer.width)) &&
		(crcTest.y >= crcContainer.y) && ((crcTest.y + crcTest.height) <= (crcContainer.y + crcContainer.height));
}

// Is crcTest completely in crcListContainer?
// May incorrectly return false if complex shape
static bool CRectListContains(const cairo_rectangle_list_t *crcListContainer, const cairo_rectangle_t &crcTest) {
	for (int r=0; r<crcListContainer->num_rectangles; r++) {
		if (CRectContains(crcListContainer->rectangles[r], crcTest))
			return true;
	}
	return false;
}

#endif

bool ScintillaGTK::PaintContains(PRectangle rc) {
	// This allows optimization when a rectangle is completely in the update region.
	// It is OK to return false when too difficult to determine as that just performs extra drawing
	bool contains = true;
	if (paintState == painting) {
		if (!rcPaint.Contains(rc)) {
			contains = false;
		} else if (rgnUpdate) {
#if GTK_CHECK_VERSION(3,0,0)
			cairo_rectangle_t grc = {rc.left, rc.top,
				rc.right - rc.left, rc.bottom - rc.top};
			contains = CRectListContains(rgnUpdate, grc);
#else
			GdkRectangle grc = {static_cast<gint>(rc.left), static_cast<gint>(rc.top),
				static_cast<gint>(rc.right - rc.left), static_cast<gint>(rc.bottom - rc.top)};
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

PRectangle ScintillaGTK::GetClientRectangle() const {
	Window &win = const_cast<Window &>(wMain);
	PRectangle rc = win.GetClientPosition();
	if (verticalScrollBarVisible)
		rc.right -= verticalScrollBarWidth;
	if (horizontalScrollBarVisible && !Wrapping())
		rc.bottom -= horizontalScrollBarHeight;
	// Move to origin
	rc.right -= rc.left;
	rc.bottom -= rc.top;
	rc.left = 0;
	rc.top = 0;
	return rc;
}

void ScintillaGTK::ScrollText(int linesToMove) {
	int diff = vs.lineHeight * -linesToMove;
	//Platform::DebugPrintf("ScintillaGTK::ScrollText %d %d %0d,%0d %0d,%0d\n", linesToMove, diff,
	//	rc.left, rc.top, rc.right, rc.bottom);
	GtkWidget *wi = PWidget(wText);
	NotifyUpdateUI();

	if (IS_WIDGET_REALIZED(wi)) {
		gdk_window_scroll(WindowFromWidget(wi), 0, -diff);
		gdk_window_process_updates(WindowFromWidget(wi), FALSE);
	}
}

void ScintillaGTK::SetVerticalScrollPos() {
	DwellEnd(true);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustmentv), topLine);
}

void ScintillaGTK::SetHorizontalScrollPos() {
	DwellEnd(true);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustmenth), xOffset);
}

bool ScintillaGTK::ModifyScrollBars(int nMax, int nPage) {
	bool modified = false;
	int pageScroll = LinesToScroll();

#if GTK_CHECK_VERSION(3,0,0)
	if (gtk_adjustment_get_upper(adjustmentv) != (nMax + 1) ||
	        gtk_adjustment_get_page_size(adjustmentv) != nPage ||
	        gtk_adjustment_get_page_increment(adjustmentv) != pageScroll) {
		gtk_adjustment_set_upper(adjustmentv, nMax + 1);
	        gtk_adjustment_set_page_size(adjustmentv, nPage);
	        gtk_adjustment_set_page_increment(adjustmentv, pageScroll);
		gtk_adjustment_changed(GTK_ADJUSTMENT(adjustmentv));
		modified = true;
	}
#else
	if (GTK_ADJUSTMENT(adjustmentv)->upper != (nMax + 1) ||
	        GTK_ADJUSTMENT(adjustmentv)->page_size != nPage ||
	        GTK_ADJUSTMENT(adjustmentv)->page_increment != pageScroll) {
		GTK_ADJUSTMENT(adjustmentv)->upper = nMax + 1;
		GTK_ADJUSTMENT(adjustmentv)->page_size = nPage;
		GTK_ADJUSTMENT(adjustmentv)->page_increment = pageScroll;
		gtk_adjustment_changed(GTK_ADJUSTMENT(adjustmentv));
		modified = true;
	}
#endif

	PRectangle rcText = GetTextRectangle();
	int horizEndPreferred = scrollWidth;
	if (horizEndPreferred < 0)
		horizEndPreferred = 0;
	unsigned int pageWidth = rcText.Width();
	unsigned int pageIncrement = pageWidth / 3;
	unsigned int charWidth = vs.styles[STYLE_DEFAULT].aveCharWidth;
#if GTK_CHECK_VERSION(3,0,0)
	if (gtk_adjustment_get_upper(adjustmenth) != horizEndPreferred ||
	        gtk_adjustment_get_page_size(adjustmenth) != pageWidth ||
	        gtk_adjustment_get_page_increment(adjustmenth) != pageIncrement ||
	        gtk_adjustment_get_step_increment(adjustmenth) != charWidth) {
		gtk_adjustment_set_upper(adjustmenth, horizEndPreferred);
	        gtk_adjustment_set_page_size(adjustmenth, pageWidth);
	        gtk_adjustment_set_page_increment(adjustmenth, pageIncrement);
	        gtk_adjustment_set_step_increment(adjustmenth, charWidth);
		gtk_adjustment_changed(GTK_ADJUSTMENT(adjustmenth));
		modified = true;
	}
#else
	if (GTK_ADJUSTMENT(adjustmenth)->upper != horizEndPreferred ||
	        GTK_ADJUSTMENT(adjustmenth)->page_size != pageWidth ||
	        GTK_ADJUSTMENT(adjustmenth)->page_increment != pageIncrement ||
	        GTK_ADJUSTMENT(adjustmenth)->step_increment != charWidth) {
		GTK_ADJUSTMENT(adjustmenth)->upper = horizEndPreferred;
		GTK_ADJUSTMENT(adjustmenth)->step_increment = charWidth;
		GTK_ADJUSTMENT(adjustmenth)->page_size = pageWidth;
		GTK_ADJUSTMENT(adjustmenth)->page_increment = pageIncrement;
		gtk_adjustment_changed(GTK_ADJUSTMENT(adjustmenth));
		modified = true;
	}
#endif
	if (modified && (paintState == painting)) {
		repaintFullWindow = true;
	}

	return modified;
}

void ScintillaGTK::ReconfigureScrollBars() {
	PRectangle rc = wMain.GetClientPosition();
	Resize(rc.Width(), rc.Height());
}

void ScintillaGTK::NotifyChange() {
	g_signal_emit(G_OBJECT(sci), scintilla_signals[COMMAND_SIGNAL], 0,
	                Platform::LongFromTwoShorts(GetCtrlID(), SCEN_CHANGE), PWidget(wMain));
}

void ScintillaGTK::NotifyFocus(bool focus) {
	g_signal_emit(G_OBJECT(sci), scintilla_signals[COMMAND_SIGNAL], 0,
	                Platform::LongFromTwoShorts
					(GetCtrlID(), focus ? SCEN_SETFOCUS : SCEN_KILLFOCUS), PWidget(wMain));
	Editor::NotifyFocus(focus);
}

void ScintillaGTK::NotifyParent(SCNotification scn) {
	scn.nmhdr.hwndFrom = PWidget(wMain);
	scn.nmhdr.idFrom = GetCtrlID();
	g_signal_emit(G_OBJECT(sci), scintilla_signals[NOTIFY_SIGNAL], 0,
	                GetCtrlID(), &scn);
}

void ScintillaGTK::NotifyKey(int key, int modifiers) {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_KEY;
	scn.ch = key;
	scn.modifiers = modifiers;

	NotifyParent(scn);
}

void ScintillaGTK::NotifyURIDropped(const char *list) {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_URIDROPPED;
	scn.text = list;

	NotifyParent(scn);
}

const char *CharacterSetID(int characterSet);

const char *ScintillaGTK::CharacterSetID() const {
	return ::CharacterSetID(vs.styles[STYLE_DEFAULT].characterSet);
}

class CaseFolderDBCS : public CaseFolderTable {
	const char *charSet;
public:
	explicit CaseFolderDBCS(const char *charSet_) : charSet(charSet_) {
		StandardASCII();
	}
	virtual size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) {
		if ((lenMixed == 1) && (sizeFolded > 0)) {
			folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
			return 1;
		} else if (*charSet) {
			std::string sUTF8 = ConvertText(mixed, lenMixed,
				"UTF-8", charSet, false);
			if (!sUTF8.empty()) {
				gchar *mapped = g_utf8_casefold(sUTF8.c_str(), sUTF8.length());
				size_t lenMapped = strlen(mapped);
				if (lenMapped < sizeFolded) {
					memcpy(folded, mapped,  lenMapped);
				} else {
					folded[0] = '\0';
					lenMapped = 1;
				}
				g_free(mapped);
				return lenMapped;
			}
		}
		// Something failed so return a single NUL byte
		folded[0] = '\0';
		return 1;
	}
};

CaseFolder *ScintillaGTK::CaseFolderForEncoding() {
	if (pdoc->dbcsCodePage == SC_CP_UTF8) {
		return new CaseFolderUnicode();
	} else {
		const char *charSetBuffer = CharacterSetID();
		if (charSetBuffer) {
			if (pdoc->dbcsCodePage == 0) {
				CaseFolderTable *pcf = new CaseFolderTable();
				pcf->StandardASCII();
				// Only for single byte encodings
				for (int i=0x80; i<0x100; i++) {
					char sCharacter[2] = "A";
					sCharacter[0] = i;
					// Silent as some bytes have no assigned character
					std::string sUTF8 = ConvertText(sCharacter, 1,
						"UTF-8", charSetBuffer, false, true);
					if (!sUTF8.empty()) {
						gchar *mapped = g_utf8_casefold(sUTF8.c_str(), sUTF8.length());
						if (mapped) {
							std::string mappedBack = ConvertText(mapped, strlen(mapped),
								charSetBuffer, "UTF-8", false, true);
							if ((mappedBack.length() == 1) && (mappedBack[0] != sCharacter[0])) {
								pcf->SetTranslation(sCharacter[0], mappedBack[0]);
							}
							g_free(mapped);
						}
					}
				}
				return pcf;
			} else {
				return new CaseFolderDBCS(charSetBuffer);
			}
		}
		return 0;
	}
}

namespace {

struct CaseMapper {
	gchar *mapped;	// Must be freed with g_free
	CaseMapper(const std::string &sUTF8, bool toUpperCase) {
		if (toUpperCase) {
			mapped = g_utf8_strup(sUTF8.c_str(), sUTF8.length());
		} else {
			mapped = g_utf8_strdown(sUTF8.c_str(), sUTF8.length());
		}
	}
	~CaseMapper() {
		g_free(mapped);
	}
};

}

std::string ScintillaGTK::CaseMapString(const std::string &s, int caseMapping) {
	if ((s.size() == 0) || (caseMapping == cmSame))
		return s;

	if (IsUnicodeMode()) {
		std::string retMapped(s.length() * maxExpansionCaseConversion, 0);
		size_t lenMapped = CaseConvertString(&retMapped[0], retMapped.length(), s.c_str(), s.length(),
			(caseMapping == cmUpper) ? CaseConversionUpper : CaseConversionLower);
		retMapped.resize(lenMapped);
		return retMapped;
	}

	const char *charSetBuffer = CharacterSetID();

	if (!*charSetBuffer) {
		CaseMapper mapper(s, caseMapping == cmUpper);
		return std::string(mapper.mapped, strlen(mapper.mapped));
	} else {
		// Change text to UTF-8
		std::string sUTF8 = ConvertText(s.c_str(), s.length(),
			"UTF-8", charSetBuffer, false);
		CaseMapper mapper(sUTF8, caseMapping == cmUpper);
		return ConvertText(mapper.mapped, strlen(mapper.mapped), charSetBuffer, "UTF-8", false);
	}
}

int ScintillaGTK::KeyDefault(int key, int modifiers) {
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

void ScintillaGTK::ClipboardReceived(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer data) {
	ScintillaGTK *sciThis = static_cast<ScintillaGTK *>(data);
	sciThis->ReceivedSelection(selection_data);
}

void ScintillaGTK::Paste() {
	atomSought = atomUTF8;
	GtkClipboard *clipBoard =
		gtk_widget_get_clipboard(GTK_WIDGET(PWidget(wMain)), atomClipboard);
	if (clipBoard == NULL)
		return;
	gtk_clipboard_request_contents(clipBoard, atomSought, ClipboardReceived, this);
}

void ScintillaGTK::CreateCallTipWindow(PRectangle rc) {
	if (!ct.wCallTip.Created()) {
		ct.wCallTip = gtk_window_new(GTK_WINDOW_POPUP);
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
				   G_CALLBACK(ScintillaGTK::PressCT), static_cast<void *>(this));
		gtk_widget_set_events(widcdrw,
			GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
	}
	gtk_widget_set_size_request(PWidget(ct.wDraw), rc.Width(), rc.Height());
	ct.wDraw.Show();
	if (PWindow(ct.wCallTip)) {
		gdk_window_resize(PWindow(ct.wCallTip), rc.Width(), rc.Height());
	}
}

void ScintillaGTK::AddToPopUp(const char *label, int cmd, bool enabled) {
	GtkWidget *menuItem;
	if (label[0])
		menuItem = gtk_menu_item_new_with_label(label);
	else
		menuItem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(popup.GetID()), menuItem);
	g_object_set_data(G_OBJECT(menuItem), "CmdNum", reinterpret_cast<void *>(cmd));
	g_signal_connect(G_OBJECT(menuItem),"activate", G_CALLBACK(PopUpCB), this);

	if (cmd) {
		if (menuItem)
			gtk_widget_set_sensitive(menuItem, enabled);
	}
}

bool ScintillaGTK::OwnPrimarySelection() {
	return ((gdk_selection_owner_get(GDK_SELECTION_PRIMARY)
		== PWindow(wMain)) &&
			(PWindow(wMain) != NULL));
}

void ScintillaGTK::ClaimSelection() {
	// X Windows has a 'primary selection' as well as the clipboard.
	// Whenever the user selects some text, we become the primary selection
	if (!sel.Empty() && IS_WIDGET_REALIZED(GTK_WIDGET(PWidget(wMain)))) {
		primarySelection = true;
		gtk_selection_owner_set(GTK_WIDGET(PWidget(wMain)),
		                        GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
		primary.Clear();
	} else if (OwnPrimarySelection()) {
		primarySelection = true;
		if (primary.Empty())
			gtk_selection_owner_set(NULL, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
	} else {
		primarySelection = false;
		primary.Clear();
	}
}

#if GTK_CHECK_VERSION(3,0,0)
static const guchar *DataOfGSD(GtkSelectionData *sd) { return gtk_selection_data_get_data(sd); }
static gint LengthOfGSD(GtkSelectionData *sd) { return gtk_selection_data_get_length(sd); }
static GdkAtom TypeOfGSD(GtkSelectionData *sd) { return gtk_selection_data_get_data_type(sd); }
static GdkAtom SelectionOfGSD(GtkSelectionData *sd) { return gtk_selection_data_get_selection(sd); }
#else
static const guchar *DataOfGSD(GtkSelectionData *sd) { return sd->data; }
static gint LengthOfGSD(GtkSelectionData *sd) { return sd->length; }
static GdkAtom TypeOfGSD(GtkSelectionData *sd) { return sd->type; }
static GdkAtom SelectionOfGSD(GtkSelectionData *sd) { return sd->selection; }
#endif

// Detect rectangular text, convert line ends to current mode, convert from or to UTF-8
void ScintillaGTK::GetGtkSelectionText(GtkSelectionData *selectionData, SelectionText &selText) {
	const char *data = reinterpret_cast<const char *>(DataOfGSD(selectionData));
	int len = LengthOfGSD(selectionData);
	GdkAtom selectionTypeData = TypeOfGSD(selectionData);

	// Return empty string if selection is not a string
	if ((selectionTypeData != GDK_TARGET_STRING) && (selectionTypeData != atomUTF8)) {
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
			dest = UTF8FromLatin1(dest.c_str(), dest.length());
			selText.Copy(dest, SC_CP_UTF8, 0, isRectangular, false);
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
			selText.Copy(dest, SC_CP_UTF8, 0, isRectangular, false);
		}
	}
}

void ScintillaGTK::ReceivedSelection(GtkSelectionData *selection_data) {
	try {
		if ((SelectionOfGSD(selection_data) == atomClipboard) ||
		        (SelectionOfGSD(selection_data) == GDK_SELECTION_PRIMARY)) {
			if ((atomSought == atomUTF8) && (LengthOfGSD(selection_data) <= 0)) {
				atomSought = atomString;
				gtk_selection_convert(GTK_WIDGET(PWidget(wMain)),
				        SelectionOfGSD(selection_data), atomSought, GDK_CURRENT_TIME);
			} else if ((LengthOfGSD(selection_data) > 0) &&
			        ((TypeOfGSD(selection_data) == GDK_TARGET_STRING) || (TypeOfGSD(selection_data) == atomUTF8))) {
				SelectionText selText;
				GetGtkSelectionText(selection_data, selText);

				UndoGroup ug(pdoc);
				if (SelectionOfGSD(selection_data) != GDK_SELECTION_PRIMARY) {
					ClearSelection(multiPasteMode == SC_MULTIPASTE_EACH);
				}

				InsertPasteShape(selText.Data(), selText.Length(),
					selText.rectangular ? pasteRectangular : pasteStream);
				EnsureCaretVisible();
			}
		}
//	else fprintf(stderr, "Target non string %d %d\n", (int)(selection_data->type),
//		(int)(atomUTF8));
		Redraw();
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::ReceivedDrop(GtkSelectionData *selection_data) {
	dragWasDropped = true;
	if (TypeOfGSD(selection_data) == atomUriList || TypeOfGSD(selection_data) == atomDROPFILES_DND) {
		const char *data = reinterpret_cast<const char *>(DataOfGSD(selection_data));
		std::vector<char> drop(data, data + LengthOfGSD(selection_data));
		drop.push_back('\0');
		NotifyURIDropped(&drop[0]);
	} else if ((TypeOfGSD(selection_data) == GDK_TARGET_STRING) || (TypeOfGSD(selection_data) == atomUTF8)) {
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
	SelectionText *newline_normalized = NULL;
	{
		std::string tmpstr = Document::TransformLineEnds(text->Data(), text->Length(), SC_EOL_LF);
		newline_normalized = new SelectionText();
		newline_normalized->Copy(tmpstr, SC_CP_UTF8, 0, text->rectangular, false);
		text = newline_normalized;
	}
#endif

	// Convert text to utf8 if it isn't already
	SelectionText *converted = 0;
	if ((text->codePage != SC_CP_UTF8) && (info == TARGET_UTF8_STRING)) {
		const char *charSet = ::CharacterSetID(text->characterSet);
		if (*charSet) {
			std::string tmputf = ConvertText(text->Data(), text->Length(), "UTF-8", charSet, false);
			converted = new SelectionText();
			converted->Copy(tmputf, SC_CP_UTF8, 0, text->rectangular, false);
			text = converted;
		}
	}

	// Here is a somewhat evil kludge.
	// As I can not work out how to store data on the clipboard in multiple formats
	// and need some way to mark the clipping as being stream or rectangular,
	// the terminating \0 is included in the length for rectangular clippings.
	// All other tested aplications behave benignly by ignoring the \0.
	// The #if is here because on Windows cfColumnSelect clip entry is used
	// instead as standard indicator of rectangularness (so no need to kludge)
	const char *textData = text->Data();
	int len = text->Length();
#if PLAT_GTK_WIN32 == 0
	if (text->rectangular)
		len++;
#endif

	if (info == TARGET_UTF8_STRING) {
		gtk_selection_data_set_text(selection_data, textData, len);
	} else {
		gtk_selection_data_set(selection_data,
			static_cast<GdkAtom>(GDK_SELECTION_TYPE_STRING),
			8, reinterpret_cast<const unsigned char *>(textData), len);
	}
	delete converted;

#if PLAT_GTK_WIN32
	delete newline_normalized;
#endif
}

void ScintillaGTK::StoreOnClipboard(SelectionText *clipText) {
	GtkClipboard *clipBoard =
		gtk_widget_get_clipboard(GTK_WIDGET(PWidget(wMain)), atomClipboard);
	if (clipBoard == NULL) // Occurs if widget isn't in a toplevel
		return;

	if (gtk_clipboard_set_with_data(clipBoard, clipboardCopyTargets, nClipboardCopyTargets,
				    ClipboardGetSelection, ClipboardClearSelection, clipText)) {
		gtk_clipboard_set_can_store(clipBoard, clipboardCopyTargets, nClipboardCopyTargets);
	}
}

void ScintillaGTK::ClipboardGetSelection(GtkClipboard *, GtkSelectionData *selection_data, guint info, void *data) {
	GetSelection(selection_data, info, static_cast<SelectionText*>(data));
}

void ScintillaGTK::ClipboardClearSelection(GtkClipboard *, void *data) {
	SelectionText *obj = static_cast<SelectionText*>(data);
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
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::Resize(int width, int height) {
	//Platform::DebugPrintf("Resize %d %d\n", width, height);
	//printf("Resize %d %d\n", width, height);

	// Not always needed, but some themes can have different sizes of scrollbars
#if GTK_CHECK_VERSION(3,0,0)
	GtkRequisition requisition;
	gtk_widget_get_preferred_size(PWidget(scrollbarv), NULL, &requisition);
	verticalScrollBarWidth = requisition.width;
	gtk_widget_get_preferred_size(PWidget(scrollbarh), NULL, &requisition);
	horizontalScrollBarHeight = requisition.height;
#else
	verticalScrollBarWidth = GTK_WIDGET(PWidget(scrollbarv))->requisition.width;
	horizontalScrollBarHeight = GTK_WIDGET(PWidget(scrollbarh))->requisition.height;
#endif

	// These allocations should never produce negative sizes as they would wrap around to huge
	// unsigned numbers inside GTK+ causing warnings.
	bool showSBHorizontal = horizontalScrollBarVisible && !Wrapping();

	GtkAllocation alloc;
	if (showSBHorizontal) {
		gtk_widget_show(GTK_WIDGET(PWidget(scrollbarh)));
		alloc.x = 0;
		alloc.y = height - horizontalScrollBarHeight;
		alloc.width = Platform::Maximum(1, width - verticalScrollBarWidth);
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
		alloc.height = Platform::Maximum(1, height - horizontalScrollBarHeight);
		gtk_widget_size_allocate(GTK_WIDGET(PWidget(scrollbarv)), &alloc);
	} else {
		gtk_widget_hide(GTK_WIDGET(PWidget(scrollbarv)));
		verticalScrollBarWidth = 0;
	}
	if (IS_WIDGET_MAPPED(PWidget(wMain))) {
		ChangeSize();
	}

	alloc.x = 0;
	alloc.y = 0;
	alloc.width = Platform::Maximum(1, width - verticalScrollBarWidth);
	alloc.height = Platform::Maximum(1, height - horizontalScrollBarHeight);
	gtk_widget_size_allocate(GTK_WIDGET(PWidget(wText)), &alloc);
}

static void SetAdjustmentValue(GtkAdjustment *object, int value) {
	GtkAdjustment *adjustment = GTK_ADJUSTMENT(object);
#if GTK_CHECK_VERSION(3,0,0)
	int maxValue = static_cast<int>(
		gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));
#else
	int maxValue = static_cast<int>(
		adjustment->upper - adjustment->page_size);
#endif

	if (value > maxValue)
		value = maxValue;
	if (value < 0)
		value = 0;
	gtk_adjustment_set_value(adjustment, value);
}

static int modifierTranslated(int sciModifier) {
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

gint ScintillaGTK::PressThis(GdkEventButton *event) {
	try {
		//Platform::DebugPrintf("Press %x time=%d state = %x button = %x\n",this,event->time, event->state, event->button);
		// Do not use GTK+ double click events as Scintilla has its own double click detection
		if (event->type != GDK_BUTTON_PRESS)
			return FALSE;

		if (evbtn) {
			gdk_event_free(reinterpret_cast<GdkEvent *>(evbtn));
			evbtn = 0;
		}
		evbtn = reinterpret_cast<GdkEventButton *>(gdk_event_copy(reinterpret_cast<GdkEvent *>(event)));
		Point pt;
		pt.x = int(event->x);
		pt.y = int(event->y);
		PRectangle rcClient = GetClientRectangle();
		//Platform::DebugPrintf("Press %0d,%0d in %0d,%0d %0d,%0d\n",
		//	pt.x, pt.y, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
		if ((pt.x > rcClient.right) || (pt.y > rcClient.bottom)) {
			Platform::DebugPrintf("Bad location\n");
			return FALSE;
		}

		bool shift = (event->state & GDK_SHIFT_MASK) != 0;
		bool ctrl = (event->state & GDK_CONTROL_MASK) != 0;
		// On X, instead of sending literal modifiers use the user specified
		// modifier, defaulting to control instead of alt.
		// This is because most X window managers grab alt + click for moving
		bool alt = (event->state & modifierTranslated(rectangularSelectionModifier)) != 0;

		gtk_widget_grab_focus(PWidget(wMain));
		if (event->button == 1) {
#if PLAT_GTK_MACOSX
			bool meta = ctrl;
			// GDK reports the Command modifer key as GDK_MOD2_MASK for button events,
			// not GDK_META_MASK like in key events.
			ctrl = (event->state & GDK_MOD2_MASK) != 0;
#else
			bool meta = false;
#endif
			ButtonDownWithModifiers(pt, event->time, ModifierFlags(shift, ctrl, alt, meta));
		} else if (event->button == 2) {
			// Grab the primary selection if it exists
			SelectionPosition pos = SPositionFromLocation(pt, false, false, UserVirtualSpace());
			if (OwnPrimarySelection() && primary.Empty())
				CopySelectionRange(&primary);

			sel.Clear();
			SetSelection(pos, pos);
			atomSought = atomUTF8;
			gtk_selection_convert(GTK_WIDGET(PWidget(wMain)), GDK_SELECTION_PRIMARY,
			        atomSought, event->time);
		} else if (event->button == 3) {
			if (!PointInSelection(pt))
				SetEmptySelection(PositionFromLocation(pt));
			if (displayPopupMenu) {
				// PopUp menu
				// Convert to screen
				int ox = 0;
				int oy = 0;
				gdk_window_get_origin(PWindow(wMain), &ox, &oy);
				ContextMenu(Point(pt.x + ox, pt.y + oy));
			} else {
				return FALSE;
			}
		} else if (event->button == 4) {
			// Wheel scrolling up (only GTK 1.x does it this way)
			if (ctrl)
				SetAdjustmentValue(adjustmenth, xOffset - 6);
			else
				SetAdjustmentValue(adjustmentv, topLine - 3);
		} else if (event->button == 5) {
			// Wheel scrolling down (only GTK 1.x does it this way)
			if (ctrl)
				SetAdjustmentValue(adjustmenth, xOffset + 6);
			else
				SetAdjustmentValue(adjustmentv, topLine + 3);
		}
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return TRUE;
}

gint ScintillaGTK::Press(GtkWidget *widget, GdkEventButton *event) {
	if (event->window != WindowFromWidget(widget))
		return FALSE;
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	return sciThis->PressThis(event);
}

gint ScintillaGTK::MouseRelease(GtkWidget *widget, GdkEventButton *event) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {
		//Platform::DebugPrintf("Release %x %d %d\n",sciThis,event->time,event->state);
		if (!sciThis->HaveMouseCapture())
			return FALSE;
		if (event->button == 1) {
			Point pt;
			pt.x = int(event->x);
			pt.y = int(event->y);
			//Platform::DebugPrintf("Up %x %x %d %d %d\n",
			//	sciThis,event->window,event->time, pt.x, pt.y);
			if (event->window != PWindow(sciThis->wMain))
				// If mouse released on scroll bar then the position is relative to the
				// scrollbar, not the drawing window so just repeat the most recent point.
				pt = sciThis->ptMouseLast;
			sciThis->ButtonUp(pt, event->time, (event->state & GDK_CONTROL_MASK) != 0);
		}
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

// win32gtk and GTK >= 2 use SCROLL_* events instead of passing the
// button4/5/6/7 events to the GTK app
gint ScintillaGTK::ScrollEvent(GtkWidget *widget, GdkEventScroll *event) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {

		if (widget == NULL || event == NULL)
			return FALSE;

		// Compute amount and direction to scroll (even tho on win32 there is
		// intensity of scrolling info in the native message, gtk doesn't
		// support this so we simulate similarly adaptive scrolling)
		// Note that this is disabled on OS X (Darwin) with the X11 backend
		// where the X11 server already has an adaptive scrolling algorithm 
		// that fights with this one
		int cLineScroll;
#if defined(__APPLE__) && !defined(GDK_WINDOWING_QUARTZ)
		cLineScroll = sciThis->linesPerScroll;
		if (cLineScroll == 0)
			cLineScroll = 4;
		sciThis->wheelMouseIntensity = cLineScroll;
#else
		int timeDelta = 1000000;
		GTimeVal curTime;
		g_get_current_time(&curTime);
		if (curTime.tv_sec == sciThis->lastWheelMouseTime.tv_sec)
			timeDelta = curTime.tv_usec - sciThis->lastWheelMouseTime.tv_usec;
		else if (curTime.tv_sec == sciThis->lastWheelMouseTime.tv_sec + 1)
			timeDelta = 1000000 + (curTime.tv_usec - sciThis->lastWheelMouseTime.tv_usec);
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
#endif
		if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_LEFT) {
			cLineScroll *= -1;
		}
		g_get_current_time(&sciThis->lastWheelMouseTime);
		sciThis->lastWheelMouseDirection = event->direction;

		// Note:  Unpatched versions of win32gtk don't set the 'state' value so
		// only regular scrolling is supported there.  Also, unpatched win32gtk
		// issues spurious button 2 mouse events during wheeling, which can cause
		// problems (a patch for both was submitted by archaeopteryx.com on 13Jun2001)

		// Data zoom not supported
		if (event->state & GDK_SHIFT_MASK) {
			return FALSE;
		}

#if GTK_CHECK_VERSION(3,4,0)
		// Smooth scrolling not supported
		if (event->direction == GDK_SCROLL_SMOOTH) {
			return FALSE;
		}
#endif

		// Horizontal scrolling
		if (event->direction == GDK_SCROLL_LEFT || event->direction == GDK_SCROLL_RIGHT) {
			sciThis->HorizontalScrollTo(sciThis->xOffset + cLineScroll);

			// Text font size zoom
		} else if (event->state & GDK_CONTROL_MASK) {
			if (cLineScroll < 0) {
				sciThis->KeyCommand(SCI_ZOOMIN);
			} else {
				sciThis->KeyCommand(SCI_ZOOMOUT);
			}

			// Regular scrolling
		} else {
			sciThis->ScrollTo(sciThis->topLine + cLineScroll);
		}
		return TRUE;
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gint ScintillaGTK::Motion(GtkWidget *widget, GdkEventMotion *event) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {
		//Platform::DebugPrintf("Motion %x %d\n",sciThis,event->time);
		if (event->window != WindowFromWidget(widget))
			return FALSE;
		int x = 0;
		int y = 0;
		GdkModifierType state;
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
		Point pt(x, y);
		int modifiers = ((event->state & GDK_SHIFT_MASK) != 0 ? SCI_SHIFT : 0) |
		                ((event->state & GDK_CONTROL_MASK) != 0 ? SCI_CTRL : 0) |
		                ((event->state & modifierTranslated(sciThis->rectangularSelectionModifier)) != 0 ? SCI_ALT : 0);
		sciThis->ButtonMoveWithModifiers(pt, modifiers);
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

// Map the keypad keys to their equivalent functions
static int KeyTranslate(int keyIn) {
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

gboolean ScintillaGTK::KeyThis(GdkEventKey *event) {
	try {
		//fprintf(stderr, "SC-key: %d %x [%s]\n",
		//	event->keyval, event->state, (event->length > 0) ? event->string : "empty");
		if (gtk_im_context_filter_keypress(im_context, event)) {
			return 1;
		}
		if (!event->keyval) {
			return true;
		}

		bool shift = (event->state & GDK_SHIFT_MASK) != 0;
		bool ctrl = (event->state & GDK_CONTROL_MASK) != 0;
		bool alt = (event->state & GDK_MOD1_MASK) != 0;
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
		bool added = KeyDown(key, shift, ctrl, alt, &consumed) != 0;
#else
		bool meta = ctrl;
		ctrl = (event->state & GDK_META_MASK) != 0;
		bool added = KeyDownWithModifiers(key, (shift ? SCI_SHIFT : 0) |
		                                       (ctrl ? SCI_CTRL : 0) |
		                                       (alt ? SCI_ALT : 0) |
		                                       (meta ? SCI_META : 0), &consumed) != 0;
#endif
		if (!consumed)
			consumed = added;
		//fprintf(stderr, "SK-key: %d %x %x\n",event->keyval, event->state, consumed);
		if (event->keyval == 0xffffff && event->length > 0) {
			ClearSelection();
			const int lengthInserted = pdoc->InsertString(CurrentPosition(), event->string, strlen(event->string));
			if (lengthInserted > 0) {
				MovePositionTo(CurrentPosition() + lengthInserted);
			}
		}
		return consumed;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gboolean ScintillaGTK::KeyPress(GtkWidget *widget, GdkEventKey *event) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	return sciThis->KeyThis(event);
}

gboolean ScintillaGTK::KeyRelease(GtkWidget *widget, GdkEventKey *event) {
	//Platform::DebugPrintf("SC-keyrel: %d %x %3s\n",event->keyval, event->state, event->string);
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	if (gtk_im_context_filter_keypress(sciThis->im_context, event)) {
		return TRUE;
	}
	return FALSE;
}

#if GTK_CHECK_VERSION(3,0,0)

gboolean ScintillaGTK::DrawPreeditThis(GtkWidget *widget, cairo_t *cr) {
	try {
		PreEditString pes(im_context);
		PangoLayout *layout = gtk_widget_create_pango_layout(PWidget(wText), pes.str);
		pango_layout_set_attributes(layout, pes.attrs);

		cairo_move_to(cr, 0, 0);
		pango_cairo_show_layout(cr, layout);

		g_object_unref(layout);
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return TRUE;
}

gboolean ScintillaGTK::DrawPreedit(GtkWidget *widget, cairo_t *cr, ScintillaGTK *sciThis) {
	return sciThis->DrawPreeditThis(widget, cr);
}

#else

gboolean ScintillaGTK::ExposePreeditThis(GtkWidget *widget, GdkEventExpose *ose) {
	try {
		PreEditString pes(im_context);
		PangoLayout *layout = gtk_widget_create_pango_layout(PWidget(wText), pes.str);
		pango_layout_set_attributes(layout, pes.attrs);

		cairo_t *context = gdk_cairo_create(reinterpret_cast<GdkDrawable *>(WindowFromWidget(widget)));
		cairo_move_to(context, 0, 0);
		pango_cairo_show_layout(context, layout);
		cairo_destroy(context);
		g_object_unref(layout);
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return TRUE;
}

gboolean ScintillaGTK::ExposePreedit(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis) {
	return sciThis->ExposePreeditThis(widget, ose);
}

#endif

bool ScintillaGTK::KoreanIME() {
	PreEditString pes(im_context);
	if (pes.pscript != PANGO_SCRIPT_COMMON)
		lastNonCommonScript = pes.pscript;
	return lastNonCommonScript == PANGO_SCRIPT_HANGUL;
}

void ScintillaGTK::MoveImeCarets(int pos) {
	// Move carets relatively by bytes
	for (size_t r=0; r<sel.Count(); r++) {
		int positionInsert = sel.Range(r).Start().Position();
		sel.Range(r).caret.SetPosition(positionInsert + pos);
		sel.Range(r).anchor.SetPosition(positionInsert + pos);
	}
}

void ScintillaGTK::DrawImeIndicator(int indicator, int len) {
	// Emulate the visual style of IME characters with indicators.
	// Draw an indicator on the character before caret by the character bytes of len
	// so it should be called after addCharUTF().
	// It does not affect caret positions.
	if (indicator < 8 || indicator > INDIC_MAX) {
		return;
	}
	pdoc->decorations.SetCurrentIndicator(indicator);
	for (size_t r=0; r<sel.Count(); r++) {
		int positionInsert = sel.Range(r).Start().Position();
		pdoc->DecorationFillRange(positionInsert - len, 1, len);
	}
}

void ScintillaGTK::GetImeUnderlines(PangoAttrList *attrs, bool *normalInput) {
	// Whether single underlines attribute is or not
	// attr position is counted by the number of UTF-8 bytes
	PangoAttrIterator *iterunderline = pango_attr_list_get_iterator(attrs);
	if (iterunderline) {
		do {
			PangoAttribute  *attrunderline = pango_attr_iterator_get(iterunderline, PANGO_ATTR_UNDERLINE);
			if (attrunderline) {
				glong start = attrunderline->start_index;
				glong end = attrunderline->end_index;
				PangoUnderline uline = (PangoUnderline)((PangoAttrInt *)attrunderline)->value;
				for (glong i=start; i < end; ++i) {
					switch (uline) {
					case PANGO_UNDERLINE_NONE:
						normalInput[i] = false;
						break;
					case PANGO_UNDERLINE_SINGLE: // normal input
						normalInput[i] = true;
						break;
					case PANGO_UNDERLINE_DOUBLE:
					case PANGO_UNDERLINE_LOW:
					case PANGO_UNDERLINE_ERROR:
						break;
					}
				}
			}
		} while (pango_attr_iterator_next(iterunderline));
		pango_attr_iterator_destroy(iterunderline);
	}
}

void ScintillaGTK::GetImeBackgrounds(PangoAttrList *attrs, bool *targetInput) {
	// Whether background color attribue is or not
	// attr position is measured in UTF-8 bytes
	PangoAttrIterator *itercolor = pango_attr_list_get_iterator(attrs);
	if (itercolor) {
		do {
			PangoAttribute  *backcolor = pango_attr_iterator_get(itercolor, PANGO_ATTR_BACKGROUND);
			if (backcolor) {
				glong start = backcolor->start_index;
				glong end =  backcolor->end_index;
				for (glong i=start; i < end; ++i) {
					targetInput[i] = true;  // target converted
				}
			}
		} while (pango_attr_iterator_next(itercolor));
		pango_attr_iterator_destroy(itercolor);
	}
}

void ScintillaGTK::SetCandidateWindowPos() {
	// Composition box accompanies candidate box.
	Point pt = PointMainCaret();
	GdkRectangle imeBox = {0}; // No need to set width
	imeBox.x = pt.x;           // Only need positiion
	imeBox.y = pt.y + vs.lineHeight; // underneath the first charater
	gtk_im_context_set_cursor_location(im_context, &imeBox);
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
		gunichar *uniStr = g_utf8_to_ucs4_fast(commitStr, strlen(commitStr), &uniStrLen);
		for (glong i = 0; i < uniStrLen; i++) {

			gunichar uniChar[1] = {0};
			uniChar[0] = uniStr[i];

			glong oneCharLen = 0;
			gchar *oneChar = g_ucs4_to_utf8(uniChar, 1, NULL, &oneCharLen, NULL);

			if (IsUnicodeMode()) {
				// Do nothing ;
			} else {
				std::string oneCharSTD = ConvertText(oneChar, oneCharLen, charSetSource, "UTF-8", true);
				oneCharLen = oneCharSTD.copy(oneChar,oneCharSTD.length(), 0);
				oneChar[oneCharLen] = '\0';
			}

			AddCharUTF(oneChar, oneCharLen);
			g_free(oneChar);
		}
		g_free(uniStr);
		ShowCaretAtCurrentPosition();
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::Commit(GtkIMContext *, char  *str, ScintillaGTK *sciThis) {
	sciThis->CommitThis(str);
}

void ScintillaGTK::PreeditChangedInlineThis() {
	// Copy & paste by johnsonj with a lot of helps of Neil
	// Great thanks for my foreruners, jiniya and BLUEnLIVE
	try {
		view.imeCaretBlockOverride = false; // If backspace.

		if (pdoc->TentativeActive()) {
			pdoc->TentativeUndo();
		} else {
			// No tentative undo means start of this composition so
			// fill in any virtual spaces.
			FillVirtualSpace();
		}

		PreEditString preeditStr(im_context);
		const char *charSetSource = CharacterSetID();

		if (!preeditStr.validUTF8 || (charSetSource == NULL)) {
			ShowCaretAtCurrentPosition();
			return;
		}

		if (preeditStr.uniStrLen == 0 || preeditStr.uniStrLen > maxLenInputIME) {
			//fprintf(stderr, "Do not allow over 200 chars: %i\n", preeditStr.uniStrLen);
			ShowCaretAtCurrentPosition();
			return;
		}

		pdoc->TentativeStart(); // TentativeActive() from now on

		// Get preedit string attribues
		bool normalInput[maxLenInputIME*3+1] = {false};
		bool targetInput[maxLenInputIME*3+1] = {false};
		GetImeUnderlines(preeditStr.attrs, normalInput);
		GetImeBackgrounds(preeditStr.attrs, targetInput);

		// Display preedit characters, one by one
		glong imeCharPos[maxLenInputIME+1] = { 0 };
		glong attrPos = -1; // Start at -1 to designate the last byte of one character.
		glong charWidth = 0;

		bool tmpRecordingMacro = recordingMacro;
		recordingMacro = false;
		for (glong i = 0; i < preeditStr.uniStrLen; i++) {

			gunichar uniChar[1] = {0};
			uniChar[0] = preeditStr.uniStr[i];

			glong oneCharLen = 0;
			gchar *oneChar = g_ucs4_to_utf8(uniChar, 1, NULL, &oneCharLen, NULL);

			// Record attribute positions in UTF-8 bytes
			attrPos += oneCharLen;

			if (IsUnicodeMode()) {
				// Do nothing
			} else {
				std::string oneCharSTD = ConvertText(oneChar, oneCharLen, charSetSource, "UTF-8", true);
				oneCharLen = oneCharSTD.copy(oneChar,oneCharSTD.length(), 0);
				oneChar[oneCharLen] = '\0';
			}

			// Record character positions in UTF-8 or DBCS bytes

			charWidth += oneCharLen;
			imeCharPos[i+1] = charWidth;

			// Display one character
			AddCharUTF(oneChar, oneCharLen);

			// Draw an indicator on the character,
			// Overlapping allowed
			if (normalInput[attrPos]) {
				DrawImeIndicator(SC_INDICATOR_INPUT, oneCharLen);
			}
			if (targetInput[attrPos]) {
				DrawImeIndicator(SC_INDICATOR_TARGET, oneCharLen);
			}
			g_free(oneChar);
		}
		recordingMacro = tmpRecordingMacro;

		// Move caret to ime cursor position.
		if (KoreanIME()) {
			view.imeCaretBlockOverride = true;
			MoveImeCarets( - (imeCharPos[preeditStr.uniStrLen]));

		} else {
			MoveImeCarets( - (imeCharPos[preeditStr.uniStrLen]) + imeCharPos[preeditStr.cursor_pos]);
		}

		SetCandidateWindowPos();
		ShowCaretAtCurrentPosition();
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::PreeditChangedWindowedThis() {
	try {
		PreEditString pes(im_context);
		if (strlen(pes.str) > 0) {
			PangoLayout *layout = gtk_widget_create_pango_layout(PWidget(wText), pes.str);
			pango_layout_set_attributes(layout, pes.attrs);

			gint w, h;
			pango_layout_get_pixel_size(layout, &w, &h);
			g_object_unref(layout);

			gint x, y;
			gdk_window_get_origin(PWindow(wText), &x, &y);

			Point pt = PointMainCaret();
			if (pt.x < 0)
				pt.x = 0;
			if (pt.y < 0)
				pt.y = 0;

			gtk_window_move(GTK_WINDOW(PWidget(wPreedit)), x + pt.x, y + pt.y);
			gtk_window_resize(GTK_WINDOW(PWidget(wPreedit)), w, h);
			gtk_widget_show(PWidget(wPreedit));
			gtk_widget_queue_draw_area(PWidget(wPreeditDraw), 0, 0, w, h);
		} else {
			gtk_widget_hide(PWidget(wPreedit));
		}
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::PreeditChanged(GtkIMContext *, ScintillaGTK *sciThis) {
	if ((sciThis->imeInteraction == imeInline) || (sciThis->KoreanIME())) {
		sciThis->PreeditChangedInlineThis();
	} else {
		sciThis->PreeditChangedWindowedThis();
	}
}

void ScintillaGTK::StyleSetText(GtkWidget *widget, GtkStyle *, void*) {
	RealizeText(widget, NULL);
}

void ScintillaGTK::RealizeText(GtkWidget *widget, void*) {
	// Set NULL background to avoid automatic clearing so Scintilla responsible for all drawing
	if (WindowFromWidget(widget)) {
#if GTK_CHECK_VERSION(3,0,0)
		gdk_window_set_background_pattern(WindowFromWidget(widget), NULL);
#else
		gdk_window_set_back_pixmap(WindowFromWidget(widget), NULL, FALSE);
#endif
	}
}

static GObjectClass *scintilla_class_parent_class;

void ScintillaGTK::Destroy(GObject *object) {
	try {
		ScintillaObject *scio = reinterpret_cast<ScintillaObject *>(object);

		// This avoids a double destruction
		if (!scio->pscin)
			return;
		ScintillaGTK *sciThis = reinterpret_cast<ScintillaGTK *>(scio->pscin);
		//Platform::DebugPrintf("Destroying %x %x\n", sciThis, object);
		sciThis->Finalise();

		delete sciThis;
		scio->pscin = 0;
		scintilla_class_parent_class->finalize(object);
	} catch (...) {
		// Its dead so nowhere to save the status
	}
}

#if GTK_CHECK_VERSION(3,0,0)

gboolean ScintillaGTK::DrawTextThis(cairo_t *cr) {
	try {
		paintState = painting;
		repaintFullWindow = false;

		rcPaint = GetClientRectangle();

		PLATFORM_ASSERT(rgnUpdate == NULL);
		rgnUpdate = cairo_copy_clip_rectangle_list(cr);
		if (rgnUpdate && rgnUpdate->status != CAIRO_STATUS_SUCCESS) {
			// If not successful then ignore
			fprintf(stderr, "DrawTextThis failed to copy update region %d [%d]\n", rgnUpdate->status, rgnUpdate->num_rectangles);
			cairo_rectangle_list_destroy(rgnUpdate);
			rgnUpdate = 0;
		}

		double x1, y1, x2, y2;
		cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
		rcPaint.left = x1;
		rcPaint.top = y1;
		rcPaint.right = x2;
		rcPaint.bottom = y2;
		PRectangle rcClient = GetClientRectangle();
		paintingAllText = rcPaint.Contains(rcClient);
		Surface *surfaceWindow = Surface::Allocate(SC_TECHNOLOGY_DEFAULT);
		if (surfaceWindow) {
			surfaceWindow->Init(cr, PWidget(wText));
			Paint(surfaceWindow, rcPaint);
			surfaceWindow->Release();
			delete surfaceWindow;
		}
		if ((paintState == paintAbandoned) || repaintFullWindow) {
			// Painting area was insufficient to cover new styling or brace highlight positions
			FullPaint();
		}
		paintState = notPainting;
		repaintFullWindow = false;

		if (rgnUpdate) {
			cairo_rectangle_list_destroy(rgnUpdate);
		}
		rgnUpdate = 0;
		paintState = notPainting;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}

	return FALSE;
}

gboolean ScintillaGTK::DrawText(GtkWidget *, cairo_t *cr, ScintillaGTK *sciThis) {
	return sciThis->DrawTextThis(cr);
}

gboolean ScintillaGTK::DrawThis(cairo_t *cr) {
	try {
		gtk_container_propagate_draw(
		    GTK_CONTAINER(PWidget(wMain)), PWidget(scrollbarh), cr);
		gtk_container_propagate_draw(
		    GTK_CONTAINER(PWidget(wMain)), PWidget(scrollbarv), cr);
// Starting from the following version, the expose event are not propagated
// for double buffered non native windows, so we need to call it ourselves
// or keep the default handler
#if GTK_CHECK_VERSION(3,0,0)
		// we want to forward on any >= 3.9.2 runtime
		if (gtk_check_version(3,9,2) == NULL) {
			gtk_container_propagate_draw(
					GTK_CONTAINER(PWidget(wMain)), PWidget(wText), cr);
		}
#endif
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gboolean ScintillaGTK::DrawMain(GtkWidget *widget, cairo_t *cr) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	return sciThis->DrawThis(cr);
}

#else

gboolean ScintillaGTK::ExposeTextThis(GtkWidget * /*widget*/, GdkEventExpose *ose) {
	try {
		paintState = painting;

		rcPaint.left = ose->area.x;
		rcPaint.top = ose->area.y;
		rcPaint.right = ose->area.x + ose->area.width;
		rcPaint.bottom = ose->area.y + ose->area.height;

		PLATFORM_ASSERT(rgnUpdate == NULL);
		rgnUpdate = gdk_region_copy(ose->region);
		PRectangle rcClient = GetClientRectangle();
		paintingAllText = rcPaint.Contains(rcClient);
		Surface *surfaceWindow = Surface::Allocate(SC_TECHNOLOGY_DEFAULT);
		if (surfaceWindow) {
			cairo_t *cr = gdk_cairo_create(PWindow(wText));
			surfaceWindow->Init(cr, PWidget(wText));
			Paint(surfaceWindow, rcPaint);
			surfaceWindow->Release();
			delete surfaceWindow;
			cairo_destroy(cr);
		}
		if (paintState == paintAbandoned) {
			// Painting area was insufficient to cover new styling or brace highlight positions
			FullPaint();
		}
		paintState = notPainting;

		if (rgnUpdate) {
			gdk_region_destroy(rgnUpdate);
		}
		rgnUpdate = 0;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}

	return FALSE;
}

gboolean ScintillaGTK::ExposeText(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis) {
	return sciThis->ExposeTextThis(widget, ose);
}

gboolean ScintillaGTK::ExposeMain(GtkWidget *widget, GdkEventExpose *ose) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
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
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

#endif

void ScintillaGTK::ScrollSignal(GtkAdjustment *adj, ScintillaGTK *sciThis) {
	try {
#if GTK_CHECK_VERSION(3,0,0)
		sciThis->ScrollTo(static_cast<int>(gtk_adjustment_get_value(adj)), false);
#else
		sciThis->ScrollTo(static_cast<int>(adj->value), false);
#endif
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::ScrollHSignal(GtkAdjustment *adj, ScintillaGTK *sciThis) {
	try {
#if GTK_CHECK_VERSION(3,0,0)
		sciThis->HorizontalScrollTo(static_cast<int>(gtk_adjustment_get_value(adj)));
#else
		sciThis->HorizontalScrollTo(static_cast<int>(adj->value));
#endif
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::SelectionReceived(GtkWidget *widget,
                                     GtkSelectionData *selection_data, guint) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	//Platform::DebugPrintf("Selection received\n");
	sciThis->ReceivedSelection(selection_data);
}

void ScintillaGTK::SelectionGet(GtkWidget *widget,
                                GtkSelectionData *selection_data, guint info, guint) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {
		//Platform::DebugPrintf("Selection get\n");
		if (SelectionOfGSD(selection_data) == GDK_SELECTION_PRIMARY) {
			if (sciThis->primary.Empty()) {
				sciThis->CopySelectionRange(&sciThis->primary);
			}
			sciThis->GetSelection(selection_data, info, &sciThis->primary);
		}
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

gint ScintillaGTK::SelectionClear(GtkWidget *widget, GdkEventSelection *selection_event) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
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
		Point npt(x, y);
		SetDragPosition(SPositionFromLocation(npt, false, false, UserVirtualSpace()));
#if GTK_CHECK_VERSION(3,0,0)
		GdkDragAction preferredAction = gdk_drag_context_get_suggested_action(context);
		GdkDragAction actions = gdk_drag_context_get_actions(context);
#else
		GdkDragAction preferredAction = context->suggested_action;
		GdkDragAction actions = context->actions;
#endif
		SelectionPosition pos = SPositionFromLocation(npt);
		if ((inDragDrop == ddDragging) && (PositionInSelection(pos.Position()))) {
			// Avoid dragging selection onto itself as that produces a move
			// with no real effect but which creates undo actions.
			preferredAction = static_cast<GdkDragAction>(0);
		} else if (actions == static_cast<GdkDragAction>
		        (GDK_ACTION_COPY | GDK_ACTION_MOVE)) {
			preferredAction = GDK_ACTION_MOVE;
		}
		gdk_drag_status(context, preferredAction, dragtime);
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gboolean ScintillaGTK::DragMotion(GtkWidget *widget, GdkDragContext *context,
                                 gint x, gint y, guint dragtime) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	return sciThis->DragMotionThis(context, x, y, dragtime);
}

void ScintillaGTK::DragLeave(GtkWidget *widget, GdkDragContext * /*context*/, guint) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {
		sciThis->SetDragPosition(SelectionPosition(invalidPosition));
		//Platform::DebugPrintf("DragLeave %x\n", sciThis);
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::DragEnd(GtkWidget *widget, GdkDragContext * /*context*/) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {
		// If drag did not result in drop here or elsewhere
		if (!sciThis->dragWasDropped)
			sciThis->SetEmptySelection(sciThis->posDrag);
		sciThis->SetDragPosition(SelectionPosition(invalidPosition));
		//Platform::DebugPrintf("DragEnd %x %d\n", sciThis, sciThis->dragWasDropped);
		sciThis->inDragDrop = ddNone;
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

gboolean ScintillaGTK::Drop(GtkWidget *widget, GdkDragContext * /*context*/,
                            gint, gint, guint) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {
		//Platform::DebugPrintf("Drop %x\n", sciThis);
		sciThis->SetDragPosition(SelectionPosition(invalidPosition));
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

void ScintillaGTK::DragDataReceived(GtkWidget *widget, GdkDragContext * /*context*/,
                                    gint, gint, GtkSelectionData *selection_data, guint /*info*/, guint) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {
		sciThis->ReceivedDrop(selection_data);
		sciThis->SetDragPosition(SelectionPosition(invalidPosition));
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::DragDataGet(GtkWidget *widget, GdkDragContext *context,
                               GtkSelectionData *selection_data, guint info, guint) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {
		sciThis->dragWasDropped = true;
		if (!sciThis->sel.Empty()) {
			sciThis->GetSelection(selection_data, info, &sciThis->drag);
		}
#if GTK_CHECK_VERSION(3,0,0)
		GdkDragAction action = gdk_drag_context_get_selected_action(context);
#else
		GdkDragAction action = context->action;
#endif
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
		sciThis->SetDragPosition(SelectionPosition(invalidPosition));
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

int ScintillaGTK::TimeOut(TimeThunk *tt) {
	tt->scintilla->TickFor(tt->reason);
	return 1;
}

gboolean ScintillaGTK::IdleCallback(ScintillaGTK *sciThis) {
	// Idler will be automatically stopped, if there is nothing
	// to do while idle.
#ifndef GDK_VERSION_3_6
	gdk_threads_enter();
#endif
	bool ret = sciThis->Idle();
	if (ret == false) {
		// FIXME: This will remove the idler from GTK, we don't want to
		// remove it as it is removed automatically when this function
		// returns false (although, it should be harmless).
		sciThis->SetIdle(false);
	}
#ifndef GDK_VERSION_3_6
	gdk_threads_leave();
#endif
	return ret;
}

gboolean ScintillaGTK::StyleIdle(ScintillaGTK *sciThis) {
#ifndef GDK_VERSION_3_6
	gdk_threads_enter();
#endif
	sciThis->IdleWork();
#ifndef GDK_VERSION_3_6
	gdk_threads_leave();
#endif
	// Idler will be automatically stopped
	return FALSE;
}

void ScintillaGTK::QueueIdleWork(WorkNeeded::workItems items, int upTo) {
	Editor::QueueIdleWork(items, upTo);
	if (!workNeeded.active) {
		// Only allow one style needed to be queued
		workNeeded.active = true;
		g_idle_add_full(G_PRIORITY_HIGH_IDLE,
			reinterpret_cast<GSourceFunc>(StyleIdle), this, NULL);
	}
}

void ScintillaGTK::PopUpCB(GtkMenuItem *menuItem, ScintillaGTK *sciThis) {
	guint action = (sptr_t)(g_object_get_data(G_OBJECT(menuItem), "CmdNum"));
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
		Point pt;
		pt.x = int(event->x);
		pt.y = int(event->y);
		sciThis->ct.MouseClick(pt);
		sciThis->CallTipClick();
	} catch (...) {
	}
	return TRUE;
}

#if GTK_CHECK_VERSION(3,0,0)

gboolean ScintillaGTK::DrawCT(GtkWidget *widget, cairo_t *cr, CallTip *ctip) {
	try {
		Surface *surfaceWindow = Surface::Allocate(SC_TECHNOLOGY_DEFAULT);
		if (surfaceWindow) {
			surfaceWindow->Init(cr, widget);
			surfaceWindow->SetUnicodeMode(SC_CP_UTF8 == ctip->codePage);
			surfaceWindow->SetDBCSMode(ctip->codePage);
			ctip->PaintCT(surfaceWindow);
			surfaceWindow->Release();
			delete surfaceWindow;
		}
	} catch (...) {
		// No pointer back to Scintilla to save status
	}
	return TRUE;
}

#else

gboolean ScintillaGTK::ExposeCT(GtkWidget *widget, GdkEventExpose * /*ose*/, CallTip *ctip) {
	try {
		Surface *surfaceWindow = Surface::Allocate(SC_TECHNOLOGY_DEFAULT);
		if (surfaceWindow) {
			cairo_t *cr = gdk_cairo_create(WindowFromWidget(widget));
			surfaceWindow->Init(cr, widget);
			surfaceWindow->SetUnicodeMode(SC_CP_UTF8 == ctip->codePage);
			surfaceWindow->SetDBCSMode(ctip->codePage);
			ctip->PaintCT(surfaceWindow);
			surfaceWindow->Release();
			delete surfaceWindow;
			cairo_destroy(cr);
		}
	} catch (...) {
		// No pointer back to Scintilla to save status
	}
	return TRUE;
}

#endif

sptr_t ScintillaGTK::DirectFunction(
    sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	return reinterpret_cast<ScintillaGTK *>(ptr)->WndProc(iMessage, wParam, lParam);
}

sptr_t scintilla_send_message(ScintillaObject *sci, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	ScintillaGTK *psci = reinterpret_cast<ScintillaGTK *>(sci->pscin);
	return psci->WndProc(iMessage, wParam, lParam);
}

static void scintilla_class_init(ScintillaClass *klass);
static void scintilla_init(ScintillaObject *sci);

extern void Platform_Initialise();
extern void Platform_Finalise();

GType scintilla_get_type() {
	static GType scintilla_type = 0;
	try {

		if (!scintilla_type) {
			scintilla_type = g_type_from_name("Scintilla");
			if (!scintilla_type) {
				static GTypeInfo scintilla_info = {
					(guint16) sizeof (ScintillaClass),
					NULL, //(GBaseInitFunc)
					NULL, //(GBaseFinalizeFunc)
					(GClassInitFunc) scintilla_class_init,
					NULL, //(GClassFinalizeFunc)
					NULL, //gconstpointer data
					(guint16) sizeof (ScintillaObject),
					0, //n_preallocs
					(GInstanceInitFunc) scintilla_init,
					NULL //(GTypeValueTable*)
				};

				scintilla_type = g_type_register_static(
				            GTK_TYPE_CONTAINER, "Scintilla", &scintilla_info, (GTypeFlags) 0);
			}
		}

	} catch (...) {
	}
	return scintilla_type;
}

void ScintillaGTK::ClassInit(OBJECT_CLASS* object_class, GtkWidgetClass *widget_class, GtkContainerClass *container_class) {
	Platform_Initialise();
#ifdef SCI_LEXER
	Scintilla_LinkLexers();
#endif
	atomClipboard = gdk_atom_intern("CLIPBOARD", FALSE);
	atomUTF8 = gdk_atom_intern("UTF8_STRING", FALSE);
	atomString = GDK_SELECTION_TYPE_STRING;
	atomUriList = gdk_atom_intern("text/uri-list", FALSE);
	atomDROPFILES_DND = gdk_atom_intern("DROPFILES_DND", FALSE);

	// Define default signal handlers for the class:  Could move more
	// of the signal handlers here (those that currently attached to wDraw
	// in Initialise() may require coordinate translation?)

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

	container_class->forall = MainForAll;
}

#define SIG_MARSHAL scintilla_marshal_NONE__INT_POINTER
#define MARSHAL_ARGUMENTS G_TYPE_INT, G_TYPE_POINTER

static void scintilla_class_init(ScintillaClass *klass) {
	try {
		OBJECT_CLASS *object_class = (OBJECT_CLASS*) klass;
		GtkWidgetClass *widget_class = (GtkWidgetClass*) klass;
		GtkContainerClass *container_class = (GtkContainerClass*) klass;

		GSignalFlags sigflags = GSignalFlags(G_SIGNAL_ACTION | G_SIGNAL_RUN_LAST);
		scintilla_signals[COMMAND_SIGNAL] = g_signal_new(
		            "command",
		            G_TYPE_FROM_CLASS(object_class),
		            sigflags,
		            G_STRUCT_OFFSET(ScintillaClass, command),
		            NULL, //(GSignalAccumulator)
		            NULL, //(gpointer)
		            SIG_MARSHAL,
		            G_TYPE_NONE,
		            2, MARSHAL_ARGUMENTS);

		scintilla_signals[NOTIFY_SIGNAL] = g_signal_new(
		            SCINTILLA_NOTIFY,
		            G_TYPE_FROM_CLASS(object_class),
		            sigflags,
		            G_STRUCT_OFFSET(ScintillaClass, notify),
		            NULL,
		            NULL,
		            SIG_MARSHAL,
		            G_TYPE_NONE,
		            2, MARSHAL_ARGUMENTS);

		klass->command = NULL;
		klass->notify = NULL;
		scintilla_class_parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
		ScintillaGTK::ClassInit(object_class, widget_class, container_class);
	} catch (...) {
	}
}

static void scintilla_init(ScintillaObject *sci) {
	try {
#if GTK_CHECK_VERSION(2,20,0)
		gtk_widget_set_can_focus(GTK_WIDGET(sci), TRUE);
#else
		GTK_WIDGET_SET_FLAGS(sci, GTK_CAN_FOCUS);
#endif
		sci->pscin = new ScintillaGTK(sci);
	} catch (...) {
	}
}

GtkWidget* scintilla_new() {
	GtkWidget *widget = GTK_WIDGET(g_object_new(scintilla_get_type(), NULL));
	gtk_widget_set_direction(widget, GTK_TEXT_DIR_LTR);

	return widget;
}

void scintilla_set_id(ScintillaObject *sci, uptr_t id) {
	ScintillaGTK *psci = reinterpret_cast<ScintillaGTK *>(sci->pscin);
	psci->ctrlID = id;
}

void scintilla_release_resources(void) {
	try {
		Platform_Finalise();
	} catch (...) {
	}
}
