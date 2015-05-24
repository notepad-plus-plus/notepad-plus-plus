//
//          Copyright (c) 1990-2011, Scientific Toolworks, Inc.
//
// The License.txt file describes the conditions under which this software may be distributed.
//
// Author: Jason Haslam
//
// Additions Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware
// ScintillaWidget.h - Qt widget that wraps ScintillaQt and provides events and scrolling


#ifndef SCINTILLAEDITBASE_H
#define SCINTILLAEDITBASE_H

#include "Platform.h"
#include "Scintilla.h"

#include <QAbstractScrollArea>
#include <QMimeData>
#include <QTime>

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

class ScintillaQt;
class SurfaceImpl;
struct SCNotification;

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

class EXPORT_IMPORT_API ScintillaEditBase : public QAbstractScrollArea {
	Q_OBJECT

public:
	ScintillaEditBase(QWidget *parent = 0);
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
	void notifyParent(SCNotification scn);
	void event_command(uptr_t wParam, sptr_t lParam);

signals:
	void horizontalScrolled(int value);
	void verticalScrolled(int value);
	void horizontalRangeChanged(int max, int page);
	void verticalRangeChanged(int max, int page);
	void notifyChange();
	void linesAdded(int linesAdded);

	// Clients can use this hook to add additional
	// formats (e.g. rich text) to the MIME data.
	void aboutToCopy(QMimeData *data);

	// Scintilla Notifications
	void styleNeeded(int position);
	void charAdded(int ch);
	void savePointChanged(bool dirty);
	void modifyAttemptReadOnly();
	void key(int key);
	void doubleClick(int position, int line);
	void updateUi();
	void modified(int type, int position, int length, int linesAdded,
	              const QByteArray &text, int line, int foldNow, int foldPrev);
	void macroRecord(int message, uptr_t wParam, sptr_t lParam);
	void marginClicked(int position, int modifiers, int margin);
	void textAreaClicked(int line, int modifiers);
	void needShown(int position, int length);
	void painted();
	void userListSelection(); // Wants some args.
	void uriDropped();        // Wants some args.
	void dwellStart(int x, int y);
	void dwellEnd(int x, int y);
	void zoom(int zoom);
	void hotSpotClick(int position, int modifiers);
	void hotSpotDoubleClick(int position, int modifiers);
	void callTipClick();
	void autoCompleteSelection(int position, const QString &text);
	void autoCompleteCancelled();

	// Base notifications for compatibility with other Scintilla implementations
	void notify(SCNotification *pscn);
	void command(uptr_t wParam, sptr_t lParam);

	// GUI event notifications needed under Qt
	void buttonPressed(QMouseEvent *event);
	void buttonReleased(QMouseEvent *event);
	void keyPressed(QKeyEvent *event);
	void resized();

protected:
	virtual bool event(QEvent *event);
	virtual void paintEvent(QPaintEvent *event);
	virtual void wheelEvent(QWheelEvent *event);
	virtual void focusInEvent(QFocusEvent *event);
	virtual void focusOutEvent(QFocusEvent *event);
	virtual void resizeEvent(QResizeEvent *event);
	virtual void keyPressEvent(QKeyEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void contextMenuEvent(QContextMenuEvent *event);
	virtual void dragEnterEvent(QDragEnterEvent *event);
	virtual void dragLeaveEvent(QDragLeaveEvent *event);
	virtual void dragMoveEvent(QDragMoveEvent *event);
	virtual void dropEvent(QDropEvent *event);
	virtual void inputMethodEvent(QInputMethodEvent *event);
	virtual QVariant inputMethodQuery(Qt::InputMethodQuery query) const;
	virtual void scrollContentsBy(int, int) {}

private:
	ScintillaQt *sqt;

	QTime time;

	int preeditPos;
	QString preeditString;

	int wheelDelta;
};

#ifdef SCI_NAMESPACE
}
#endif

#endif /* SCINTILLAEDITBASE_H */
