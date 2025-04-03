//
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// @file ScintillaEditBase.h - Qt widget that wraps ScintillaQt and provides events and scrolling


#ifndef SCINTILLAEDITBASE_H
#define SCINTILLAEDITBASE_H

#include <cstddef>
#include <cstdint>

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <memory>

#include "Debugging.h"
#include "Geometry.h"
#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
#include "Platform.h"
#include "Scintilla.h"

#include <QAbstractScrollArea>
#include <QMimeData>
#include <QElapsedTimer>

namespace Scintilla::Internal {

class ScintillaQt;
class SurfaceImpl;

}

#ifndef EXPORT_IMPORT_API
#ifdef WIN32
#ifdef MAKING_LIBRARY
#define EXPORT_IMPORT_API __declspec(dllexport)
#else
// Defining dllimport upsets moc
#define EXPORT_IMPORT_API __declspec(dllimport)
//#define EXPORT_IMPORT_API
#endif
#else
#define EXPORT_IMPORT_API
#endif
#endif

class EXPORT_IMPORT_API ScintillaEditBase : public QAbstractScrollArea {
	Q_OBJECT

public:
	explicit ScintillaEditBase(QWidget *parent = 0);
	virtual ~ScintillaEditBase();

	virtual sptr_t send(
		unsigned int iMessage,
		uptr_t wParam = 0,
		sptr_t lParam = 0) const;

	virtual sptr_t sends(
		unsigned int iMessage,
		uptr_t wParam = 0,
		const char *s = 0) const;

public slots:
	// Scroll events coming from GUI to be sent to Scintilla.
	void scrollHorizontal(int value);
	void scrollVertical(int value);

	// Emit Scintilla notifications as signals.
	void notifyParent(Scintilla::NotificationData scn);
	void event_command(Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);

signals:
	void horizontalScrolled(int value);
	void verticalScrolled(int value);
	void horizontalRangeChanged(int max, int page);
	void verticalRangeChanged(int max, int page);
	void notifyChange();
	void linesAdded(Scintilla::Position linesAdded);

	// Clients can use this hook to add additional
	// formats (e.g. rich text) to the MIME data.
	void aboutToCopy(QMimeData *data);

	// Scintilla Notifications
	void styleNeeded(Scintilla::Position position);
	void charAdded(int ch);
	void savePointChanged(bool dirty);
	void modifyAttemptReadOnly();
	void key(int key);
	void doubleClick(Scintilla::Position position, Scintilla::Position line);
	void updateUi(Scintilla::Update updated);
	void modified(Scintilla::ModificationFlags type, Scintilla::Position position, Scintilla::Position length, Scintilla::Position linesAdded,
		      const QByteArray &text, Scintilla::Position line, Scintilla::FoldLevel foldNow, Scintilla::FoldLevel foldPrev);
	void macroRecord(Scintilla::Message message, Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);
	void marginClicked(Scintilla::Position position, Scintilla::KeyMod modifiers, int margin);
	void textAreaClicked(Scintilla::Position line, int modifiers);
	void needShown(Scintilla::Position position, Scintilla::Position length);
	void painted();
	void userListSelection(); // Wants some args.
	void uriDropped(const QString &uri);
	void dwellStart(int x, int y);
	void dwellEnd(int x, int y);
	void zoom(int zoom);
	void hotSpotClick(Scintilla::Position position, Scintilla::KeyMod modifiers);
	void hotSpotDoubleClick(Scintilla::Position position, Scintilla::KeyMod modifiers);
	void callTipClick();
	void autoCompleteSelection(Scintilla::Position position, const QString &text);
	void autoCompleteCancelled();
	void focusChanged(bool focused);

	// Base notifications for compatibility with other Scintilla implementations
	void notify(Scintilla::NotificationData *pscn);
	void command(Scintilla::uptr_t wParam, Scintilla::sptr_t lParam);

	// GUI event notifications needed under Qt
	void buttonPressed(QMouseEvent *event);
	void buttonReleased(QMouseEvent *event);
	void keyPressed(QKeyEvent *event);
	void resized();

protected:
	bool event(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void focusOutEvent(QFocusEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void inputMethodEvent(QInputMethodEvent *event) override;
	QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
	void scrollContentsBy(int, int) override {}

private:
	Scintilla::Internal::ScintillaQt *sqt;

	QElapsedTimer time;

	Scintilla::Position preeditPos;
	QString preeditString;

	int wheelDelta;

	static bool IsHangul(const QChar qchar);
	void MoveImeCarets(Scintilla::Position offset);
	void DrawImeIndicator(int indicator, int len);
	static Scintilla::KeyMod ModifiersOfKeyboard();
};

#endif /* SCINTILLAEDITBASE_H */
