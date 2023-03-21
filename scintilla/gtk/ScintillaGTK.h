// Scintilla source code edit control
// ScintillaGTK.h - GTK+ specific subclass of ScintillaBase
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SCINTILLAGTK_H
#define SCINTILLAGTK_H

namespace Scintilla::Internal {

class ScintillaGTKAccessible;

#define OBJECT_CLASS GObjectClass

struct FontOptions {
	cairo_antialias_t antialias {};
	cairo_subpixel_order_t order {};
	cairo_hint_style_t hint {};
	FontOptions() noexcept = default;
	explicit FontOptions(GtkWidget *widget) noexcept;
	bool operator==(const FontOptions &other) const noexcept;
};

class ScintillaGTK : public ScintillaBase {
	friend class ScintillaGTKAccessible;

	_ScintillaObject *sci;
	Window wText;
	Window scrollbarv;
	Window scrollbarh;
	GtkAdjustment *adjustmentv;
	GtkAdjustment *adjustmenth;
	int verticalScrollBarWidth;
	int horizontalScrollBarHeight;

	PRectangle rectangleClient;

	SelectionText primary;
	SelectionPosition posPrimary;

	UniqueGdkEvent evbtn;
	guint buttonMouse;
	bool capturedMouse;
	bool dragWasDropped;
	int lastKey;
	int rectangularSelectionModifier;

	GtkWidgetClass *parentClass;

	static inline GdkAtom atomUTF8 {};
	static inline GdkAtom atomUTF8Mime {};
	static inline GdkAtom atomString {};
	static inline GdkAtom atomUriList {};
	static inline GdkAtom atomDROPFILES_DND {};
	GdkAtom atomSought;
	size_t inClearSelection = 0;

#if PLAT_GTK_WIN32
	CLIPFORMAT cfColumnSelect;
#endif

	bool preeditInitialized;
	Window wPreedit;
	Window wPreeditDraw;
	UniqueIMContext im_context;
	GUnicodeScript lastNonCommonScript;

	GtkSettings *settings;
	gulong settingsHandlerId;

	// Wheel mouse support
	unsigned int linesPerScroll;
	gint64 lastWheelMouseTime;
	gint lastWheelMouseDirection;
	gint wheelMouseIntensity;
	gdouble smoothScrollY;
	gdouble smoothScrollX;

#if GTK_CHECK_VERSION(3,0,0)
	cairo_rectangle_list_t *rgnUpdate;
#else
	GdkRegion *rgnUpdate;
#endif
	bool repaintFullWindow;

	guint styleIdleID;
	guint scrollBarIdleID = 0;
	FontOptions fontOptionsPrevious;
	int accessibilityEnabled;
	AtkObject *accessible;

public:
	explicit ScintillaGTK(_ScintillaObject *sci_);
	// Deleted so ScintillaGTK objects can not be copied.
	ScintillaGTK(const ScintillaGTK &) = delete;
	ScintillaGTK(ScintillaGTK &&) = delete;
	ScintillaGTK &operator=(const ScintillaGTK &) = delete;
	ScintillaGTK &operator=(ScintillaGTK &&) = delete;
	~ScintillaGTK() override;
	static ScintillaGTK *FromWidget(GtkWidget *widget) noexcept;
	static void ClassInit(OBJECT_CLASS *object_class, GtkWidgetClass *widget_class, GtkContainerClass *container_class);
private:
	void Init();
	void Finalise() override;
	bool AbandonPaint() override;
	void DisplayCursor(Window::Cursor c) override;
	bool DragThreshold(Point ptStart, Point ptNow) override;
	void StartDrag() override;
	Sci::Position TargetAsUTF8(char *text) const;
	Sci::Position EncodedFromUTF8(const char *utf8, char *encoded) const;
	bool ValidCodePage(int codePage) const override;
	std::string UTF8FromEncoded(std::string_view encoded) const override;
	std::string EncodedFromUTF8(std::string_view utf8) const override;
public: 	// Public for scintilla_send_message
	sptr_t WndProc(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam) override;
private:
	sptr_t DefWndProc(Scintilla::Message iMessage, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam) override;
	struct TimeThunk {
		TickReason reason;
		ScintillaGTK *scintilla;
		guint timer;
		TimeThunk() noexcept : reason(TickReason::caret), scintilla(nullptr), timer(0) {}
	};
	TimeThunk timers[static_cast<size_t>(TickReason::dwell)+1];
	bool FineTickerRunning(TickReason reason) override;
	void FineTickerStart(TickReason reason, int millis, int tolerance) override;
	void FineTickerCancel(TickReason reason) override;
	bool SetIdle(bool on) override;
	void SetMouseCapture(bool on) override;
	bool HaveMouseCapture() override;
	bool PaintContains(PRectangle rc) override;
	void FullPaint();
	void SetClientRectangle();
	PRectangle GetClientRectangle() const override;
	void ScrollText(Sci::Line linesToMove) override;
	void SetVerticalScrollPos() override;
	void SetHorizontalScrollPos() override;
	bool ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) override;
	void ReconfigureScrollBars() override;
	void SetScrollBars() override;
	void NotifyChange() override;
	void NotifyFocus(bool focus) override;
	void NotifyParent(Scintilla::NotificationData scn) override;
	void NotifyKey(Scintilla::Keys key, Scintilla::KeyMod modifiers);
	void NotifyURIDropped(const char *list);
	const char *CharacterSetID() const;
	std::unique_ptr<CaseFolder> CaseFolderForEncoding() override;
	std::string CaseMapString(const std::string &s, CaseMapping caseMapping) override;
	int KeyDefault(Scintilla::Keys key, Scintilla::KeyMod modifiers) override;
	void CopyToClipboard(const SelectionText &selectedText) override;
	void Copy() override;
	void RequestSelection(GdkAtom atomSelection);
	void Paste() override;
	void CreateCallTipWindow(PRectangle rc) override;
	void AddToPopUp(const char *label, int cmd = 0, bool enabled = true) override;
	bool OwnPrimarySelection();
	void ClaimSelection() override;
	static bool IsStringAtom(GdkAtom type);
	void GetGtkSelectionText(GtkSelectionData *selectionData, SelectionText &selText);
	void InsertSelection(GtkClipboard *clipBoard, GtkSelectionData *selectionData);
public:	// Public for SelectionReceiver
	GObject *MainObject() const noexcept;
	void ReceivedClipboard(GtkClipboard *clipBoard, GtkSelectionData *selection_data) noexcept;
private:
	void ReceivedSelection(GtkSelectionData *selection_data);
	void ReceivedDrop(GtkSelectionData *selection_data);
	static void GetSelection(GtkSelectionData *selection_data, guint info, SelectionText *text);
	void StoreOnClipboard(SelectionText *clipText);
	static void ClipboardGetSelection(GtkClipboard *clip, GtkSelectionData *selection_data, guint info, void *data);
	static void ClipboardClearSelection(GtkClipboard *clip, void *data);

	void ClearPrimarySelection();
	void PrimaryGetSelectionThis(GtkClipboard *clip, GtkSelectionData *selection_data, guint info);
	static void PrimaryGetSelection(GtkClipboard *clip, GtkSelectionData *selection_data, guint info, gpointer pSci);
	void PrimaryClearSelectionThis(GtkClipboard *clip);
	static void PrimaryClearSelection(GtkClipboard *clip, gpointer pSci);

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
	void CheckForFontOptionChange();
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
	AtkObject *GetAccessibleThis(GtkWidget *widget);
	static AtkObject *GetAccessible(GtkWidget *widget);

	bool KoreanIME();
	void CommitThis(char *commitStr);
	static void Commit(GtkIMContext *context, char *str, ScintillaGTK *sciThis);
	void PreeditChangedInlineThis();
	void PreeditChangedWindowedThis();
	static void PreeditChanged(GtkIMContext *context, ScintillaGTK *sciThis);
	bool RetrieveSurroundingThis(GtkIMContext *context);
	static gboolean RetrieveSurrounding(GtkIMContext *context, ScintillaGTK *sciThis);
	bool DeleteSurroundingThis(GtkIMContext *context, gint characterOffset, gint characterCount);
	static gboolean DeleteSurrounding(GtkIMContext *context, gint characterOffset, gint characterCount,
					  ScintillaGTK *sciThis);
	void MoveImeCarets(Sci::Position pos);
	void DrawImeIndicator(int indicator, Sci::Position len);
	void SetCandidateWindowPos();

	static void StyleSetText(GtkWidget *widget, GtkStyle *previous, void *);
	static void RealizeText(GtkWidget *widget, void *);
	static void Dispose(GObject *object);
	static void Destroy(GObject *object);
	static void SelectionReceived(GtkWidget *widget, GtkSelectionData *selection_data,
				      guint time);
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
	static gboolean TimeOut(gpointer ptt);
	static gboolean IdleCallback(gpointer pSci);
	static gboolean StyleIdle(gpointer pSci);
	void IdleWork() override;
	void QueueIdleWork(WorkItems items, Sci::Position upTo) override;
	void SetDocPointer(Document *document) override;
	static void PopUpCB(GtkMenuItem *menuItem, ScintillaGTK *sciThis);

#if GTK_CHECK_VERSION(3,0,0)
	static gboolean DrawCT(GtkWidget *widget, cairo_t *cr, CallTip *ctip);
#else
	static gboolean ExposeCT(GtkWidget *widget, GdkEventExpose *ose, CallTip *ctip);
#endif
	static gboolean PressCT(GtkWidget *widget, GdkEventButton *event, ScintillaGTK *sciThis);

	static sptr_t DirectFunction(sptr_t ptr,
				     unsigned int iMessage, uptr_t wParam, sptr_t lParam);
	static sptr_t DirectStatusFunction(sptr_t ptr,
				     unsigned int iMessage, uptr_t wParam, sptr_t lParam, int *pStatus);
};

// helper class to watch a GObject lifetime and get notified when it dies
class GObjectWatcher {
	GObject *weakRef;

	void WeakNotifyThis(GObject *obj G_GNUC_UNUSED) {
		PLATFORM_ASSERT(obj == weakRef);

		Destroyed();
		weakRef = nullptr;
	}

	static void WeakNotify(gpointer data, GObject *obj) {
		static_cast<GObjectWatcher *>(data)->WeakNotifyThis(obj);
	}

public:
	GObjectWatcher(GObject *obj) :
		weakRef(obj) {
		g_object_weak_ref(weakRef, WeakNotify, this);
	}

	// Deleted so GObjectWatcher objects can not be copied.
	GObjectWatcher(const GObjectWatcher&) = delete;
	GObjectWatcher(GObjectWatcher&&) = delete;
	GObjectWatcher&operator=(const GObjectWatcher&) = delete;
	GObjectWatcher&operator=(GObjectWatcher&&) = delete;

	virtual ~GObjectWatcher() {
		if (weakRef) {
			g_object_weak_unref(weakRef, WeakNotify, this);
		}
	}

	virtual void Destroyed() {}

	bool IsDestroyed() const {
		return weakRef != nullptr;
	}
};

std::string ConvertText(const char *s, size_t len, const char *charSetDest,
			const char *charSetSource, bool transliterations, bool silent=false);

}

#endif
