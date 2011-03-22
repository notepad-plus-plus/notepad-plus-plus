// Scintilla source code edit control
// ScintillaGTK.cxx - GTK+ specific subclass of ScintillaBase
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <new>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>

#include <string>
#include <vector>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "Platform.h"

#if PLAT_GTK_WIN32
#include "windows.h"
#endif

#include "ILexer.h"
#include "Scintilla.h"
#include "ScintillaWidget.h"
#ifdef SCI_LEXER
#include "SciLexer.h"
#endif
#include "SVector.h"
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
#include "AutoComplete.h"
#include "ViewStyle.h"
#include "Decoration.h"
#include "CharClassify.h"
#include "Document.h"
#include "Selection.h"
#include "PositionCache.h"
#include "Editor.h"
#include "ScintillaBase.h"
#include "UniConversion.h"

#include "gtk/gtksignal.h"
#include "gtk/gtkmarshal.h"
#include "scintilla-marshal.h"

#ifdef SCI_LEXER
#include <glib.h>
#include <gmodule.h>
#include "LexerModule.h"
#include "ExternalLexer.h"
#endif

#include "Converter.h"

#if GTK_CHECK_VERSION(2,20,0)
#define IS_WIDGET_REALIZED(w) (gtk_widget_get_realized(GTK_WIDGET(w)))
#define IS_WIDGET_MAPPED(w) (gtk_widget_get_mapped(GTK_WIDGET(w)))
#define IS_WIDGET_VISIBLE(w) (gtk_widget_get_visible(GTK_WIDGET(w)))
#else
#define IS_WIDGET_REALIZED(w) (GTK_WIDGET_REALIZED(w))
#define IS_WIDGET_MAPPED(w) (GTK_WIDGET_MAPPED(w))
#define IS_WIDGET_VISIBLE(w) (GTK_WIDGET_VISIBLE(w))
#endif

#if GTK_CHECK_VERSION(2,22,0)
#define USE_CAIRO 1
#endif

#ifdef _MSC_VER
// Constant conditional expressions are because of GTK+ headers
#pragma warning(disable: 4127)
// Ignore unreferenced local functions in GTK+ headers
#pragma warning(disable: 4505)
#endif

#if GTK_CHECK_VERSION(2,6,0)
#define USE_GTK_CLIPBOARD
#endif

#define OBJECT_CLASS GObjectClass

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

extern char *UTF8FromLatin1(const char *s, int &len);

class ScintillaGTK : public ScintillaBase {
	_ScintillaObject *sci;
	Window wText;
	Window scrollbarv;
	Window scrollbarh;
	GtkObject *adjustmentv;
	GtkObject *adjustmenth;
	int scrollBarWidth;
	int scrollBarHeight;

	// Because clipboard access is asynchronous, copyText is created by Copy
#ifndef USE_GTK_CLIPBOARD
	SelectionText copyText;
#endif

	SelectionText primary;

	GdkEventButton evbtn;
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

	// Wheel mouse support
	unsigned int linesPerScroll;
	GTimeVal lastWheelMouseTime;
	gint lastWheelMouseDirection;
	gint wheelMouseIntensity;

	GdkRegion *rgnUpdate;

	// Private so ScintillaGTK objects can not be copied
	ScintillaGTK(const ScintillaGTK &);
	ScintillaGTK &operator=(const ScintillaGTK &);

public:
	ScintillaGTK(_ScintillaObject *sci_);
	virtual ~ScintillaGTK();
	static void ClassInit(OBJECT_CLASS* object_class, GtkWidgetClass *widget_class, GtkContainerClass *container_class);
private:
	virtual void Initialise();
	virtual void Finalise();
	virtual void DisplayCursor(Window::Cursor c);
	virtual bool DragThreshold(Point ptStart, Point ptNow);
	virtual void StartDrag();
	int TargetAsUTF8(char *text);
	int EncodedFromUTF8(char *utf8, char *encoded);
	virtual bool ValidCodePage(int codePage) const;
public: 	// Public for scintilla_send_message
	virtual sptr_t WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
private:
	virtual sptr_t DefWndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	virtual void SetTicking(bool on);
	virtual bool SetIdle(bool on);
	virtual void SetMouseCapture(bool on);
	virtual bool HaveMouseCapture();
	virtual bool PaintContains(PRectangle rc);
	void FullPaint();
	virtual PRectangle GetClientRectangle();
	void SyncPaint(PRectangle rc);
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
#ifdef USE_GTK_CLIPBOARD
	void StoreOnClipboard(SelectionText *clipText);
	static void ClipboardGetSelection(GtkClipboard* clip, GtkSelectionData *selection_data, guint info, void *data);
	static void ClipboardClearSelection(GtkClipboard* clip, void *data);
#endif

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
	static void SizeAllocate(GtkWidget *widget, GtkAllocation *allocation);
	gint Expose(GtkWidget *widget, GdkEventExpose *ose);
	static gint ExposeMain(GtkWidget *widget, GdkEventExpose *ose);
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
	gboolean ExposePreeditThis(GtkWidget *widget, GdkEventExpose *ose);
	static gboolean ExposePreedit(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis);
	void CommitThis(char *str);
	static void Commit(GtkIMContext *context, char *str, ScintillaGTK *sciThis);
	void PreeditChangedThis();
	static void PreeditChanged(GtkIMContext *context, ScintillaGTK *sciThis);
	static gint StyleSetText(GtkWidget *widget, GtkStyle *previous, void*);
	static gint RealizeText(GtkWidget *widget, void*);
	static void Destroy(GObject *object);
	static void SelectionReceived(GtkWidget *widget, GtkSelectionData *selection_data,
	                              guint time);
	static void SelectionGet(GtkWidget *widget, GtkSelectionData *selection_data,
	                         guint info, guint time);
	static gint SelectionClear(GtkWidget *widget, GdkEventSelection *selection_event);
	static void DragBegin(GtkWidget *widget, GdkDragContext *context);
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
	static gint TimeOut(ScintillaGTK *sciThis);
	static gboolean IdleCallback(ScintillaGTK *sciThis);
	static gboolean StyleIdle(ScintillaGTK *sciThis);
	virtual void QueueStyling(int upTo);
	static void PopUpCB(GtkMenuItem *menuItem, ScintillaGTK *sciThis);

	gint ExposeTextThis(GtkWidget *widget, GdkEventExpose *ose);
	static gint ExposeText(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis);

	static gint ExposeCT(GtkWidget *widget, GdkEventExpose *ose, CallTip *ct);
	static gint PressCT(GtkWidget *widget, GdkEventButton *event, ScintillaGTK *sciThis);

	static sptr_t DirectFunction(ScintillaGTK *sciThis,
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
static const gint nClipboardCopyTargets = sizeof(clipboardCopyTargets) / sizeof(clipboardCopyTargets[0]);

static const GtkTargetEntry clipboardPasteTargets[] = {
	{ (gchar *) "text/uri-list", 0, TARGET_URI },
	{ (gchar *) "UTF8_STRING", 0, TARGET_UTF8_STRING },
	{ (gchar *) "STRING", 0, TARGET_STRING },
};
static const gint nClipboardPasteTargets = sizeof(clipboardPasteTargets) / sizeof(clipboardPasteTargets[0]);

static GtkWidget *PWidget(Window &w) {
	return reinterpret_cast<GtkWidget *>(w.GetID());
}

static ScintillaGTK *ScintillaFromWidget(GtkWidget *widget) {
	ScintillaObject *scio = reinterpret_cast<ScintillaObject *>(widget);
	return reinterpret_cast<ScintillaGTK *>(scio->pscin);
}

ScintillaGTK::ScintillaGTK(_ScintillaObject *sci_) :
		adjustmentv(0), adjustmenth(0),
		scrollBarWidth(30), scrollBarHeight(30),
		capturedMouse(false), dragWasDropped(false),
		lastKey(0), rectangularSelectionModifier(SCMOD_CTRL), parentClass(0),
		im_context(NULL),
		lastWheelMouseDirection(0),
		wheelMouseIntensity(0),
		rgnUpdate(0) {
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
	attrs.x = widget->allocation.x;
	attrs.y = widget->allocation.y;
	attrs.width = widget->allocation.width;
	attrs.height = widget->allocation.height;
	attrs.wclass = GDK_INPUT_OUTPUT;
	attrs.visual = gtk_widget_get_visual(widget);
	attrs.colormap = gtk_widget_get_colormap(widget);
	attrs.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;
	GdkCursor *cursor = gdk_cursor_new(GDK_XTERM);
	attrs.cursor = cursor;
	widget->window = gdk_window_new(gtk_widget_get_parent_window(widget), &attrs,
		GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP | GDK_WA_CURSOR);
	gdk_window_set_user_data(widget->window, widget);
	gdk_window_set_background(widget->window, &widget->style->bg[GTK_STATE_NORMAL]);
	gdk_window_show(widget->window);
	gdk_cursor_unref(cursor);
	widget->style = gtk_style_attach(widget->style, widget->window);
	wPreedit = gtk_window_new(GTK_WINDOW_POPUP);
	wPreeditDraw = gtk_drawing_area_new();
	GtkWidget *predrw = PWidget(wPreeditDraw);	// No code inside the G_OBJECT macro
	g_signal_connect(G_OBJECT(predrw), "expose_event",
		G_CALLBACK(ExposePreedit), this);
	gtk_container_add(GTK_CONTAINER(PWidget(wPreedit)), predrw);
	gtk_widget_realize(PWidget(wPreedit));
	gtk_widget_realize(predrw);
	gtk_widget_show(predrw);

	im_context = gtk_im_multicontext_new();
	g_signal_connect(G_OBJECT(im_context), "commit",
		G_CALLBACK(Commit), this);
	g_signal_connect(G_OBJECT(im_context), "preedit_changed",
		G_CALLBACK(PreeditChanged), this);
	gtk_im_context_set_client_window(im_context, widget->window);
	GtkWidget *widtxt = PWidget(wText);	//	// No code inside the G_OBJECT macro
	g_signal_connect_after(G_OBJECT(widtxt), "style_set",
		G_CALLBACK(ScintillaGTK::StyleSetText), NULL);
	g_signal_connect_after(G_OBJECT(widtxt), "realize",
		G_CALLBACK(ScintillaGTK::RealizeText), NULL);
	gtk_widget_realize(widtxt);
	gtk_widget_realize(PWidget(scrollbarv));
	gtk_widget_realize(PWidget(scrollbarh));
}

void ScintillaGTK::Realize(GtkWidget *widget) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	sciThis->RealizeThis(widget);
}

void ScintillaGTK::UnRealizeThis(GtkWidget *widget) {
	try {
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
		gdk_window_show(PWidget(wMain)->window);
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
		DropGraphics();
		gdk_window_hide(PWidget(wMain)->window);
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

gint ScintillaGTK::FocusInThis(GtkWidget *widget) {
	try {
		SetFocusState(true);
		if (im_context != NULL) {
			gchar *str = NULL;
			gint cursor_pos;

			gtk_im_context_get_preedit_string(im_context, &str, NULL, &cursor_pos);
			if (PWidget(wPreedit) != NULL) {
				if (strlen(str) > 0) {
					gtk_widget_show(PWidget(wPreedit));
				} else {
					gtk_widget_hide(PWidget(wPreedit));
				}
			}
			g_free(str);
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
	requisition->width = 600;
	requisition->height = gdk_screen_height();
	GtkRequisition child_requisition;
	gtk_widget_size_request(PWidget(sciThis->scrollbarh), &child_requisition);
	gtk_widget_size_request(PWidget(sciThis->scrollbarv), &child_requisition);
}

void ScintillaGTK::SizeAllocate(GtkWidget *widget, GtkAllocation *allocation) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {
		widget->allocation = *allocation;
		if (IS_WIDGET_REALIZED(widget))
			gdk_window_move_resize(widget->window,
			        widget->allocation.x,
			        widget->allocation.y,
			        widget->allocation.width,
			        widget->allocation.height);

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
	g_signal_connect(G_OBJECT(widtxt), "expose_event",
			   G_CALLBACK(ScintillaGTK::ExposeText), this);
	gtk_widget_set_events(widtxt, GDK_EXPOSURE_MASK);
	// Avoid background drawing flash
	gtk_widget_set_double_buffered(widtxt, FALSE);
	gtk_widget_set_size_request(widtxt, 100, 100);
	adjustmentv = gtk_adjustment_new(0.0, 0.0, 201.0, 1.0, 20.0, 20.0);
	scrollbarv = gtk_vscrollbar_new(GTK_ADJUSTMENT(adjustmentv));
#if GTK_CHECK_VERSION(2,20,0)
	gtk_widget_set_can_focus(PWidget(scrollbarv), FALSE);
#else
	GTK_WIDGET_UNSET_FLAGS(PWidget(scrollbarv), GTK_CAN_FOCUS);
#endif
	g_signal_connect(G_OBJECT(adjustmentv), "value_changed",
			   G_CALLBACK(ScrollSignal), this);
	gtk_widget_set_parent(PWidget(scrollbarv), PWidget(wMain));
	gtk_widget_show(PWidget(scrollbarv));

	adjustmenth = gtk_adjustment_new(0.0, 0.0, 101.0, 1.0, 20.0, 20.0);
	scrollbarh = gtk_hscrollbar_new(GTK_ADJUSTMENT(adjustmenth));
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

	gtk_selection_add_targets(GTK_WIDGET(PWidget(wMain)), GDK_SELECTION_PRIMARY,
	                          clipboardCopyTargets, nClipboardCopyTargets);

#ifndef USE_GTK_CLIPBOARD
	gtk_selection_add_targets(GTK_WIDGET(PWidget(wMain)), atomClipboard,
	                          clipboardPasteTargets, nClipboardPasteTargets);
#endif

	gtk_drag_dest_set(GTK_WIDGET(PWidget(wMain)),
	                  GTK_DEST_DEFAULT_ALL, clipboardPasteTargets, nClipboardPasteTargets,
	                  static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE));

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

	SetTicking(true);
}

void ScintillaGTK::Finalise() {
	SetTicking(false);
	ScintillaBase::Finalise();
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
	dragWasDropped = false;
	inDragDrop = ddDragging;
	GtkTargetList *tl = gtk_target_list_new(clipboardCopyTargets, nClipboardCopyTargets);
	gtk_drag_begin(GTK_WIDGET(PWidget(wMain)),
	               tl,
	               static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE),
	               evbtn.button,
	               reinterpret_cast<GdkEvent *>(&evbtn));
}

static char *ConvertText(int *lenResult, char *s, size_t len, const char *charSetDest,
	const char *charSetSource, bool transliterations, bool silent=false) {
	// s is not const because of different versions of iconv disagreeing about const
	*lenResult = 0;
	char *destForm = 0;
	Converter conv(charSetDest, charSetSource, transliterations);
	if (conv) {
		destForm = new char[len*3+1];
		char *pin = s;
		size_t inLeft = len;
		char *pout = destForm;
		size_t outLeft = len*3+1;
		size_t conversions = conv.Convert(&pin, &inLeft, &pout, &outLeft);
		if (conversions == ((size_t)(-1))) {
			if (!silent)
				fprintf(stderr, "iconv %s->%s failed for %s\n",
					charSetSource, charSetDest, static_cast<char *>(s));
			delete []destForm;
			destForm = 0;
		} else {
//fprintf(stderr, "iconv OK %s %d\n", destForm, pout - destForm);
			*pout = '\0';
			*lenResult = pout - destForm;
		}
	} else {
fprintf(stderr, "Can not iconv %s %s\n", charSetDest, charSetSource);
	}
	if (!destForm) {
		destForm = new char[1];
		destForm[0] = '\0';
		*lenResult = 0;
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
//~ fprintf(stderr, "AsUTF8 %s %d  %0d-%0d\n", charSetBuffer, targetLength, targetStart, targetEnd);
			char *s = new char[targetLength];
			if (s) {
				pdoc->GetCharRange(s, targetStart, targetLength);
//~ fprintf(stderr, "    \"%s\"\n", s);
				if (text) {
					char *tmputf = ConvertText(&targetLength, s, targetLength, "UTF-8", charSetBuffer, false);
					memcpy(text, tmputf, targetLength);
					delete []tmputf;
//~ fprintf(stderr, "    \"%s\"\n", text);
				}
				delete []s;
			}
		} else {
			if (text) {
				pdoc->GetCharRange(text, targetStart, targetLength);
			}
		}
	}
//~ fprintf(stderr, "Length = %d bytes\n", targetLength);
	return targetLength;
}

// Translates a nul terminated UTF8 string into the document encoding.
// Return the length of the result in bytes.
int ScintillaGTK::EncodedFromUTF8(char *utf8, char *encoded) {
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
			int outLength = 0;
			char *tmpEncoded = ConvertText(&outLength, utf8, inputLength, charSetBuffer, "UTF-8", true);
			if (tmpEncoded) {
				if (encoded) {
					memcpy(encoded, tmpEncoded, outLength);
				}
				delete []tmpEncoded;
			}
			return outLength;
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

void ScintillaGTK::SetTicking(bool on) {
	if (timer.ticking != on) {
		timer.ticking = on;
		if (timer.ticking) {
			timer.tickerID = reinterpret_cast<TickerID>(g_timeout_add(timer.tickSize, (GtkFunction)TimeOut, this));
		} else {
			g_source_remove(GPOINTER_TO_UINT(timer.tickerID));
		}
	}
	timer.ticksToWait = caret.period;
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

bool ScintillaGTK::PaintContains(PRectangle rc) {
	bool contains = true;
	if (paintState == painting) {
		if (!rcPaint.Contains(rc)) {
			contains = false;
		} else if (rgnUpdate) {
			GdkRectangle grc = {rc.left, rc.top,
				rc.right - rc.left, rc.bottom - rc.top};
			if (gdk_region_rect_in(rgnUpdate, &grc) != GDK_OVERLAP_RECTANGLE_IN) {
				contains = false;
			}
		}
	}
	return contains;
}

// Redraw all of text area. This paint will not be abandoned.
void ScintillaGTK::FullPaint() {
	wText.InvalidateAll();
}

PRectangle ScintillaGTK::GetClientRectangle() {
	PRectangle rc = wMain.GetClientPosition();
	if (verticalScrollBarVisible)
		rc.right -= scrollBarWidth;
	if (horizontalScrollBarVisible && (wrapState == eWrapNone))
		rc.bottom -= scrollBarHeight;
	// Move to origin
	rc.right -= rc.left;
	rc.bottom -= rc.top;
	rc.left = 0;
	rc.top = 0;
	return rc;
}

// Synchronously paint a rectangle of the window.
void ScintillaGTK::SyncPaint(PRectangle rc) {
	paintState = painting;
	rcPaint = rc;
	PRectangle rcClient = GetClientRectangle();
	paintingAllText = rcPaint.Contains(rcClient);
	if ((PWidget(wText))->window) {
		Surface *sw = Surface::Allocate();
		if (sw) {
			sw->Init(PWidget(wText)->window, PWidget(wText));
			Paint(sw, rc);
			sw->Release();
			delete sw;
		}
	}
	if (paintState == paintAbandoned) {
		// Painting area was insufficient to cover new styling or brace highlight positions
		FullPaint();
	}
	paintState = notPainting;
}

void ScintillaGTK::ScrollText(int linesToMove) {
	int diff = vs.lineHeight * -linesToMove;
	//Platform::DebugPrintf("ScintillaGTK::ScrollText %d %d %0d,%0d %0d,%0d\n", linesToMove, diff,
	//	rc.left, rc.top, rc.right, rc.bottom);
	GtkWidget *wi = PWidget(wText);

	gdk_window_scroll(wi->window, 0, -diff);
	gdk_window_process_updates(wi->window, FALSE);
}

void ScintillaGTK::SetVerticalScrollPos() {
	DwellEnd(true);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustmentv), topLine);
}

void ScintillaGTK::SetHorizontalScrollPos() {
	DwellEnd(true);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustmenth), xOffset / 2);
}

bool ScintillaGTK::ModifyScrollBars(int nMax, int nPage) {
	bool modified = false;
	int pageScroll = LinesToScroll();

	if (GTK_ADJUSTMENT(adjustmentv)->upper != (nMax + 1) ||
	        GTK_ADJUSTMENT(adjustmentv)->page_size != nPage ||
	        GTK_ADJUSTMENT(adjustmentv)->page_increment != pageScroll) {
		GTK_ADJUSTMENT(adjustmentv)->upper = nMax + 1;
		GTK_ADJUSTMENT(adjustmentv)->page_size = nPage;
		GTK_ADJUSTMENT(adjustmentv)->page_increment = pageScroll;
		gtk_adjustment_changed(GTK_ADJUSTMENT(adjustmentv));
		modified = true;
	}

	PRectangle rcText = GetTextRectangle();
	int horizEndPreferred = scrollWidth;
	if (horizEndPreferred < 0)
		horizEndPreferred = 0;
	unsigned int pageWidth = rcText.Width();
	unsigned int pageIncrement = pageWidth / 3;
	unsigned int charWidth = vs.styles[STYLE_DEFAULT].aveCharWidth;
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
}

void ScintillaGTK::NotifyParent(SCNotification scn) {
	scn.nmhdr.hwndFrom = PWidget(wMain);
	scn.nmhdr.idFrom = GetCtrlID();
	g_signal_emit(G_OBJECT(sci), scintilla_signals[NOTIFY_SIGNAL], 0,
	                GetCtrlID(), &scn);
}

void ScintillaGTK::NotifyKey(int key, int modifiers) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_KEY;
	scn.ch = key;
	scn.modifiers = modifiers;

	NotifyParent(scn);
}

void ScintillaGTK::NotifyURIDropped(const char *list) {
	SCNotification scn = {0};
	scn.nmhdr.code = SCN_URIDROPPED;
	scn.text = list;

	NotifyParent(scn);
}

const char *CharacterSetID(int characterSet);

const char *ScintillaGTK::CharacterSetID() const {
	return ::CharacterSetID(vs.styles[STYLE_DEFAULT].characterSet);
}

class CaseFolderUTF8 : public CaseFolderTable {
public:
	CaseFolderUTF8() {
		StandardASCII();
	}
	virtual size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) {
		if ((lenMixed == 1) && (sizeFolded > 0)) {
			folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
			return 1;
		} else {
			gchar *mapped = g_utf8_casefold(mixed, lenMixed);
			size_t lenMapped = strlen(mapped);
			if (lenMapped < sizeFolded) {
				memcpy(folded, mapped,  lenMapped);
			} else {
				lenMapped = 0;
			}
			g_free(mapped);
			return lenMapped;
		}
	}
};

class CaseFolderDBCS : public CaseFolderTable {
	const char *charSet;
public:
	CaseFolderDBCS(const char *charSet_) : charSet(charSet_) {
		StandardASCII();
	}
	virtual size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) {
		if ((lenMixed == 1) && (sizeFolded > 0)) {
			folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
			return 1;
		} else if (*charSet) {
			int convertedLength = lenMixed;
			char *sUTF8 = ConvertText(&convertedLength, const_cast<char *>(mixed), lenMixed,
				"UTF-8", charSet, false);
			if (sUTF8) {
				gchar *mapped = g_utf8_casefold(sUTF8, strlen(sUTF8));
				size_t lenMapped = strlen(mapped);
				if (lenMapped < sizeFolded) {
					memcpy(folded, mapped,  lenMapped);
				} else {
					folded[0] = '\0';
					lenMapped = 1;
				}
				g_free(mapped);
				delete []sUTF8;
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
		return new CaseFolderUTF8();
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
					int convertedLength = 1;
					const char *sUTF8 = ConvertText(&convertedLength, sCharacter, 1,
						"UTF-8", charSetBuffer, false);
					if (sUTF8) {
						gchar *mapped = g_utf8_casefold(sUTF8, strlen(sUTF8));
						if (mapped) {
							int mappedLength = strlen(mapped);
							const char *mappedBack = ConvertText(&mappedLength, mapped,
								mappedLength, charSetBuffer, "UTF-8", false, true);
							if (mappedBack && (strlen(mappedBack) == 1) && (mappedBack[0] != sCharacter[0])) {
								pcf->SetTranslation(sCharacter[0], mappedBack[0]);
							}
							delete []mappedBack;
							g_free(mapped);
						}
					}
					delete []sUTF8;
				}
				return pcf;
			} else {
				return new CaseFolderDBCS(charSetBuffer);
			}
		}
		return 0;
	}
}

std::string ScintillaGTK::CaseMapString(const std::string &s, int caseMapping) {
	if (s.size() == 0)
		return std::string();

	if (caseMapping == cmSame)
		return s;

	const char *needsFree1 = 0;	// Must be freed with delete []
	const char *charSetBuffer = CharacterSetID();
	const char *sUTF8 = s.c_str();
	int rangeBytes = s.size();

	int convertedLength = rangeBytes;
	// Change text to UTF-8
	if (!IsUnicodeMode()) {
		// Need to convert
		if (*charSetBuffer) {
			sUTF8 = ConvertText(&convertedLength, const_cast<char *>(s.c_str()), rangeBytes,
				"UTF-8", charSetBuffer, false);
			needsFree1 = sUTF8;
		}
	}
	gchar *mapped;	// Must be freed with g_free
	if (caseMapping == cmUpper) {
		mapped = g_utf8_strup(sUTF8, convertedLength);
	} else {
		mapped = g_utf8_strdown(sUTF8, convertedLength);
	}
	int mappedLength = strlen(mapped);
	char *mappedBack = mapped;

	char *needsFree2 = 0;	// Must be freed with delete []
	if (!IsUnicodeMode()) {
		if (*charSetBuffer) {
			mappedBack = ConvertText(&mappedLength, mapped, mappedLength, charSetBuffer, "UTF-8", false);
			needsFree2 = mappedBack;
		}
	}

	std::string ret(mappedBack, mappedLength);
	g_free(mapped);
	delete []needsFree1;
	delete []needsFree2;
	return ret;
}

int ScintillaGTK::KeyDefault(int key, int modifiers) {
	if (!(modifiers & SCI_CTRL) && !(modifiers & SCI_ALT)) {
		if (key < 256) {
			NotifyKey(key, modifiers);
			return 0;
		} else {
			// Pass up to container in case it is an accelerator
			NotifyKey(key, modifiers);
			return 0;
		}
	} else {
		// Pass up to container in case it is an accelerator
		NotifyKey(key, modifiers);
		return 0;
	}
	//Platform::DebugPrintf("SK-key: %d %x %x\n",key, modifiers);
}

void ScintillaGTK::CopyToClipboard(const SelectionText &selectedText) {
#ifndef USE_GTK_CLIPBOARD
	copyText.Copy(selectedText);
	gtk_selection_owner_set(GTK_WIDGET(PWidget(wMain)),
				atomClipboard,
				GDK_CURRENT_TIME);
#else
	SelectionText *clipText = new SelectionText();
	clipText->Copy(selectedText);
	StoreOnClipboard(clipText);
#endif
}

void ScintillaGTK::Copy() {
	if (!sel.Empty()) {
#ifndef USE_GTK_CLIPBOARD
		CopySelectionRange(&copyText);
		gtk_selection_owner_set(GTK_WIDGET(PWidget(wMain)),
		                        atomClipboard,
		                        GDK_CURRENT_TIME);
#else
		SelectionText *clipText = new SelectionText();
		CopySelectionRange(clipText);
		StoreOnClipboard(clipText);
#endif
#if PLAT_GTK_WIN32
		if (sel.IsRectangular()) {
			::OpenClipboard(NULL);
			::SetClipboardData(cfColumnSelect, 0);
			::CloseClipboard();
		}
#endif
	}
}

void ScintillaGTK::Paste() {
	atomSought = atomUTF8;
	gtk_selection_convert(GTK_WIDGET(PWidget(wMain)),
	                      atomClipboard, atomSought, GDK_CURRENT_TIME);
}

void ScintillaGTK::CreateCallTipWindow(PRectangle rc) {
	if (!ct.wCallTip.Created()) {
		ct.wCallTip = gtk_window_new(GTK_WINDOW_POPUP);
		ct.wDraw = gtk_drawing_area_new();
		GtkWidget *widcdrw = PWidget(ct.wDraw);	//	// No code inside the G_OBJECT macro
		gtk_container_add(GTK_CONTAINER(PWidget(ct.wCallTip)), widcdrw);
		g_signal_connect(G_OBJECT(widcdrw), "expose_event",
				   G_CALLBACK(ScintillaGTK::ExposeCT), &ct);
		g_signal_connect(G_OBJECT(widcdrw), "button_press_event",
				   G_CALLBACK(ScintillaGTK::PressCT), static_cast<void *>(this));
		gtk_widget_set_events(widcdrw,
			GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
	}
	gtk_widget_set_size_request(PWidget(ct.wDraw), rc.Width(), rc.Height());
	ct.wDraw.Show();
	if (PWidget(ct.wCallTip)->window) {
		gdk_window_resize(PWidget(ct.wCallTip)->window, rc.Width(), rc.Height());
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
		== GTK_WIDGET(PWidget(wMain))->window) &&
			(GTK_WIDGET(PWidget(wMain))->window != NULL));
}

void ScintillaGTK::ClaimSelection() {
	// X Windows has a 'primary selection' as well as the clipboard.
	// Whenever the user selects some text, we become the primary selection
	if (!sel.Empty() && IS_WIDGET_REALIZED(GTK_WIDGET(PWidget(wMain)))) {
		primarySelection = true;
		gtk_selection_owner_set(GTK_WIDGET(PWidget(wMain)),
		                        GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
		primary.Free();
	} else if (OwnPrimarySelection()) {
		primarySelection = true;
		if (primary.s == NULL)
			gtk_selection_owner_set(NULL, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
	} else {
		primarySelection = false;
		primary.Free();
	}
}

// Detect rectangular text, convert line ends to current mode, convert from or to UTF-8
void ScintillaGTK::GetGtkSelectionText(GtkSelectionData *selectionData, SelectionText &selText) {
	char *data = reinterpret_cast<char *>(selectionData->data);
	int len = selectionData->length;
	GdkAtom selectionTypeData = selectionData->type;

	// Return empty string if selection is not a string
	if ((selectionTypeData != GDK_TARGET_STRING) && (selectionTypeData != atomUTF8)) {
		char *empty = new char[1];
		empty[0] = '\0';
		selText.Set(empty, 0, SC_CP_UTF8, 0, false, false);
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

	char *dest;
	if (selectionTypeData == GDK_TARGET_STRING) {
		dest = Document::TransformLineEnds(&len, data, len, pdoc->eolMode);
		if (IsUnicodeMode()) {
			// Unknown encoding so assume in Latin1
			char *destPrevious = dest;
			dest = UTF8FromLatin1(dest, len);
			selText.Set(dest, len, SC_CP_UTF8, 0, selText.rectangular, false);
			delete []destPrevious;
		} else {
			// Assume buffer is in same encoding as selection
			selText.Set(dest, len, pdoc->dbcsCodePage,
				vs.styles[STYLE_DEFAULT].characterSet, isRectangular, false);
		}
	} else {	// UTF-8
		dest = Document::TransformLineEnds(&len, data, len, pdoc->eolMode);
		selText.Set(dest, len, SC_CP_UTF8, 0, isRectangular, false);
		const char *charSetBuffer = CharacterSetID();
		if (!IsUnicodeMode() && *charSetBuffer) {
			// Convert to locale
			dest = ConvertText(&len, selText.s, selText.len, charSetBuffer, "UTF-8", true);
			selText.Set(dest, len, pdoc->dbcsCodePage,
				vs.styles[STYLE_DEFAULT].characterSet, selText.rectangular, false);
		}
	}
}

void ScintillaGTK::ReceivedSelection(GtkSelectionData *selection_data) {
	try {
		if ((selection_data->selection == atomClipboard) ||
		        (selection_data->selection == GDK_SELECTION_PRIMARY)) {
			if ((atomSought == atomUTF8) && (selection_data->length <= 0)) {
				atomSought = atomString;
				gtk_selection_convert(GTK_WIDGET(PWidget(wMain)),
				        selection_data->selection, atomSought, GDK_CURRENT_TIME);
			} else if ((selection_data->length > 0) &&
			        ((selection_data->type == GDK_TARGET_STRING) || (selection_data->type == atomUTF8))) {
				SelectionText selText;
				GetGtkSelectionText(selection_data, selText);

				UndoGroup ug(pdoc);
				if (selection_data->selection != GDK_SELECTION_PRIMARY) {
					ClearSelection(multiPasteMode == SC_MULTIPASTE_EACH);
				}
				SelectionPosition selStart = sel.IsRectangular() ?
					sel.Rectangular().Start() :
					sel.Range(sel.Main()).Start();

				if (selText.rectangular) {
					PasteRectangular(selStart, selText.s, selText.len);
				} else {
					InsertPaste(selStart, selText.s, selText.len);
				}
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
	if (selection_data->type == atomUriList || selection_data->type == atomDROPFILES_DND) {
		char *ptr = new char[selection_data->length + 1];
		ptr[selection_data->length] = '\0';
		memcpy(ptr, selection_data->data, selection_data->length);
 		NotifyURIDropped(ptr);
		delete []ptr;
	} else if ((selection_data->type == GDK_TARGET_STRING) || (selection_data->type == atomUTF8)) {
		if (selection_data->length > 0) {
			SelectionText selText;
			GetGtkSelectionText(selection_data, selText);
			DropAt(posDrop, selText.s, false, selText.rectangular);
		}
	} else if (selection_data->length > 0) {
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
		int tmpstr_len;
		char *tmpstr = Document::TransformLineEnds(&tmpstr_len, text->s, text->len, SC_EOL_LF);
		newline_normalized = new SelectionText();
		newline_normalized->Set(tmpstr, tmpstr_len, SC_CP_UTF8, 0, text->rectangular, false);
		text = newline_normalized;
	}
#endif

	// Convert text to utf8 if it isn't already
	SelectionText *converted = 0;
	if ((text->codePage != SC_CP_UTF8) && (info == TARGET_UTF8_STRING)) {
		const char *charSet = ::CharacterSetID(text->characterSet);
		if (*charSet) {
			int new_len;
			char* tmputf = ConvertText(&new_len, text->s, text->len, "UTF-8", charSet, false);
			converted = new SelectionText();
			converted->Set(tmputf, new_len, SC_CP_UTF8, 0, text->rectangular, false);
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
	const char *textData = text->s ? text->s : "";
	int len = strlen(textData);
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

#ifdef USE_GTK_CLIPBOARD
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
#endif

void ScintillaGTK::UnclaimSelection(GdkEventSelection *selection_event) {
	try {
		//Platform::DebugPrintf("UnclaimSelection\n");
		if (selection_event->selection == GDK_SELECTION_PRIMARY) {
			//Platform::DebugPrintf("UnclaimPrimarySelection\n");
			if (!OwnPrimarySelection()) {
				primary.Free();
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
	scrollBarWidth = GTK_WIDGET(PWidget(scrollbarv))->requisition.width;
	scrollBarHeight = GTK_WIDGET(PWidget(scrollbarh))->requisition.height;

	// These allocations should never produce negative sizes as they would wrap around to huge
	// unsigned numbers inside GTK+ causing warnings.
	bool showSBHorizontal = horizontalScrollBarVisible && (wrapState == eWrapNone);
	int horizontalScrollBarHeight = scrollBarHeight;
	if (!showSBHorizontal)
		horizontalScrollBarHeight = 0;

	GtkAllocation alloc;
	if (showSBHorizontal) {
		gtk_widget_show(GTK_WIDGET(PWidget(scrollbarh)));
		alloc.x = 0;
		alloc.y = height - scrollBarHeight;
		alloc.width = Platform::Maximum(1, width - scrollBarWidth) + 1;
		alloc.height = horizontalScrollBarHeight;
		gtk_widget_size_allocate(GTK_WIDGET(PWidget(scrollbarh)), &alloc);
	} else {
		gtk_widget_hide(GTK_WIDGET(PWidget(scrollbarh)));
	}

	if (verticalScrollBarVisible) {
		gtk_widget_show(GTK_WIDGET(PWidget(scrollbarv)));
		alloc.x = width - scrollBarWidth;
		alloc.y = 0;
		alloc.width = scrollBarWidth;
		alloc.height = Platform::Maximum(1, height - scrollBarHeight) + 1;
		if (!showSBHorizontal)
			alloc.height += scrollBarWidth-1;
		gtk_widget_size_allocate(GTK_WIDGET(PWidget(scrollbarv)), &alloc);
	} else {
		gtk_widget_hide(GTK_WIDGET(PWidget(scrollbarv)));
	}
	if (IS_WIDGET_MAPPED(PWidget(wMain))) {
		ChangeSize();
	}

	alloc.x = 0;
	alloc.y = 0;
	alloc.width = Platform::Maximum(1, width - scrollBarWidth);
	alloc.height = Platform::Maximum(1, height - scrollBarHeight);
	if (!showSBHorizontal)
		alloc.height += scrollBarHeight;
	if (!verticalScrollBarVisible)
		alloc.width += scrollBarWidth;
	gtk_widget_size_allocate(GTK_WIDGET(PWidget(wText)), &alloc);
}

static void SetAdjustmentValue(GtkObject *object, int value) {
	GtkAdjustment *adjustment = GTK_ADJUSTMENT(object);
	int maxValue = static_cast<int>(
		adjustment->upper - adjustment->page_size);
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

		evbtn = *event;
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

		bool ctrl = (event->state & GDK_CONTROL_MASK) != 0;

		gtk_widget_grab_focus(PWidget(wMain));
		if (event->button == 1) {
			// On X, instead of sending literal modifiers use the user specified
			// modifier, defaulting to control instead of alt.
			// This is because most X window managers grab alt + click for moving
			ButtonDown(pt, event->time,
			        (event->state & GDK_SHIFT_MASK) != 0,
			        (event->state & GDK_CONTROL_MASK) != 0,
			        (event->state & modifierTranslated(rectangularSelectionModifier)) != 0);
		} else if (event->button == 2) {
			// Grab the primary selection if it exists
			SelectionPosition pos = SPositionFromLocation(pt, false, false, UserVirtualSpace());
			if (OwnPrimarySelection() && primary.s == NULL)
				CopySelectionRange(&primary);

			sel.Clear();
			SetSelection(pos, pos);
			atomSought = atomUTF8;
			gtk_selection_convert(GTK_WIDGET(PWidget(wMain)), GDK_SELECTION_PRIMARY,
			        atomSought, event->time);
		} else if (event->button == 3) {
			if (displayPopupMenu) {
				// PopUp menu
				// Convert to screen
				int ox = 0;
				int oy = 0;
				gdk_window_get_origin(PWidget(wMain)->window, &ox, &oy);
				ContextMenu(Point(pt.x + ox, pt.y + oy));
			} else {
				return FALSE;
			}
		} else if (event->button == 4) {
			// Wheel scrolling up (only GTK 1.x does it this way)
			if (ctrl)
				SetAdjustmentValue(adjustmenth, (xOffset / 2) - 6);
			else
				SetAdjustmentValue(adjustmentv, topLine - 3);
		} else if (event->button == 5) {
			// Wheel scrolling down (only GTK 1.x does it this way)
			if (ctrl)
				SetAdjustmentValue(adjustmenth, (xOffset / 2) + 6);
			else
				SetAdjustmentValue(adjustmentv, topLine + 3);
		}
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return TRUE;
}

gint ScintillaGTK::Press(GtkWidget *widget, GdkEventButton *event) {
	if (event->window != widget->window)
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
			if (event->window != PWidget(sciThis->wMain)->window)
				// If mouse released on scroll bar then the position is relative to the
				// scrollbar, not the drawing window so just repeat the most recent point.
				pt = sciThis->ptMouseLast;
			sciThis->ButtonUp(pt, event->time, (event->state & 4) != 0);
		}
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

// win32gtk and GTK >= 2 use SCROLL_* events instead of passing the
// button4/5/6/7 events to the GTK app
gint ScintillaGTK::ScrollEvent(GtkWidget *widget,
                               GdkEventScroll *event) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	try {

		if (widget == NULL || event == NULL)
			return FALSE;

		// Compute amount and direction to scroll (even tho on win32 there is
		// intensity of scrolling info in the native message, gtk doesn't
		// support this so we simulate similarly adaptive scrolling)
		// Note that this is disabled on OS X (Darwin) where the X11 server already has
		// and adaptive scrolling algorithm that fights with this one
		int cLineScroll;
#if defined(__MWERKS__) || defined(__APPLE_CPP__) || defined(__APPLE_CC__)
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
		if (event->window != widget->window)
			return FALSE;
		int x = 0;
		int y = 0;
		GdkModifierType state;
		if (event->is_hint) {
			gdk_window_get_pointer(event->window, &x, &y, &state);
		} else {
			x = static_cast<int>(event->x);
			y = static_cast<int>(event->y);
			state = static_cast<GdkModifierType>(event->state);
		}
		//Platform::DebugPrintf("Move %x %x %d %c %d %d\n",
		//	sciThis,event->window,event->time,event->is_hint? 'h' :'.', x, y);
		Point pt(x, y);
		sciThis->ButtonMove(pt);
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

// Map the keypad keys to their equivalent functions
static int KeyTranslate(int keyIn) {
	switch (keyIn) {
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
		if (ctrl && (key < 128))
			key = toupper(key);
		else if (!ctrl && (key >= GDK_KP_Multiply && key <= GDK_KP_9))
			key &= 0x7F;
		// Hack for keys over 256 and below command keys but makes Hungarian work.
		// This will have to change for Unicode
		else if (key >= 0xFE00)
			key = KeyTranslate(key);

		bool consumed = false;
		bool added = KeyDown(key, shift, ctrl, alt, &consumed) != 0;
		if (!consumed)
			consumed = added;
		//fprintf(stderr, "SK-key: %d %x %x\n",event->keyval, event->state, consumed);
		if (event->keyval == 0xffffff && event->length > 0) {
			ClearSelection();
			if (pdoc->InsertCString(CurrentPosition(), event->string)) {
				MovePositionTo(CurrentPosition() + event->length);
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

gboolean ScintillaGTK::KeyRelease(GtkWidget *, GdkEventKey * /*event*/) {
	//Platform::DebugPrintf("SC-keyrel: %d %x %3s\n",event->keyval, event->state, event->string);
	return FALSE;
}

gboolean ScintillaGTK::ExposePreeditThis(GtkWidget *widget, GdkEventExpose *ose) {
	try {
		gchar *str;
		gint cursor_pos;
		PangoAttrList *attrs;

		gtk_im_context_get_preedit_string(im_context, &str, &attrs, &cursor_pos);
		PangoLayout *layout = gtk_widget_create_pango_layout(PWidget(wText), str);
		pango_layout_set_attributes(layout, attrs);

#ifndef USE_CAIRO
		GdkGC *gc = gdk_gc_new(widget->window);
		GdkColor color[2] = {   {0, 0x0000, 0x0000, 0x0000},
			{0, 0xffff, 0xffff, 0xffff}
		};
		gdk_colormap_alloc_color(gdk_colormap_get_system(), color, FALSE, TRUE);
		gdk_colormap_alloc_color(gdk_colormap_get_system(), color + 1, FALSE, TRUE);

		gdk_gc_set_foreground(gc, color + 1);
		gdk_draw_rectangle(widget->window, gc, TRUE, ose->area.x, ose->area.y,
		        ose->area.width, ose->area.height);

		gdk_gc_set_foreground(gc, color);
		gdk_gc_set_background(gc, color + 1);
		gdk_draw_layout(widget->window, gc, 0, 0, layout);
		g_object_unref(gc);
#endif
		g_free(str);
		pango_attr_list_unref(attrs);
		g_object_unref(layout);
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return TRUE;
}

gboolean ScintillaGTK::ExposePreedit(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis) {
	return sciThis->ExposePreeditThis(widget, ose);
}

void ScintillaGTK::CommitThis(char *utfVal) {
	try {
		//~ fprintf(stderr, "Commit '%s'\n", utfVal);
		if (IsUnicodeMode()) {
			AddCharUTF(utfVal, strlen(utfVal));
		} else {
			const char *source = CharacterSetID();
			if (*source) {
				Converter conv(source, "UTF-8", true);
				if (conv) {
					char localeVal[4] = "\0\0\0";
					char *pin = utfVal;
					size_t inLeft = strlen(utfVal);
					char *pout = localeVal;
					size_t outLeft = sizeof(localeVal);
					size_t conversions = conv.Convert(&pin, &inLeft, &pout, &outLeft);
					if (conversions != ((size_t)(-1))) {
						*pout = '\0';
						for (int i = 0; localeVal[i]; i++) {
							AddChar(localeVal[i]);
						}
					} else {
						fprintf(stderr, "Conversion failed '%s'\n", utfVal);
					}
				}
			}
		}
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::Commit(GtkIMContext *, char  *str, ScintillaGTK *sciThis) {
	sciThis->CommitThis(str);
}

void ScintillaGTK::PreeditChangedThis() {
	try {
		gchar *str;
		PangoAttrList *attrs;
		gint cursor_pos;
		gtk_im_context_get_preedit_string(im_context, &str, &attrs, &cursor_pos);
		if (strlen(str) > 0) {
			PangoLayout *layout = gtk_widget_create_pango_layout(PWidget(wText), str);
			pango_layout_set_attributes(layout, attrs);

			gint w, h;
			pango_layout_get_pixel_size(layout, &w, &h);
			g_object_unref(layout);

			gint x, y;
			gdk_window_get_origin((PWidget(wText))->window, &x, &y);

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
		g_free(str);
		pango_attr_list_unref(attrs);
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::PreeditChanged(GtkIMContext *, ScintillaGTK *sciThis) {
	sciThis->PreeditChangedThis();
}

gint ScintillaGTK::StyleSetText(GtkWidget *widget, GtkStyle *, void*) {
	if (widget->window != NULL)
		gdk_window_set_back_pixmap(widget->window, NULL, FALSE);
	return FALSE;
}

gint ScintillaGTK::RealizeText(GtkWidget *widget, void*) {
	if (widget->window != NULL)
		gdk_window_set_back_pixmap(widget->window, NULL, FALSE);
	return FALSE;
}

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
	} catch (...) {
		// Its dead so nowhere to save the status
	}
}

gint ScintillaGTK::ExposeTextThis(GtkWidget * /*widget*/, GdkEventExpose *ose) {
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
		Surface *surfaceWindow = Surface::Allocate();
		if (surfaceWindow) {
			surfaceWindow->Init(PWidget(wText)->window, PWidget(wText));
			Paint(surfaceWindow, rcPaint);
			surfaceWindow->Release();
			delete surfaceWindow;
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

gint ScintillaGTK::ExposeText(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis) {
	return sciThis->ExposeTextThis(widget, ose);
}

gint ScintillaGTK::ExposeMain(GtkWidget *widget, GdkEventExpose *ose) {
	ScintillaGTK *sciThis = ScintillaFromWidget(widget);
	//Platform::DebugPrintf("Expose Main %0d,%0d %0d,%0d\n",
	//ose->area.x, ose->area.y, ose->area.width, ose->area.height);
	return sciThis->Expose(widget, ose);
}

gint ScintillaGTK::Expose(GtkWidget *, GdkEventExpose *ose) {
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

void ScintillaGTK::ScrollSignal(GtkAdjustment *adj, ScintillaGTK *sciThis) {
	try {
		sciThis->ScrollTo(static_cast<int>(adj->value), false);
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::ScrollHSignal(GtkAdjustment *adj, ScintillaGTK *sciThis) {
	try {
		sciThis->HorizontalScrollTo(static_cast<int>(adj->value * 2));
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
		if (selection_data->selection == GDK_SELECTION_PRIMARY) {
			if (sciThis->primary.s == NULL) {
				sciThis->CopySelectionRange(&sciThis->primary);
			}
			sciThis->GetSelection(selection_data, info, &sciThis->primary);
		}
#ifndef USE_GTK_CLIPBOARD
		else {
			sciThis->GetSelection(selection_data, info, &sciThis->copyText);
		}
#endif
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

void ScintillaGTK::DragBegin(GtkWidget *, GdkDragContext *) {
	//Platform::DebugPrintf("DragBegin\n");
}

gboolean ScintillaGTK::DragMotionThis(GdkDragContext *context,
                                 gint x, gint y, guint dragtime) {
	try {
		Point npt(x, y);
		SetDragPosition(SPositionFromLocation(npt, false, false, UserVirtualSpace()));
		GdkDragAction preferredAction = context->suggested_action;
		SelectionPosition pos = SPositionFromLocation(npt);
		if ((inDragDrop == ddDragging) && (PositionInSelection(pos.Position()))) {
			// Avoid dragging selection onto itself as that produces a move
			// with no real effect but which creates undo actions.
			preferredAction = static_cast<GdkDragAction>(0);
		} else if (context->actions == static_cast<GdkDragAction>
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
		if (context->action == GDK_ACTION_MOVE) {
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

int ScintillaGTK::TimeOut(ScintillaGTK *sciThis) {
	sciThis->Tick();
	return 1;
}

gboolean ScintillaGTK::IdleCallback(ScintillaGTK *sciThis) {
	// Idler will be automatically stopped, if there is nothing
	// to do while idle.
	gdk_threads_enter();
	bool ret = sciThis->Idle();
	if (ret == false) {
		// FIXME: This will remove the idler from GTK, we don't want to
		// remove it as it is removed automatically when this function
		// returns false (although, it should be harmless).
		sciThis->SetIdle(false);
	}
	gdk_threads_leave();
	return ret;
}

gboolean ScintillaGTK::StyleIdle(ScintillaGTK *sciThis) {
	gdk_threads_enter();
	sciThis->IdleStyling();
	gdk_threads_leave();
	// Idler will be automatically stopped
	return FALSE;
}

void ScintillaGTK::QueueStyling(int upTo) {
	Editor::QueueStyling(upTo);
	if (!styleNeeded.active) {
		// Only allow one style needed to be queued
		styleNeeded.active = true;
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

gint ScintillaGTK::PressCT(GtkWidget *widget, GdkEventButton *event, ScintillaGTK *sciThis) {
	try {
		if (event->window != widget->window)
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

gint ScintillaGTK::ExposeCT(GtkWidget *widget, GdkEventExpose * /*ose*/, CallTip *ctip) {
	try {
		Surface *surfaceWindow = Surface::Allocate();
		if (surfaceWindow) {
			surfaceWindow->Init(widget->window, widget);
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

sptr_t ScintillaGTK::DirectFunction(
    ScintillaGTK *sciThis, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	return sciThis->WndProc(iMessage, wParam, lParam);
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
	widget_class->size_request = SizeRequest;
	widget_class->size_allocate = SizeAllocate;
	widget_class->expose_event = ExposeMain;
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
	return GTK_WIDGET(g_object_new(scintilla_get_type(), NULL));
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
