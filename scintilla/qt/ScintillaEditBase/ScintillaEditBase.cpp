//
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// ScintillaEditBase.cpp - Qt widget that wraps ScintillaQt and provides events and scrolling

#include "ScintillaEditBase.h"
#include "ScintillaQt.h"
#include "PlatQt.h"

#include <QApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QInputContext>
#endif
#include <QPainter>
#include <QScrollBar>
#include <QTextFormat>
#include <QVarLengthArray>

#define INDIC_INPUTMETHOD 24

#define MAXLENINPUTIME 200
#define SC_INDICATOR_INPUT INDIC_IME
#define SC_INDICATOR_TARGET INDIC_IME+1
#define SC_INDICATOR_CONVERTED INDIC_IME+2
#define SC_INDICATOR_UNKNOWN INDIC_IME_MAX

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

ScintillaEditBase::ScintillaEditBase(QWidget *parent)
: QAbstractScrollArea(parent), sqt(0), preeditPos(-1), wheelDelta(0)
{
	sqt = new ScintillaQt(this);

	time.start();

	// Set Qt defaults.
	setAcceptDrops(true);
	setMouseTracking(true);
	setAutoFillBackground(false);
	setFrameStyle(QFrame::NoFrame);
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_StaticContents);
	viewport()->setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_KeyCompression);
	setAttribute(Qt::WA_InputMethodEnabled);

	sqt->vs.indicators[SC_INDICATOR_UNKNOWN] = Indicator(INDIC_HIDDEN, ColourDesired(0, 0, 0xff));
	sqt->vs.indicators[SC_INDICATOR_INPUT] = Indicator(INDIC_DOTS, ColourDesired(0, 0, 0xff));
	sqt->vs.indicators[SC_INDICATOR_CONVERTED] = Indicator(INDIC_COMPOSITIONTHICK, ColourDesired(0, 0, 0xff));
	sqt->vs.indicators[SC_INDICATOR_TARGET] = Indicator(INDIC_STRAIGHTBOX, ColourDesired(0, 0, 0xff));

	connect(sqt, SIGNAL(notifyParent(SCNotification)),
	        this, SLOT(notifyParent(SCNotification)));

	// Connect scroll bars.
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)),
	        this, SLOT(scrollVertical(int)));
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
	        this, SLOT(scrollHorizontal(int)));

	// Connect pass-through signals.
	connect(sqt, SIGNAL(horizontalRangeChanged(int,int)),
	        this, SIGNAL(horizontalRangeChanged(int,int)));
	connect(sqt, SIGNAL(verticalRangeChanged(int,int)),
	        this, SIGNAL(verticalRangeChanged(int,int)));
	connect(sqt, SIGNAL(horizontalScrolled(int)),
	        this, SIGNAL(horizontalScrolled(int)));
	connect(sqt, SIGNAL(verticalScrolled(int)),
	        this, SIGNAL(verticalScrolled(int)));

	connect(sqt, SIGNAL(notifyChange()),
	        this, SIGNAL(notifyChange()));

	connect(sqt, SIGNAL(command(uptr_t, sptr_t)),
	        this, SLOT(event_command(uptr_t, sptr_t)));

	connect(sqt, SIGNAL(aboutToCopy(QMimeData *)),
	        this, SIGNAL(aboutToCopy(QMimeData *)));
}

ScintillaEditBase::~ScintillaEditBase() {}

sptr_t ScintillaEditBase::send(
	unsigned int iMessage,
	uptr_t wParam,
	sptr_t lParam) const
{
	return sqt->WndProc(iMessage, wParam, lParam);
}

sptr_t ScintillaEditBase::sends(
    unsigned int iMessage,
    uptr_t wParam,
    const char *s) const
{
	return sqt->WndProc(iMessage, wParam, (sptr_t)s);
}

void ScintillaEditBase::scrollHorizontal(int value)
{
	sqt->HorizontalScrollTo(value);
}

void ScintillaEditBase::scrollVertical(int value)
{
	sqt->ScrollTo(value);
}

bool ScintillaEditBase::event(QEvent *event)
{
	bool result = false;

	if (event->type() == QEvent::KeyPress) {
		// Circumvent the tab focus convention.
		keyPressEvent(static_cast<QKeyEvent *>(event));
		result = event->isAccepted();
	} else if (event->type() == QEvent::Hide) {
		setMouseTracking(false);
		result = QAbstractScrollArea::event(event);
	} else {
		result = QAbstractScrollArea::event(event);
	}

	return result;
}

void ScintillaEditBase::paintEvent(QPaintEvent *event)
{
	sqt->PartialPaint(PRectFromQRect(event->rect()));
}

void ScintillaEditBase::wheelEvent(QWheelEvent *event)
{
	if (event->orientation() == Qt::Horizontal) {
		if (horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff)
			event->ignore();
		else
			QAbstractScrollArea::wheelEvent(event);
	} else {
		if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
			// Zoom! We play with the font sizes in the styles.
			// Number of steps/line is ignored, we just care if sizing up or down
			if (event->delta() > 0) {
				sqt->KeyCommand(SCI_ZOOMIN);
			} else {
				sqt->KeyCommand(SCI_ZOOMOUT);
			}
		} else {
			// Ignore wheel events when the scroll bars are disabled.
			if (verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff) {
				event->ignore();
			} else {
				// Scroll
				QAbstractScrollArea::wheelEvent(event);
			}
		}
	}
}

void ScintillaEditBase::focusInEvent(QFocusEvent *event)
{
	sqt->SetFocusState(true);
	emit updateUi();

	QAbstractScrollArea::focusInEvent(event);
}

void ScintillaEditBase::focusOutEvent(QFocusEvent *event)
{
	sqt->SetFocusState(false);

	QAbstractScrollArea::focusOutEvent(event);
}

void ScintillaEditBase::resizeEvent(QResizeEvent *)
{
	sqt->ChangeSize();
	emit resized();
}

void ScintillaEditBase::keyPressEvent(QKeyEvent *event)
{
	// All keystrokes containing the meta modifier are
	// assumed to be shortcuts not handled by scintilla.
	if (QApplication::keyboardModifiers() & Qt::MetaModifier) {
		QAbstractScrollArea::keyPressEvent(event);
		emit keyPressed(event);
		return;
	}

	int key = 0;
	switch (event->key()) {
		case Qt::Key_Down:          key = SCK_DOWN;     break;
		case Qt::Key_Up:            key = SCK_UP;       break;
		case Qt::Key_Left:          key = SCK_LEFT;     break;
		case Qt::Key_Right:         key = SCK_RIGHT;    break;
		case Qt::Key_Home:          key = SCK_HOME;     break;
		case Qt::Key_End:           key = SCK_END;      break;
		case Qt::Key_PageUp:        key = SCK_PRIOR;    break;
		case Qt::Key_PageDown:      key = SCK_NEXT;     break;
		case Qt::Key_Delete:        key = SCK_DELETE;   break;
		case Qt::Key_Insert:        key = SCK_INSERT;   break;
		case Qt::Key_Escape:        key = SCK_ESCAPE;   break;
		case Qt::Key_Backspace:     key = SCK_BACK;     break;
		case Qt::Key_Plus:          key = SCK_ADD;      break;
		case Qt::Key_Minus:         key = SCK_SUBTRACT; break;
		case Qt::Key_Backtab:       // fall through
		case Qt::Key_Tab:           key = SCK_TAB;      break;
		case Qt::Key_Enter:         // fall through
		case Qt::Key_Return:        key = SCK_RETURN;   break;
		case Qt::Key_Control:       key = 0;            break;
		case Qt::Key_Alt:           key = 0;            break;
		case Qt::Key_Shift:         key = 0;            break;
		case Qt::Key_Meta:          key = 0;            break;
		default:                    key = event->key(); break;
	}

	bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
	bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
	bool alt   = QApplication::keyboardModifiers() & Qt::AltModifier;

	bool consumed = false;
	bool added = sqt->KeyDown(key, shift, ctrl, alt, &consumed) != 0;
	if (!consumed)
		consumed = added;

	if (!consumed) {
		// Don't insert text if the control key was pressed unless
		// it was pressed in conjunction with alt for AltGr emulation.
		bool input = (!ctrl || alt);

		// Additionally, on non-mac platforms, don't insert text
		// if the alt key was pressed unless control is also present.
		// On mac alt can be used to insert special characters.
#ifndef Q_WS_MAC
		input &= (!alt || ctrl);
#endif

		QString text = event->text();
		if (input && !text.isEmpty() && text[0].isPrint()) {
			QByteArray utext = sqt->BytesForDocument(text);
			sqt->AddCharUTF(utext.data(), utext.size());
		} else {
			event->ignore();
		}
	}

	emit keyPressed(event);
}

#ifdef Q_WS_X11
static int modifierTranslated(int sciModifier)
{
	switch (sciModifier) {
		case SCMOD_SHIFT:
			return Qt::ShiftModifier;
		case SCMOD_CTRL:
			return Qt::ControlModifier;
		case SCMOD_ALT:
			return Qt::AltModifier;
		case SCMOD_SUPER:
			return Qt::MetaModifier;
		default:
			return 0;
	}
}
#endif

void ScintillaEditBase::mousePressEvent(QMouseEvent *event)
{
	Point pos = PointFromQPoint(event->pos());

	emit buttonPressed(event);

	if (event->button() == Qt::MidButton &&
	    QApplication::clipboard()->supportsSelection()) {
		SelectionPosition selPos = sqt->SPositionFromLocation(
					pos, false, false, sqt->UserVirtualSpace());
		sqt->sel.Clear();
		sqt->SetSelection(selPos, selPos);
		sqt->PasteFromMode(QClipboard::Selection);
		return;
	}

	bool button = event->button() == Qt::LeftButton;

	if (button) {
		bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
		bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
#ifdef Q_WS_X11
		// On X allow choice of rectangular modifier since most window
		// managers grab alt + click for moving windows.
		bool alt   = QApplication::keyboardModifiers() & modifierTranslated(sqt->rectangularSelectionModifier);
#else
		bool alt   = QApplication::keyboardModifiers() & Qt::AltModifier;
#endif

		sqt->ButtonDown(pos, time.elapsed(), shift, ctrl, alt);
	}
}

void ScintillaEditBase::mouseReleaseEvent(QMouseEvent *event)
{
	Point point = PointFromQPoint(event->pos());
	bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
	if (event->button() == Qt::LeftButton)
		sqt->ButtonUp(point, time.elapsed(), ctrl);

	int pos = send(SCI_POSITIONFROMPOINT, point.x, point.y);
	int line = send(SCI_LINEFROMPOSITION, pos);
	int modifiers = QApplication::keyboardModifiers();

	emit textAreaClicked(line, modifiers);
	emit buttonReleased(event);
}

void ScintillaEditBase::mouseDoubleClickEvent(QMouseEvent *event)
{
	// Scintilla does its own double-click detection.
	mousePressEvent(event);
}

void ScintillaEditBase::mouseMoveEvent(QMouseEvent *event)
{
	Point pos = PointFromQPoint(event->pos());

	bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
	bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
#ifdef Q_WS_X11
	// On X allow choice of rectangular modifier since most window
	// managers grab alt + click for moving windows.
	bool alt   = QApplication::keyboardModifiers() & modifierTranslated(sqt->rectangularSelectionModifier);
#else
	bool alt   = QApplication::keyboardModifiers() & Qt::AltModifier;
#endif

	int modifiers = (shift ? SCI_SHIFT : 0) | (ctrl ? SCI_CTRL : 0) | (alt ? SCI_ALT : 0);

	sqt->ButtonMoveWithModifiers(pos, modifiers);
}

void ScintillaEditBase::contextMenuEvent(QContextMenuEvent *event)
{
	Point pos = PointFromQPoint(event->globalPos());
	Point pt = PointFromQPoint(event->pos());
	if (!sqt->PointInSelection(pt))
		sqt->SetEmptySelection(sqt->PositionFromLocation(pt));
	sqt->ContextMenu(pos);
}

void ScintillaEditBase::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasText()) {
		event->acceptProposedAction();

		Point point = PointFromQPoint(event->pos());
		sqt->DragEnter(point);
	} else {
		event->ignore();
	}
}

void ScintillaEditBase::dragLeaveEvent(QDragLeaveEvent * /* event */)
{
	sqt->DragLeave();
}

void ScintillaEditBase::dragMoveEvent(QDragMoveEvent *event)
{
	if (event->mimeData()->hasText()) {
		event->acceptProposedAction();

		Point point = PointFromQPoint(event->pos());
		sqt->DragMove(point);
	} else {
		event->ignore();
	}
}

void ScintillaEditBase::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasText()) {
		event->acceptProposedAction();

		Point point = PointFromQPoint(event->pos());
		bool move = (event->source() == this &&
                 event->proposedAction() == Qt::MoveAction);
		sqt->Drop(point, event->mimeData(), move);
	} else {
		event->ignore();
	}
}

bool ScintillaEditBase::IsHangul(const QChar qchar)
{
	int unicode = (int)qchar.unicode();
	// Korean character ranges used for preedit chars.
	// http://www.programminginkorean.com/programming/hangul-in-unicode/
	const bool HangulJamo = (0x1100 <= unicode && unicode <= 0x11FF);
	const bool HangulCompatibleJamo = (0x3130 <= unicode && unicode <= 0x318F);
	const bool HangulJamoExtendedA = (0xA960 <= unicode && unicode <= 0xA97F);
	const bool HangulJamoExtendedB = (0xD7B0 <= unicode && unicode <= 0xD7FF);
	const bool HangulSyllable = (0xAC00 <= unicode && unicode <= 0xD7A3);
	return HangulJamo || HangulCompatibleJamo  || HangulSyllable ||
				HangulJamoExtendedA || HangulJamoExtendedB; 
}

void ScintillaEditBase::MoveImeCarets(int offset) 
{
	// Move carets relatively by bytes
	for (size_t r=0; r < sqt->sel.Count(); r++) {
		int positionInsert = sqt->sel.Range(r).Start().Position();
		sqt->sel.Range(r).caret.SetPosition(positionInsert + offset);
		sqt->sel.Range(r).anchor.SetPosition(positionInsert + offset);
 	}
}
 
void ScintillaEditBase::DrawImeIndicator(int indicator, int len) 
{
	// Emulate the visual style of IME characters with indicators.
	// Draw an indicator on the character before caret by the character bytes of len
	// so it should be called after AddCharUTF().
	// It does not affect caret positions.
	if (indicator < 8 || indicator > INDIC_MAX) {
		return;
	}
	sqt->pdoc->decorations.SetCurrentIndicator(indicator);
	for (size_t r=0; r< sqt-> sel.Count(); r++) {
		int positionInsert = sqt->sel.Range(r).Start().Position();
		sqt->pdoc->DecorationFillRange(positionInsert - len, 1, len);
	}
}

void ScintillaEditBase::inputMethodEvent(QInputMethodEvent *event)
{
	// Copy & paste by johnsonj with a lot of helps of Neil
	// Great thanks for my forerunners, jiniya and BLUEnLIVE

	if (sqt->pdoc->TentativeActive()) {
		sqt->pdoc->TentativeUndo();
	} else {
		// No tentative undo means start of this composition so
		// Fill in any virtual spaces.
		sqt->FillVirtualSpace();
	}

	sqt->view.imeCaretBlockOverride = false;

	if (!event->commitString().isEmpty()) {
		const QString commitStr = event->commitString();
		const unsigned int commitStrLen = commitStr.length();

		for (unsigned int i = 0; i < commitStrLen;) {
			const unsigned int ucWidth = commitStr.at(i).isHighSurrogate() ? 2 : 1;
			const QString oneCharUTF16 = commitStr.mid(i, ucWidth);
			const QByteArray oneChar = sqt->BytesForDocument(oneCharUTF16);
			const int oneCharLen = oneChar.length();

			sqt->AddCharUTF(oneChar.data(), oneCharLen);
			i += ucWidth;
		}

	} else if (!event->preeditString().isEmpty()) {
		const QString preeditStr = event->preeditString();
		const unsigned int preeditStrLen = preeditStr.length();
		if ((preeditStrLen == 0) || (preeditStrLen > MAXLENINPUTIME)) {
			sqt->ShowCaretAtCurrentPosition();
			return;
		}

		sqt->pdoc->TentativeStart(); // TentativeActive() from now on.

		// Mark segments and get ime caret position.
		unsigned int imeCaretPos = 0;
		unsigned int imeIndicator[MAXLENINPUTIME] = {0};
#ifdef Q_OS_LINUX
		// ibus-qt has a bug to return only one underline style.
		// Q_OS_LINUX blocks are temporary work around to cope with it.
		unsigned int attrSegment = 0;
#endif

		foreach (QInputMethodEvent::Attribute attr, event->attributes()) {
			if (attr.type == QInputMethodEvent::TextFormat) {
				QTextFormat format = attr.value.value<QTextFormat>();
				QTextCharFormat charFormat = format.toCharFormat();

				unsigned int indicator = SC_INDICATOR_UNKNOWN;
				switch (charFormat.underlineStyle()) {
					case QTextCharFormat::NoUnderline:
						indicator = SC_INDICATOR_TARGET; //target input
						break;
					case QTextCharFormat::SingleUnderline:
					case QTextCharFormat::DashUnderline:
						indicator = SC_INDICATOR_INPUT; //normal input
						break;
					case QTextCharFormat::DotLine:
					case QTextCharFormat::DashDotLine:
					case QTextCharFormat::WaveUnderline:
					case QTextCharFormat::SpellCheckUnderline:
						indicator = SC_INDICATOR_CONVERTED;
						break;
		
					default:
						indicator = SC_INDICATOR_UNKNOWN;
				}

#ifdef Q_OS_LINUX
				attrSegment++;
				indicator = attr.start;
#endif
				for (int i = attr.start; i < attr.start+attr.length; i++) {
					imeIndicator[i] = indicator;
				}
			} else if (attr.type == QInputMethodEvent::Cursor) {
				imeCaretPos = attr.start;
			}
		}
#ifdef Q_OS_LINUX
		const bool targetInput = (attrSegment > 1) || ((imeCaretPos == 0) && (preeditStr != preeditString));
		preeditString = preeditStr;
#endif
		// Display preedit characters one by one.
		int imeCharPos[MAXLENINPUTIME] = {0};
		int numBytes = 0;

		const bool recording = sqt->recordingMacro;
		sqt->recordingMacro = false;
		for (unsigned int i = 0; i < preeditStrLen;) {
			const unsigned int ucWidth = preeditStr.at(i).isHighSurrogate() ? 2 : 1;
			const QString oneCharUTF16 = preeditStr.mid(i, ucWidth);
			const QByteArray oneChar = sqt->BytesForDocument(oneCharUTF16);
			const int oneCharLen = oneChar.length();

			// Record character positions for moving ime caret.
			numBytes += oneCharLen;
			imeCharPos[i+1] = numBytes;

			sqt->AddCharUTF(oneChar.data(), oneCharLen);

#ifdef Q_OS_LINUX
			// Segment marked with imeCaretPos is for target input.
			if ((imeIndicator[i] == imeCaretPos) && (targetInput)) {
				DrawImeIndicator(SC_INDICATOR_TARGET, oneCharLen);
			} else {
				DrawImeIndicator(SC_INDICATOR_INPUT, oneCharLen);
			}
#else
			DrawImeIndicator(imeIndicator[i], oneCharLen);
#endif
			i += ucWidth;
		} 
		sqt->recordingMacro = recording;

		// Move IME carets.
		if (IsHangul(preeditStr.at(0))) {
			sqt->view.imeCaretBlockOverride = true;
			MoveImeCarets(- imeCharPos[preeditStrLen]);
		} else {
			MoveImeCarets(- imeCharPos[preeditStrLen] + imeCharPos[imeCaretPos]);
		}

		// Set candidate box position for Qt::ImMicroFocus.
		preeditPos = sqt->CurrentPosition();
		updateMicroFocus();
	}
	sqt->ShowCaretAtCurrentPosition();
}

QVariant ScintillaEditBase::inputMethodQuery(Qt::InputMethodQuery query) const
{
	int pos = send(SCI_GETCURRENTPOS);
	int line = send(SCI_LINEFROMPOSITION, pos);

	switch (query) {
		case Qt::ImMicroFocus:
		{
			int startPos = (preeditPos >= 0) ? preeditPos : pos;
			Point pt = sqt->LocationFromPosition(startPos);
			int width = send(SCI_GETCARETWIDTH);
			int height = send(SCI_TEXTHEIGHT, line);
			return QRect(pt.x, pt.y, width, height);
		}

		case Qt::ImFont:
		{
			char fontName[64];
			int style = send(SCI_GETSTYLEAT, pos);
			int len = send(SCI_STYLEGETFONT, style, (sptr_t)fontName);
			int size = send(SCI_STYLEGETSIZE, style);
			bool italic = send(SCI_STYLEGETITALIC, style);
			int weight = send(SCI_STYLEGETBOLD, style) ? QFont::Bold : -1;
			return QFont(QString::fromUtf8(fontName, len), size, weight, italic);
		}

		case Qt::ImCursorPosition:
		{
			int paraStart = sqt->pdoc->ParaUp(pos);
			return pos - paraStart;
		}

		case Qt::ImSurroundingText:
		{
			int paraStart = sqt->pdoc->ParaUp(pos);
			int paraEnd = sqt->pdoc->ParaDown(pos);
			QVarLengthArray<char,1024> buffer(paraEnd - paraStart + 1);

			Sci_CharacterRange charRange;
			charRange.cpMin = paraStart;
			charRange.cpMax = paraEnd;

			Sci_TextRange textRange;
			textRange.chrg = charRange;
			textRange.lpstrText = buffer.data();

			send(SCI_GETTEXTRANGE, 0, (sptr_t)&textRange);

			return sqt->StringFromDocument(buffer.constData());
		}

		case Qt::ImCurrentSelection:
		{
			QVarLengthArray<char,1024> buffer(send(SCI_GETSELTEXT));
			send(SCI_GETSELTEXT, 0, (sptr_t)buffer.data());

			return sqt->StringFromDocument(buffer.constData());
		}

		default:
			return QVariant();
	}
}

void ScintillaEditBase::notifyParent(SCNotification scn)
{
	emit notify(&scn);
	switch (scn.nmhdr.code) {
		case SCN_STYLENEEDED:
			emit styleNeeded(scn.position);
			break;

		case SCN_CHARADDED:
			emit charAdded(scn.ch);
			break;

		case SCN_SAVEPOINTREACHED:
			emit savePointChanged(false);
			break;

		case SCN_SAVEPOINTLEFT:
			emit savePointChanged(true);
			break;

		case SCN_MODIFYATTEMPTRO:
			emit modifyAttemptReadOnly();
			break;

		case SCN_KEY:
			emit key(scn.ch);
			break;

		case SCN_DOUBLECLICK:
			emit doubleClick(scn.position, scn.line);
			break;

		case SCN_UPDATEUI:
			emit updateUi();
			break;

		case SCN_MODIFIED:
		{
			bool added = scn.modificationType & SC_MOD_INSERTTEXT;
			bool deleted = scn.modificationType & SC_MOD_DELETETEXT;

			int length = send(SCI_GETTEXTLENGTH);
			bool firstLineAdded = (added && length == 1) ||
			                      (deleted && length == 0);

			if (scn.linesAdded != 0) {
				emit linesAdded(scn.linesAdded);
			} else if (firstLineAdded) {
				emit linesAdded(added ? 1 : -1);
			}

			const QByteArray bytes = QByteArray::fromRawData(scn.text, scn.length);
			emit modified(scn.modificationType, scn.position, scn.length,
			              scn.linesAdded, bytes, scn.line,
			              scn.foldLevelNow, scn.foldLevelPrev);
			break;
		}

		case SCN_MACRORECORD:
			emit macroRecord(scn.message, scn.wParam, scn.lParam);
			break;

		case SCN_MARGINCLICK:
			emit marginClicked(scn.position, scn.modifiers, scn.margin);
			break;

		case SCN_NEEDSHOWN:
			emit needShown(scn.position, scn.length);
			break;

		case SCN_PAINTED:
			emit painted();
			break;

		case SCN_USERLISTSELECTION:
			emit userListSelection();
			break;

		case SCN_URIDROPPED:
			emit uriDropped();
			break;

		case SCN_DWELLSTART:
			emit dwellStart(scn.x, scn.y);
			break;

		case SCN_DWELLEND:
			emit dwellEnd(scn.x, scn.y);
			break;

		case SCN_ZOOM:
			emit zoom(send(SCI_GETZOOM));
			break;

		case SCN_HOTSPOTCLICK:
			emit hotSpotClick(scn.position, scn.modifiers);
			break;

		case SCN_HOTSPOTDOUBLECLICK:
			emit hotSpotDoubleClick(scn.position, scn.modifiers);
			break;

		case SCN_CALLTIPCLICK:
			emit callTipClick();
			break;

		case SCN_AUTOCSELECTION:
			emit autoCompleteSelection(scn.lParam, QString::fromUtf8(scn.text));
			break;

		case SCN_AUTOCCANCELLED:
			emit autoCompleteCancelled();
			break;

		default:
			return;
	}
}

void ScintillaEditBase::event_command(uptr_t wParam, sptr_t lParam)
{
	emit command(wParam, lParam);
}
