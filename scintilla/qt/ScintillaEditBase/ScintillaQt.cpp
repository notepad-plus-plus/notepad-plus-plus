//
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// @file ScintillaQt.cpp - Qt specific subclass of ScintillaBase

#include "ScintillaQt.h"
#include "PlatQt.h"

#include <QApplication>
#include <QDrag>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QInputContext>
#endif
#include <QMimeData>
#include <QMenu>
#include <QTextCodec>
#include <QScrollBar>
#include <QTimer>

using namespace Scintilla;
using namespace Scintilla::Internal;

ScintillaQt::ScintillaQt(QAbstractScrollArea *parent)
: QObject(parent), scrollArea(parent), vMax(0),  hMax(0), vPage(0), hPage(0),
 haveMouseCapture(false), dragWasDropped(false),
 rectangularSelectionModifier(SCMOD_ALT)
{

	wMain = scrollArea->viewport();

	imeInteraction = IMEInteraction::Inline;

	// On macOS drawing text into a pixmap moves it around 1 pixel to
	// the right compared to drawing it directly onto a window.
	// Buffered drawing turned off by default to avoid this.
	view.bufferedDraw = false;

	Init();

	std::fill(timers, std::end(timers), 0);
}

ScintillaQt::~ScintillaQt()
{
	CancelTimers();
	ChangeIdle(false);
}

void ScintillaQt::execCommand(QAction *action)
{
	int command = action->data().toInt();
	Command(command);
}

#if defined(Q_OS_WIN)
static const QString sMSDEVColumnSelect("MSDEVColumnSelect");
static const QString sWrappedMSDEVColumnSelect("application/x-qt-windows-mime;value=\"MSDEVColumnSelect\"");
static const QString sVSEditorLineCutCopy("VisualStudioEditorOperationsLineCutCopyClipboardTag");
static const QString sWrappedVSEditorLineCutCopy("application/x-qt-windows-mime;value=\"VisualStudioEditorOperationsLineCutCopyClipboardTag\"");
#elif defined(Q_OS_MAC)
static const QString sScintillaRecPboardType("com.scintilla.utf16-plain-text.rectangular");
static const QString sScintillaRecMimeType("text/x-scintilla.utf16-plain-text.rectangular");
#else
// Linux
static const QString sMimeRectangularMarker("text/x-rectangular-marker");
#endif

#if defined(Q_OS_MAC) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)

class ScintillaRectangularMime : public QMacPasteboardMime {
public:
	ScintillaRectangularMime() : QMacPasteboardMime(MIME_ALL) {
	}

	QString convertorName() {
		return QString("ScintillaRectangularMime");
	}

	bool canConvert(const QString &mime, QString flav) {
		return mimeFor(flav) == mime;
	}

	QString mimeFor(QString flav) {
		if (flav == sScintillaRecPboardType)
			return sScintillaRecMimeType;
		return QString();
	}

	QString flavorFor(const QString &mime) {
		if (mime == sScintillaRecMimeType)
			return sScintillaRecPboardType;
		return QString();
	}

	QVariant convertToMime(const QString & /* mime */, QList<QByteArray> data, QString /* flav */) {
		QByteArray all;
		foreach (QByteArray i, data) {
			all += i;
		}
		return QVariant(all);
	}

	QList<QByteArray> convertFromMime(const QString & /* mime */, QVariant data, QString /* flav */) {
		QByteArray a = data.toByteArray();
		QList<QByteArray> l;
		l.append(a);
		return l;
	}

};

// The Mime object registers itself but only want one for all Scintilla instances.
// Should delete at exit to help memory leak detection but that would be extra work
// and, since the clipboard exists after last Scintilla instance, may be complex.
static ScintillaRectangularMime *singletonMime = 0;

#endif

void ScintillaQt::Init()
{
	rectangularSelectionModifier = SCMOD_ALT;

#if defined(Q_OS_MAC) && QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	if (!singletonMime) {
		singletonMime = new ScintillaRectangularMime();

		QStringList slTypes(sScintillaRecPboardType);
		qRegisterDraggedTypes(slTypes);
	}
#endif

	connect(QApplication::clipboard(), SIGNAL(selectionChanged()),
		this, SLOT(SelectionChanged()));
}

void ScintillaQt::Finalise()
{
	CancelTimers();
	ScintillaBase::Finalise();
}

void ScintillaQt::SelectionChanged()
{
	bool nowPrimary = QApplication::clipboard()->ownsSelection();
	if (nowPrimary != primarySelection) {
		primarySelection = nowPrimary;
		Redraw();
	}
}

bool ScintillaQt::DragThreshold(Point ptStart, Point ptNow)
{
	int xMove = std::abs(ptStart.x - ptNow.x);
	int yMove = std::abs(ptStart.y - ptNow.y);
	return (xMove > QApplication::startDragDistance()) ||
		(yMove > QApplication::startDragDistance());
}

static QString StringFromSelectedText(const SelectionText &selectedText)
{
	if (selectedText.codePage == SC_CP_UTF8) {
		return QString::fromUtf8(selectedText.Data(), static_cast<int>(selectedText.Length()));
	} else {
		QTextCodec *codec = QTextCodec::codecForName(
				CharacterSetID(selectedText.characterSet));
		return codec->toUnicode(selectedText.Data(), static_cast<int>(selectedText.Length()));
	}
}

static void AddRectangularToMime(QMimeData *mimeData, [[maybe_unused]] const QString &su)
{
#if defined(Q_OS_WIN)
	// Add an empty marker
	mimeData->setData(sMSDEVColumnSelect, QByteArray());
#elif defined(Q_OS_MAC)
	// macOS gets marker + data to work with other implementations.
	// Don't understand how this works but it does - the
	// clipboard format is supposed to be UTF-16, not UTF-8.
	mimeData->setData(sScintillaRecMimeType, su.toUtf8());
#else
	// Linux
	// Add an empty marker
	mimeData->setData(sMimeRectangularMarker, QByteArray());
#endif
}

static void AddLineCutCopyToMime([[maybe_unused]] QMimeData *mimeData)
{
#if defined(Q_OS_WIN)
	// Add an empty marker
	mimeData->setData(sVSEditorLineCutCopy, QByteArray());
#endif
}

static bool IsRectangularInMime(const QMimeData *mimeData)
{
	QStringList formats = mimeData->formats();
	for (int i = 0; i < formats.size(); ++i) {
#if defined(Q_OS_WIN)
		// Windows rectangular markers
		// If rectangular copies made by this application, see base name.
		if (formats[i] == sMSDEVColumnSelect)
			return true;
		// Otherwise see wrapped name.
		if (formats[i] == sWrappedMSDEVColumnSelect)
			return true;
#elif defined(Q_OS_MAC)
		if (formats[i] == sScintillaRecMimeType)
			return true;
#else
		// Linux
		if (formats[i] == sMimeRectangularMarker)
			return true;
#endif
	}
	return false;
}

static bool IsLineCutCopyInMime(const QMimeData *mimeData)
{
	QStringList formats = mimeData->formats();
	for (int i = 0; i < formats.size(); ++i) {
#if defined(Q_OS_WIN)
		// Visual Studio Line Cut/Copy markers
		// If line cut/copy made by this application, see base name.
		if (formats[i] == sVSEditorLineCutCopy)
			return true;
		// Otherwise see wrapped name.
		if (formats[i] == sWrappedVSEditorLineCutCopy)
			return true;
#endif
	}
	return false;
}

bool ScintillaQt::ValidCodePage(int codePage) const
{
	return codePage == 0
	|| codePage == SC_CP_UTF8
	|| codePage == 932
	|| codePage == 936
	|| codePage == 949
	|| codePage == 950
	|| codePage == 1361;
}

std::string ScintillaQt::UTF8FromEncoded(std::string_view encoded) const {
	if (IsUnicodeMode()) {
		return std::string(encoded);
	} else {
		QTextCodec *codec = QTextCodec::codecForName(
				CharacterSetID(CharacterSetOfDocument()));
		QString text = codec->toUnicode(encoded.data(), static_cast<int>(encoded.length()));
		return text.toStdString();
	}
}

std::string ScintillaQt::EncodedFromUTF8(std::string_view utf8) const {
	if (IsUnicodeMode()) {
		return std::string(utf8);
	} else {
		QString text = QString::fromUtf8(utf8.data(), static_cast<int>(utf8.length()));
		QTextCodec *codec = QTextCodec::codecForName(
				CharacterSetID(CharacterSetOfDocument()));
		QByteArray ba = codec->fromUnicode(text);
		return std::string(ba.data(), ba.length());
	}
}

void ScintillaQt::ScrollText(Sci::Line linesToMove)
{
	int dy = vs.lineHeight * (linesToMove);
	scrollArea->viewport()->scroll(0, dy);
}

void ScintillaQt::SetVerticalScrollPos()
{
	scrollArea->verticalScrollBar()->setValue(topLine);
	emit verticalScrolled(topLine);
}

void ScintillaQt::SetHorizontalScrollPos()
{
	scrollArea->horizontalScrollBar()->setValue(xOffset);
	emit horizontalScrolled(xOffset);
}

bool ScintillaQt::ModifyScrollBars(Sci::Line nMax, Sci::Line nPage)
{
	bool modified = false;

	int vNewPage = nPage;
	int vNewMax = nMax - vNewPage + 1;
	if (vMax != vNewMax || vPage != vNewPage) {
		vMax = vNewMax;
		vPage = vNewPage;
		modified = true;

		scrollArea->verticalScrollBar()->setMaximum(vMax);
		scrollArea->verticalScrollBar()->setPageStep(vPage);
		emit verticalRangeChanged(vMax, vPage);
	}

	int hNewPage = GetTextRectangle().Width();
	int hNewMax = (scrollWidth > hNewPage) ? scrollWidth - hNewPage : 0;
	int charWidth = vs.styles[STYLE_DEFAULT].aveCharWidth;
	if (hMax != hNewMax || hPage != hNewPage ||
	    scrollArea->horizontalScrollBar()->singleStep() != charWidth) {
		hMax = hNewMax;
		hPage = hNewPage;
		modified = true;

		scrollArea->horizontalScrollBar()->setMaximum(hMax);
		scrollArea->horizontalScrollBar()->setPageStep(hPage);
		scrollArea->horizontalScrollBar()->setSingleStep(charWidth);
		emit horizontalRangeChanged(hMax, hPage);
	}

	return modified;
}

void ScintillaQt::ReconfigureScrollBars()
{
	if (verticalScrollBarVisible) {
		scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	} else {
		scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}

	if (horizontalScrollBarVisible && !Wrapping()) {
		scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	} else {
		scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}
}

void ScintillaQt::CopyToModeClipboard(const SelectionText &selectedText, QClipboard::Mode clipboardMode_)
{
	QClipboard *clipboard = QApplication::clipboard();
	QString su = StringFromSelectedText(selectedText);
	QMimeData *mimeData = new QMimeData();
	mimeData->setText(su);
	if (selectedText.rectangular) {
		AddRectangularToMime(mimeData, su);
	}

	if (selectedText.lineCopy) {
		AddLineCutCopyToMime(mimeData);
	}

	// Allow client code to add additional data (e.g rich text).
	emit aboutToCopy(mimeData);

	clipboard->setMimeData(mimeData, clipboardMode_);
}

void ScintillaQt::Copy()
{
	if (!sel.Empty()) {
		SelectionText st;
		CopySelectionRange(&st);
		CopyToClipboard(st);
	}
}

void ScintillaQt::CopyToClipboard(const SelectionText &selectedText)
{
	CopyToModeClipboard(selectedText, QClipboard::Clipboard);
}

void ScintillaQt::PasteFromMode(QClipboard::Mode clipboardMode_)
{
	QClipboard *clipboard = QApplication::clipboard();
	const QMimeData *mimeData = clipboard->mimeData(clipboardMode_);
	bool isRectangular = IsRectangularInMime(mimeData);
	bool isLine = SelectionEmpty() && IsLineCutCopyInMime(mimeData);
	QString text = clipboard->text(clipboardMode_);
	QByteArray utext = BytesForDocument(text);
	std::string dest(utext.constData(), utext.length());
	SelectionText selText;
	selText.Copy(dest, pdoc->dbcsCodePage, CharacterSetOfDocument(), isRectangular, false);

	UndoGroup ug(pdoc);
	ClearSelection(multiPasteMode == MultiPaste::Each);
	InsertPasteShape(selText.Data(), selText.Length(),
		isRectangular ? PasteShape::rectangular : (isLine ? PasteShape::line : PasteShape::stream));
	EnsureCaretVisible();
}

void ScintillaQt::Paste()
{
	PasteFromMode(QClipboard::Clipboard);
}

void ScintillaQt::ClaimSelection()
{
	if (QApplication::clipboard()->supportsSelection()) {
		// X Windows has a 'primary selection' as well as the clipboard.
		// Whenever the user selects some text, we become the primary selection
		if (!sel.Empty()) {
			primarySelection = true;
			SelectionText st;
			CopySelectionRange(&st);
			CopyToModeClipboard(st, QClipboard::Selection);
		} else {
			primarySelection = false;
		}
	}
}

void ScintillaQt::NotifyChange()
{
	emit notifyChange();
	emit command(
			Platform::LongFromTwoShorts(GetCtrlID(), SCEN_CHANGE),
			reinterpret_cast<sptr_t>(wMain.GetID()));
}

void ScintillaQt::NotifyFocus(bool focus)
{
	if (commandEvents) {
		emit command(
				Platform::LongFromTwoShorts
						(GetCtrlID(), focus ? SCEN_SETFOCUS : SCEN_KILLFOCUS),
				reinterpret_cast<sptr_t>(wMain.GetID()));
	}

	Editor::NotifyFocus(focus);
}

void ScintillaQt::NotifyParent(NotificationData scn)
{
	scn.nmhdr.hwndFrom = wMain.GetID();
	scn.nmhdr.idFrom = GetCtrlID();
	emit notifyParent(scn);
}

void ScintillaQt::NotifyURIDropped(const char *uri)
{
	NotificationData scn = {};
	scn.nmhdr.code = Notification::URIDropped;
	scn.text = uri;

	NotifyParent(scn);
}

bool ScintillaQt::FineTickerRunning(TickReason reason)
{
	return timers[static_cast<size_t>(reason)] != 0;
}

void ScintillaQt::FineTickerStart(TickReason reason, int millis, int /* tolerance */)
{
	FineTickerCancel(reason);
	timers[static_cast<size_t>(reason)] = startTimer(millis);
}

// CancelTimers cleans up all fine-ticker timers and is non-virtual to avoid warnings when
// called during destruction.
void ScintillaQt::CancelTimers()
{
	for (size_t tr = static_cast<size_t>(TickReason::caret); tr <= static_cast<size_t>(TickReason::dwell); tr++) {
		if (timers[tr]) {
			killTimer(timers[tr]);
			timers[tr] = 0;
		}
	}
}

void ScintillaQt::FineTickerCancel(TickReason reason)
{
	const size_t reasonIndex = static_cast<size_t>(reason);
	if (timers[reasonIndex]) {
		killTimer(timers[reasonIndex]);
		timers[reasonIndex] = 0;
	}
}

void ScintillaQt::onIdle()
{
	bool continueIdling = Idle();
	if (!continueIdling) {
		SetIdle(false);
	}
}

bool ScintillaQt::ChangeIdle(bool on)
{
	if (on) {
		// Start idler, if it's not running.
		if (!idler.state) {
			idler.state = true;
			QTimer *qIdle = new QTimer;
			connect(qIdle, SIGNAL(timeout()), this, SLOT(onIdle()));
			qIdle->start(0);
			idler.idlerID = qIdle;
		}
	} else {
		// Stop idler, if it's running
		if (idler.state) {
			idler.state = false;
			QTimer *qIdle = static_cast<QTimer *>(idler.idlerID);
			qIdle->stop();
			disconnect(qIdle, SIGNAL(timeout()), nullptr, nullptr);
			delete qIdle;
			idler.idlerID = {};
		}
	}
	return true;
}

bool ScintillaQt::SetIdle(bool on)
{
	return ChangeIdle(on);
}

CharacterSet ScintillaQt::CharacterSetOfDocument() const
{
	return vs.styles[STYLE_DEFAULT].characterSet;
}

const char *ScintillaQt::CharacterSetIDOfDocument() const
{
	return CharacterSetID(CharacterSetOfDocument());
}

QString ScintillaQt::StringFromDocument(const char *s) const
{
	if (IsUnicodeMode()) {
		return QString::fromUtf8(s);
	} else {
		QTextCodec *codec = QTextCodec::codecForName(
				CharacterSetID(CharacterSetOfDocument()));
		return codec->toUnicode(s);
	}
}

QByteArray ScintillaQt::BytesForDocument(const QString &text) const
{
	if (IsUnicodeMode()) {
		return text.toUtf8();
	} else {
		QTextCodec *codec = QTextCodec::codecForName(
				CharacterSetID(CharacterSetOfDocument()));
		return codec->fromUnicode(text);
	}
}

namespace {

class CaseFolderDBCS : public CaseFolderTable {
	QTextCodec *codec;
public:
	explicit CaseFolderDBCS(QTextCodec *codec_) : codec(codec_) {
	}
	size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) override {
		if ((lenMixed == 1) && (sizeFolded > 0)) {
			folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
			return 1;
		} else if (codec) {
			QString su = codec->toUnicode(mixed, static_cast<int>(lenMixed));
			QString suFolded = su.toCaseFolded();
			QByteArray bytesFolded = codec->fromUnicode(suFolded);

			if (bytesFolded.length() < static_cast<int>(sizeFolded)) {
				memcpy(folded, bytesFolded,  bytesFolded.length());
				return bytesFolded.length();
			}
		}
		// Something failed so return a single NUL byte
		folded[0] = '\0';
		return 1;
	}
};

}

std::unique_ptr<CaseFolder> ScintillaQt::CaseFolderForEncoding()
{
	if (pdoc->dbcsCodePage == SC_CP_UTF8) {
		return std::make_unique<CaseFolderUnicode>();
	} else {
		const char *charSetBuffer = CharacterSetIDOfDocument();
		if (charSetBuffer) {
			if (pdoc->dbcsCodePage == 0) {
				std::unique_ptr<CaseFolderTable> pcf = std::make_unique<CaseFolderTable>();
				QTextCodec *codec = QTextCodec::codecForName(charSetBuffer);
				// Only for single byte encodings
				for (int i=0x80; i<0x100; i++) {
					char sCharacter[2] = "A";
					sCharacter[0] = static_cast<char>(i);
					QString su = codec->toUnicode(sCharacter, 1);
					QString suFolded = su.toCaseFolded();
					if (codec->canEncode(suFolded)) {
						QByteArray bytesFolded = codec->fromUnicode(suFolded);
						if (bytesFolded.length() == 1) {
							pcf->SetTranslation(sCharacter[0], bytesFolded[0]);
						}
					}
				}
				return pcf;
			} else {
				return std::make_unique<CaseFolderDBCS>(QTextCodec::codecForName(charSetBuffer));
			}
		}
		return nullptr;
	}
}

std::string ScintillaQt::CaseMapString(const std::string &s, CaseMapping caseMapping)
{
	if (s.empty() || (caseMapping == CaseMapping::same))
		return s;

	if (IsUnicodeMode()) {
		std::string retMapped(s.length() * maxExpansionCaseConversion, 0);
		size_t lenMapped = CaseConvertString(&retMapped[0], retMapped.length(), s.c_str(), s.length(),
			(caseMapping == CaseMapping::upper) ? CaseConversion::upper : CaseConversion::lower);
		retMapped.resize(lenMapped);
		return retMapped;
	}

	QTextCodec *codec = QTextCodec::codecForName(CharacterSetIDOfDocument());
	QString text = codec->toUnicode(s.c_str(), static_cast<int>(s.length()));

	if (caseMapping == CaseMapping::upper) {
		text = text.toUpper();
	} else {
		text = text.toLower();
	}

	QByteArray bytes = BytesForDocument(text);
	return std::string(bytes.data(), bytes.length());
}

void ScintillaQt::SetMouseCapture(bool on)
{
	// This is handled automatically by Qt
	if (mouseDownCaptures) {
		haveMouseCapture = on;
	}
}

bool ScintillaQt::HaveMouseCapture()
{
	return haveMouseCapture;
}

void ScintillaQt::StartDrag()
{
	inDragDrop = DragDrop::dragging;
	dropWentOutside = true;
	if (drag.Length()) {
		QMimeData *mimeData = new QMimeData;
		QString sText = StringFromSelectedText(drag);
		mimeData->setText(sText);
		if (drag.rectangular) {
			AddRectangularToMime(mimeData, sText);
		}
		// This QDrag is not freed as that causes a crash on Linux
		QDrag *dragon = new QDrag(scrollArea);
		dragon->setMimeData(mimeData);

		Qt::DropAction dropAction = dragon->exec(static_cast<Qt::DropActions>(Qt::CopyAction|Qt::MoveAction));
		if ((dropAction == Qt::MoveAction) && dropWentOutside) {
			// Remove dragged out text
			ClearSelection();
		}
	}
	inDragDrop = DragDrop::none;
	SetDragPosition(SelectionPosition(Sci::invalidPosition));
}

class CallTipImpl : public QWidget {
public:
	explicit CallTipImpl(CallTip *pct_)
		: QWidget(nullptr, Qt::ToolTip),
		  pct(pct_)
	{
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
		setWindowFlag(Qt::WindowTransparentForInput);
#endif
	}

	void paintEvent(QPaintEvent *) override
	{
		if (pct->inCallTipMode) {
			std::unique_ptr<Surface> surfaceWindow = Surface::Allocate(Technology::Default);
			surfaceWindow->Init(this);
			surfaceWindow->SetMode(SurfaceMode(pct->codePage, false));
			pct->PaintCT(surfaceWindow.get());
		}
	}

private:
	CallTip *pct;
};

void ScintillaQt::CreateCallTipWindow(PRectangle rc)
{

	if (!ct.wCallTip.Created()) {
		QWidget *pCallTip = new CallTipImpl(&ct);
		ct.wCallTip = pCallTip;
		pCallTip->move(rc.left, rc.top);
		pCallTip->resize(rc.Width(), rc.Height());
	}
}

void ScintillaQt::AddToPopUp(const char *label,
                             int cmd,
                             bool enabled)
{
	QMenu *menu = static_cast<QMenu *>(popup.GetID());
	QString text(label);

	if (text.isEmpty()) {
		menu->addSeparator();
	} else {
		QAction *action = menu->addAction(text);
		action->setData(cmd);
		action->setEnabled(enabled);
	}

	// Make sure the menu's signal is connected only once.
	menu->disconnect();
	connect(menu, SIGNAL(triggered(QAction*)),
		this, SLOT(execCommand(QAction*)));
}

sptr_t ScintillaQt::WndProc(Message iMessage, uptr_t wParam, sptr_t lParam)
{
	try {
		switch (iMessage) {

		case Message::SetIMEInteraction:
			// Only inline IME supported on Qt
			break;

		case Message::GrabFocus:
			scrollArea->setFocus(Qt::OtherFocusReason);
			break;

		case Message::GetDirectFunction:
			return reinterpret_cast<sptr_t>(DirectFunction);

		case Message::GetDirectStatusFunction:
			return reinterpret_cast<sptr_t>(DirectStatusFunction);

		case Message::GetDirectPointer:
			return reinterpret_cast<sptr_t>(this);

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

sptr_t ScintillaQt::DefWndProc(Message, uptr_t, sptr_t)
{
	return 0;
}

sptr_t ScintillaQt::DirectFunction(
    sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam)
{
	ScintillaQt *sci = reinterpret_cast<ScintillaQt *>(ptr);
	return sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
}

sptr_t ScintillaQt::DirectStatusFunction(
    sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam, int *pStatus)
{
	ScintillaQt *sci = reinterpret_cast<ScintillaQt *>(ptr);
	const sptr_t returnValue = sci->WndProc(static_cast<Message>(iMessage), wParam, lParam);
	*pStatus = static_cast<int>(sci->errorStatus);
	return returnValue;
}

// Additions to merge in Scientific Toolworks widget structure

void ScintillaQt::PartialPaint(const PRectangle &rect)
{
	rcPaint = rect;
	paintState = PaintState::painting;
	PRectangle rcClient = GetClientRectangle();
	paintingAllText = rcPaint.Contains(rcClient);

	AutoSurface surfacePaint(this);
	Paint(surfacePaint, rcPaint);
	surfacePaint->Release();

	if (paintState == PaintState::abandoned) {
		// FIXME: Failure to paint the requested rectangle in each
		// paint event causes flicker on some platforms (Mac?)
		// Paint rect immediately.
		paintState = PaintState::painting;
		paintingAllText = true;

		AutoSurface surface(this);
		Paint(surface, rcPaint);
		surface->Release();

		// Queue a full repaint.
		scrollArea->viewport()->update();
	}

	paintState = PaintState::notPainting;
}

void ScintillaQt::DragEnter(const Point &point)
{
	SetDragPosition(SPositionFromLocation(point,
					      false, false, UserVirtualSpace()));
}

void ScintillaQt::DragMove(const Point &point)
{
	SetDragPosition(SPositionFromLocation(point,
					      false, false, UserVirtualSpace()));
}

void ScintillaQt::DragLeave()
{
	SetDragPosition(SelectionPosition(Sci::invalidPosition));
}

void ScintillaQt::Drop(const Point &point, const QMimeData *data, bool move)
{
	QString text = data->text();
	bool rectangular = IsRectangularInMime(data);
	QByteArray bytes = BytesForDocument(text);
	int len = bytes.length();

	SelectionPosition movePos = SPositionFromLocation(point,
				false, false, UserVirtualSpace());

	DropAt(movePos, bytes, len, move, rectangular);
}

void ScintillaQt::DropUrls(const QMimeData *data)
{
	foreach(const QUrl &url, data->urls()) {
		NotifyURIDropped(url.toString().toUtf8().constData());
	}
}

void ScintillaQt::timerEvent(QTimerEvent *event)
{
	for (size_t tr=static_cast<size_t>(TickReason::caret); tr<=static_cast<size_t>(TickReason::dwell); tr++) {
		if (timers[tr] == event->timerId()) {
			TickFor(static_cast<TickReason>(tr));
		}
	}
}
