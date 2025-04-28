//
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// @file ScintillaEditBase.cpp - Qt widget that wraps ScintillaQt and provides events and scrolling

#include "ScintillaEditBase.h"
#include "ScintillaQt.h"
#include "PlatQt.h"

#include <QApplication>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QInputContext>
#endif
#include <QPainter>
#include <QVarLengthArray>
#include <QScrollBar>
#include <QTextFormat>

constexpr int IndicatorInput = static_cast<int>(Scintilla::IndicatorNumbers::Ime);
constexpr int IndicatorTarget = IndicatorInput + 1;
constexpr int IndicatorConverted = IndicatorInput + 2;
constexpr int IndicatorUnknown = IndicatorInput + 3;

// Q_WS_MAC and Q_WS_X11 aren't defined in Qt5
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#ifdef Q_OS_MAC
#define Q_WS_MAC 1
#endif

#if !defined(Q_OS_MAC) && !defined(Q_OS_WIN)
#define Q_WS_X11 1
#endif
#endif // QT_VERSION >= 5.0.0

using namespace Scintilla;
using namespace Scintilla::Internal;

ScintillaEditBase::ScintillaEditBase(QWidget *parent)
: QAbstractScrollArea(parent), sqt(new ScintillaQt(this)), preeditPos(-1), wheelDelta(0)
{
	time.start();

	// Set Qt defaults.
	setAcceptDrops(true);
	setMouseTracking(true);
	setAutoFillBackground(false);
	setFrameStyle(QFrame::NoFrame);
	setFocusPolicy(Qt::StrongFocus);
	setAttribute(Qt::WA_StaticContents);
	viewport()->setAutoFillBackground(false);
	setAttribute(Qt::WA_KeyCompression);
	setAttribute(Qt::WA_InputMethodEnabled);

	sqt->vs.indicators[IndicatorUnknown] = Indicator(IndicatorStyle::Hidden, colourIME);
	sqt->vs.indicators[IndicatorInput] = Indicator(IndicatorStyle::Dots, colourIME);
	sqt->vs.indicators[IndicatorConverted] = Indicator(IndicatorStyle::CompositionThick, colourIME);
	sqt->vs.indicators[IndicatorTarget] = Indicator(IndicatorStyle::StraightBox, colourIME);

	connect(sqt, SIGNAL(notifyParent(Scintilla::NotificationData)),
		this, SLOT(notifyParent(Scintilla::NotificationData)));

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

	connect(sqt, SIGNAL(command(Scintilla::uptr_t,Scintilla::sptr_t)),
		this, SLOT(event_command(Scintilla::uptr_t,Scintilla::sptr_t)));

	connect(sqt, SIGNAL(aboutToCopy(QMimeData*)),
		this, SIGNAL(aboutToCopy(QMimeData*)));
}

ScintillaEditBase::~ScintillaEditBase() = default;

sptr_t ScintillaEditBase::send(
	unsigned int iMessage,
	uptr_t wParam,
	sptr_t lParam) const
{
	return sqt->WndProc(static_cast<Message>(iMessage), wParam, lParam);
}

sptr_t ScintillaEditBase::sends(
    unsigned int iMessage,
    uptr_t wParam,
    const char *s) const
{
	return sqt->WndProc(static_cast<Message>(iMessage), wParam, reinterpret_cast<sptr_t>(s));
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
	} else if (event->type() == QEvent::Show) {
		setMouseTracking(true);
		result = QAbstractScrollArea::event(event);
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

namespace {

bool isWheelEventHorizontal(QWheelEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	return event->angleDelta().y() == 0;
#else
	return event->orientation() == Qt::Horizontal;
#endif
}

int wheelEventYDelta(QWheelEvent *event) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
	return event->angleDelta().y();
#else
	return event->delta();
#endif
}

}

void ScintillaEditBase::wheelEvent(QWheelEvent *event)
{
	if (isWheelEventHorizontal(event)) {
		QAbstractScrollArea::wheelEvent(event);
	} else {
		if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
			// Zoom! We play with the font sizes in the styles.
			// Number of steps/line is ignored, we just care if sizing up or down
			if (wheelEventYDelta(event) > 0) {
				sqt->KeyCommand(Message::ZoomIn);
			} else {
				sqt->KeyCommand(Message::ZoomOut);
			}
		} else {
			// Scroll
			QAbstractScrollArea::wheelEvent(event);
		}
	}
}

void ScintillaEditBase::focusInEvent(QFocusEvent *event)
{
	sqt->SetFocusState(true);

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

	const bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
	const bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
	const bool alt   = QApplication::keyboardModifiers() & Qt::AltModifier;

	bool consumed = false;
	const bool added = sqt->KeyDownWithModifiers(static_cast<Keys>(key),
					       ModifierFlags(shift, ctrl, alt),
					       &consumed) != 0;
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
			const int strLen = text.length();
			for (int i = 0; i < strLen;) {
				const int ucWidth = text.at(i).isHighSurrogate() ? 2 : 1;
				const QString oneCharUTF16 = text.mid(i, ucWidth);
				const QByteArray oneChar = sqt->BytesForDocument(oneCharUTF16);

				sqt->InsertCharacter(std::string_view(oneChar.data(), oneChar.length()), CharacterSource::DirectInput);
				i += ucWidth;
			}
		} else {
			event->ignore();
		}
	}

	emit keyPressed(event);
}

namespace {

int modifierTranslated(int sciModifier)
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

// Convert QElapsedTimer timestamp qint64 into unsigned int milliseconds wanted by Editor methods.
unsigned int TimeOfEvent(const QElapsedTimer &timer)
{
	// Wrap every 2 billion milliseconds -> 24 days
	constexpr qint64 maxTime = 2'000'000'000;
	return static_cast<unsigned int>(timer.elapsed() % maxTime);
}

}

void ScintillaEditBase::mousePressEvent(QMouseEvent *event)
{
	const Point pos = PointFromQPoint(event->pos());

	emit buttonPressed(event);

	if (event->button() == Qt::MiddleButton &&
	    QApplication::clipboard()->supportsSelection()) {
		const SelectionPosition selPos = sqt->SPositionFromLocation(
					pos, false, false, sqt->UserVirtualSpace());
		sqt->sel.Clear();
		sqt->SetSelection(selPos, selPos);
		sqt->PasteFromMode(QClipboard::Selection);
		return;
	}

	if (event->button() == Qt::LeftButton) {
		const bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
		const bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
		const bool alt   = QApplication::keyboardModifiers() & modifierTranslated(sqt->rectangularSelectionModifier);

		sqt->ButtonDownWithModifiers(pos, TimeOfEvent(time), ModifierFlags(shift, ctrl, alt));
	}

	if (event->button() == Qt::RightButton) {
		sqt->RightButtonDownWithModifiers(pos, TimeOfEvent(time), ModifiersOfKeyboard());
	}
}

void ScintillaEditBase::mouseReleaseEvent(QMouseEvent *event)
{
	const QPoint point = event->pos();
	if (event->button() == Qt::LeftButton)
		sqt->ButtonUpWithModifiers(PointFromQPoint(point), TimeOfEvent(time), ModifiersOfKeyboard());

	const sptr_t pos = send(SCI_POSITIONFROMPOINT, point.x(), point.y());
	const sptr_t line = send(SCI_LINEFROMPOSITION, pos);
	const int modifiers = QApplication::keyboardModifiers();

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
	const Point pos = PointFromQPoint(event->pos());

	const bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
	const bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
	const bool alt   = QApplication::keyboardModifiers() & modifierTranslated(sqt->rectangularSelectionModifier);

	const KeyMod modifiers = ModifierFlags(shift, ctrl, alt);

	sqt->ButtonMoveWithModifiers(pos, TimeOfEvent(time), modifiers);
}

void ScintillaEditBase::leaveEvent(QEvent *event)
{
	QWidget::leaveEvent(event);
	sqt->MouseLeave();
}

void ScintillaEditBase::contextMenuEvent(QContextMenuEvent *event)
{
	const Point pos = PointFromQPoint(event->globalPos());
	const Point pt = PointFromQPoint(event->pos());
	if (!sqt->PointInSelection(pt)) {
		sqt->SetEmptySelection(sqt->PositionFromLocation(pt));
	}
	if (sqt->ShouldDisplayPopup(pt)) {
		sqt->ContextMenu(pos);
		event->accept();
	} else {
		event->ignore();
	}
}

void ScintillaEditBase::dragEnterEvent(QDragEnterEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
	} else if (event->mimeData()->hasText()) {
		event->acceptProposedAction();

		const Point point = PointFromQPoint(event->pos());
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
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
	} else if (event->mimeData()->hasText()) {
		event->acceptProposedAction();

		const Point point = PointFromQPoint(event->pos());
		sqt->DragMove(point);
	} else {
		event->ignore();
	}
}

void ScintillaEditBase::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
		sqt->DropUrls(event->mimeData());
	} else if (event->mimeData()->hasText()) {
		event->acceptProposedAction();

		const Point point = PointFromQPoint(event->pos());
		const bool move = (event->source() == this &&
                 event->proposedAction() == Qt::MoveAction);
		sqt->Drop(point, event->mimeData(), move);
	} else {
		event->ignore();
	}
}

bool ScintillaEditBase::IsHangul(const QChar qchar)
{
	const unsigned int unicode = qchar.unicode();
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

void ScintillaEditBase::MoveImeCarets(Scintilla::Position offset)
{
	// Move carets relatively by bytes
	for (size_t r=0; r < sqt->sel.Count(); r++) {
		const Sci::Position positionInsert = sqt->sel.Range(r).Start().Position();
		sqt->sel.Range(r) = SelectionRange(positionInsert + offset);
 	}
}

void ScintillaEditBase::DrawImeIndicator(int indicator, int len)
{
	// Emulate the visual style of IME characters with indicators.
	// Draw an indicator on the character before caret by the character bytes of len
	// so it should be called after InsertCharacter().
	// It does not affect caret positions.
	if (indicator < INDICATOR_CONTAINER || indicator > INDICATOR_MAX) {
		return;
	}
	sqt->pdoc->DecorationSetCurrentIndicator(indicator);
	for (size_t r=0; r< sqt-> sel.Count(); r++) {
		const Sci::Position positionInsert = sqt->sel.Range(r).Start().Position();
		sqt->pdoc->DecorationFillRange(positionInsert - len, 1, len);
	}
}

namespace {

int GetImeCaretPos(QInputMethodEvent *event)
{
	foreach (const QInputMethodEvent::Attribute attr, event->attributes()) {
		if (attr.type == QInputMethodEvent::Cursor)
			return attr.start;
	}
	return 0;
}

std::vector<int> MapImeIndicators(QInputMethodEvent *event)
{
	std::vector<int> imeIndicator(event->preeditString().size(), IndicatorUnknown);
	foreach (const QInputMethodEvent::Attribute attr, event->attributes()) {
		if (attr.type == QInputMethodEvent::TextFormat) {
			const QTextFormat format = attr.value.value<QTextFormat>();
			const QTextCharFormat charFormat = format.toCharFormat();

			int indicator = IndicatorUnknown;
			switch (charFormat.underlineStyle()) {
				case QTextCharFormat::NoUnderline: // win32, linux
				case QTextCharFormat::SingleUnderline: // osx
				case QTextCharFormat::DashUnderline: // win32, linux
					indicator = IndicatorInput;
					break;
				case QTextCharFormat::DotLine:
				case QTextCharFormat::DashDotLine:
				case QTextCharFormat::WaveUnderline:
				case QTextCharFormat::SpellCheckUnderline:
					indicator = IndicatorConverted;
					break;

				default:
					indicator = IndicatorUnknown;
			}

			if (format.hasProperty(QTextFormat::BackgroundBrush)) // win32, linux
				indicator = IndicatorTarget;

#ifdef Q_OS_OSX
			if (charFormat.underlineStyle() == QTextCharFormat::SingleUnderline) {
				QColor uc = charFormat.underlineColor();
				if (uc.lightness() < 2) { // osx
					indicator = IndicatorTarget;
				}
			}
#endif

			for (int i = attr.start; i < attr.start+attr.length; i++) {
				imeIndicator[i] = indicator;
			}
		}
	}
	return imeIndicator;
}

}

void ScintillaEditBase::inputMethodEvent(QInputMethodEvent *event)
{
	// Copy & paste by johnsonj with a lot of helps of Neil
	// Great thanks for my forerunners, jiniya and BLUEnLIVE

	if (sqt->pdoc->IsReadOnly() || sqt->SelectionContainsProtected()) {
		// Here, a canceling and/or completing composition function is needed.
		return;
	}

	bool initialCompose = false;
	if (sqt->pdoc->TentativeActive()) {
		sqt->pdoc->TentativeUndo();
	} else {
		// No tentative undo means start of this composition so
		// Fill in any virtual spaces.
		initialCompose = true;
	}

	sqt->view.imeCaretBlockOverride = false;
	preeditPos = -1; // reset not to interrupt Qt::ImCursorRectangle.

	const int rpLength = event->replacementLength();
	if (rpLength != 0) {
		// Qt has called setCommitString().
		// Make room for the string to sit in.
		const int rpStart = event->replacementStart();
		const Scintilla::Position rpBase = sqt->CurrentPosition();
		const Scintilla::Position start = sqt->pdoc->GetRelativePositionUTF16(rpBase, rpStart);
		const Scintilla::Position end = sqt->pdoc->GetRelativePositionUTF16(start, rpLength);
		sqt->pdoc->DeleteChars(start, end - start);
	}

	if (!event->commitString().isEmpty()) {
		const QString &commitStr = event->commitString();
		const int commitStrLen = commitStr.length();

		for (int i = 0; i < commitStrLen;) {
			const int ucWidth = commitStr.at(i).isHighSurrogate() ? 2 : 1;
			const QString oneCharUTF16 = commitStr.mid(i, ucWidth);
			const QByteArray oneChar = sqt->BytesForDocument(oneCharUTF16);

			sqt->InsertCharacter(std::string_view(oneChar.data(), oneChar.length()), CharacterSource::DirectInput);
			i += ucWidth;
		}

	} else if (!event->preeditString().isEmpty()) {
		const QString preeditStr = event->preeditString();
		const int preeditStrLen = preeditStr.length();
		if (preeditStrLen == 0) {
			sqt->ShowCaretAtCurrentPosition();
			return;
		}

		if (initialCompose)
			sqt->ClearBeforeTentativeStart();
		sqt->pdoc->TentativeStart(); // TentativeActive() from now on.

		// Fix candidate window position at the start of preeditString.
		preeditPos = sqt->CurrentPosition();

		std::vector<int> imeIndicator = MapImeIndicators(event);

		for (int i = 0; i < preeditStrLen;) {
			const int ucWidth = preeditStr.at(i).isHighSurrogate() ? 2 : 1;
			const QString oneCharUTF16 = preeditStr.mid(i, ucWidth);
			const QByteArray oneChar = sqt->BytesForDocument(oneCharUTF16);
			const int oneCharLen = oneChar.length();

			sqt->InsertCharacter(std::string_view(oneChar.data(), oneCharLen), CharacterSource::TentativeInput);

			DrawImeIndicator(imeIndicator[i], oneCharLen);
			i += ucWidth;
		}

		// Move IME carets.
		const int imeCaretPos = GetImeCaretPos(event);
		const int imeEndToImeCaretU16 = imeCaretPos - preeditStrLen;
		const Sci::Position imeCaretPosDoc = sqt->pdoc->GetRelativePositionUTF16(sqt->CurrentPosition(), imeEndToImeCaretU16);

		MoveImeCarets(- sqt->CurrentPosition() + imeCaretPosDoc);

		if (IsHangul(preeditStr.at(0))) {
#ifndef Q_OS_WIN
			if (imeCaretPos > 0) {
				int oneCharBefore = sqt->pdoc->GetRelativePosition(sqt->CurrentPosition(), -1);
				MoveImeCarets(- sqt->CurrentPosition() + oneCharBefore);
			}
#endif
			sqt->view.imeCaretBlockOverride = true;
		}
		sqt->EnsureCaretVisible();
	}
	sqt->ShowCaretAtCurrentPosition();
}

QVariant ScintillaEditBase::inputMethodQuery(Qt::InputMethodQuery query) const
{
	const Scintilla::Position pos = send(SCI_GETCURRENTPOS);
	const Scintilla::Position line = send(SCI_LINEFROMPOSITION, pos);

	switch (query) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
		// Qt 5 renamed ImMicroFocus to ImCursorRectangle then deprecated
		// ImMicroFocus. Its the same value (2) and same description.
		case Qt::ImCursorRectangle:
		{
			const Scintilla::Position startPos = (preeditPos >= 0) ? preeditPos : pos;
			const Point pt = sqt->LocationFromPosition(startPos);
			const int width = static_cast<int>(send(SCI_GETCARETWIDTH));
			const int height = static_cast<int>(send(SCI_TEXTHEIGHT, line));
			return QRectF(pt.x, pt.y, width, height).toRect();
		}
#else
		case Qt::ImMicroFocus:
		{
			const Scintilla::Position startPos = (preeditPos >= 0) ? preeditPos : pos;
			const Point pt = sqt->LocationFromPosition(startPos);
			const int width = static_cast<int>(send(SCI_GETCARETWIDTH));
			const int height = static_cast<int>(send(SCI_TEXTHEIGHT, line));
			return QRect(pt.x, pt.y, width, height);
		}
#endif

		case Qt::ImFont:
		{
			char fontName[64];
			const sptr_t style = send(SCI_GETSTYLEAT, pos);
			const int len = static_cast<int>(sends(SCI_STYLEGETFONT, style, fontName));
			const int size = static_cast<int>(send(SCI_STYLEGETSIZE, style));
			const bool italic = send(SCI_STYLEGETITALIC, style);
			const int weight = send(SCI_STYLEGETBOLD, style) ? QFont::Bold : -1;
			return QFont(QString::fromUtf8(fontName, len), size, weight, italic);
		}

		case Qt::ImCursorPosition:
		{
			const Scintilla::Position paraStart = sqt->pdoc->ParaUp(pos);
			return static_cast<int>(sqt->pdoc->CountUTF16(paraStart, pos));
		}

		case Qt::ImSurroundingText:
		{
			const Scintilla::Position paraStart = sqt->pdoc->ParaUp(pos);
			const Scintilla::Position paraEnd = sqt->pdoc->ParaDown(pos);
			const std::string buffer = sqt->RangeText(paraStart, paraEnd);
			return sqt->StringFromDocument(buffer.c_str());
		}

		case Qt::ImCurrentSelection:
		{
			// Most of the time selection is small so can be stack allocated
			constexpr int reasonableSelectionLength = 1024;
			QVarLengthArray<char,reasonableSelectionLength> buffer(send(SCI_GETSELTEXT)+1);
			sends(SCI_GETSELTEXT, 0, buffer.data());

			return sqt->StringFromDocument(buffer.constData());
		}

		default:
			return QVariant();
	}
}

void ScintillaEditBase::notifyParent(NotificationData scn)
{
	emit notify(&scn);
	switch (scn.nmhdr.code) {
		case Notification::StyleNeeded:
			emit styleNeeded(scn.position);
			break;

		case Notification::CharAdded:
			emit charAdded(scn.ch);
			break;

		case Notification::SavePointReached:
			emit savePointChanged(false);
			break;

		case Notification::SavePointLeft:
			emit savePointChanged(true);
			break;

		case Notification::ModifyAttemptRO:
			emit modifyAttemptReadOnly();
			break;

		case Notification::Key:
			emit key(scn.ch);
			break;

		case Notification::DoubleClick:
			emit doubleClick(scn.position, scn.line);
			break;

		case Notification::UpdateUI:
			if (FlagSet(scn.updated, Update::Selection)) {
				updateMicroFocus();
			}
			emit updateUi(scn.updated);
			break;

		case Notification::Modified:
		{
			const bool added = FlagSet(scn.modificationType, ModificationFlags::InsertText);
			const bool deleted = FlagSet(scn.modificationType, ModificationFlags::DeleteText);

			const Scintilla::Position length = send(SCI_GETTEXTLENGTH);
			const bool firstLineAdded = (added && length == 1) ||
			                      (deleted && length == 0);

			if (scn.linesAdded != 0) {
				emit linesAdded(scn.linesAdded);
			} else if (firstLineAdded) {
				emit linesAdded(added ? 1 : -1);
			}

			const QByteArray bytes = QByteArray::fromRawData(scn.text, scn.text ? scn.length : 0);
			emit modified(scn.modificationType, scn.position, scn.length,
			              scn.linesAdded, bytes, scn.line,
			              scn.foldLevelNow, scn.foldLevelPrev);
			break;
		}

		case Notification::MacroRecord:
			emit macroRecord(scn.message, scn.wParam, scn.lParam);
			break;

		case Notification::MarginClick:
			emit marginClicked(scn.position, scn.modifiers, scn.margin);
			break;

		case Notification::NeedShown:
			emit needShown(scn.position, scn.length);
			break;

		case Notification::Painted:
			emit painted();
			break;

		case Notification::UserListSelection:
			emit userListSelection();
			break;

		case Notification::URIDropped:
			emit uriDropped(QString::fromUtf8(scn.text));
			break;

		case Notification::DwellStart:
			emit dwellStart(scn.x, scn.y);
			break;

		case Notification::DwellEnd:
			emit dwellEnd(scn.x, scn.y);
			break;

		case Notification::Zoom:
			emit zoom(send(SCI_GETZOOM));
			break;

		case Notification::HotSpotClick:
			emit hotSpotClick(scn.position, scn.modifiers);
			break;

		case Notification::HotSpotDoubleClick:
			emit hotSpotDoubleClick(scn.position, scn.modifiers);
			break;

		case Notification::CallTipClick:
			emit callTipClick();
			break;

		case Notification::AutoCSelection:
			emit autoCompleteSelection(scn.lParam, sqt->IsUnicodeMode() ? QString::fromUtf8(scn.text) : QString::fromLocal8Bit(scn.text));
			break;

		case Notification::AutoCCancelled:
			emit autoCompleteCancelled();
			break;

		case Notification::FocusIn:
			emit focusChanged(true);
			break;

		case Notification::FocusOut:
			emit focusChanged(false);
			break;

		default:
			return;
	}
}

void ScintillaEditBase::event_command(uptr_t wParam, sptr_t lParam)
{
	emit command(wParam, lParam);
}

KeyMod ScintillaEditBase::ModifiersOfKeyboard()
{
	const bool shift = QApplication::keyboardModifiers() & Qt::ShiftModifier;
	const bool ctrl  = QApplication::keyboardModifiers() & Qt::ControlModifier;
	const bool alt   = QApplication::keyboardModifiers() & Qt::AltModifier;

	return ModifierFlags(shift, ctrl, alt);
}
