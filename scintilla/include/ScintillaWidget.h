/* Scintilla source code edit control */
/* @file ScintillaWidget.h
 * Definition of Scintilla widget for GTK+.
 * Only needed by GTK+ code but is harmless on other platforms.
 * This comment is not a doc-comment as that causes warnings from g-ir-scanner.
 */
/* Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
 * The License.txt file describes the conditions under which this software may be distributed. */

#ifndef SCINTILLAWIDGET_H
#define SCINTILLAWIDGET_H

#if defined(GTK)

#ifdef __cplusplus
extern "C" {
#endif

#define SCINTILLA(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, scintilla_get_type (), ScintillaObject)
#define SCINTILLA_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, scintilla_get_type (), ScintillaClass)
#define IS_SCINTILLA(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, scintilla_get_type ())

#define SCINTILLA_TYPE_OBJECT             (scintilla_object_get_type())
#define SCINTILLA_OBJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), SCINTILLA_TYPE_OBJECT, ScintillaObject))
#define SCINTILLA_IS_OBJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SCINTILLA_TYPE_OBJECT))
#define SCINTILLA_OBJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), SCINTILLA_TYPE_OBJECT, ScintillaObjectClass))
#define SCINTILLA_IS_OBJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), SCINTILLA_TYPE_OBJECT))
#define SCINTILLA_OBJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), SCINTILLA_TYPE_OBJECT, ScintillaObjectClass))

typedef struct _ScintillaObject ScintillaObject;
typedef struct _ScintillaClass  ScintillaObjectClass;

struct _ScintillaObject {
	GtkContainer cont;
	void *pscin;
};

struct _ScintillaClass {
	GtkContainerClass parent_class;

	void (* command) (ScintillaObject *sci, int cmd, GtkWidget *window);
	void (* notify) (ScintillaObject *sci, int id, SCNotification *scn);
};

GType		scintilla_object_get_type		(void);
GtkWidget*	scintilla_object_new			(void);
gintptr		scintilla_object_send_message	(ScintillaObject *sci, unsigned int iMessage, guintptr wParam, gintptr lParam);


GType		scnotification_get_type			(void);
#define SCINTILLA_TYPE_NOTIFICATION        (scnotification_get_type())

#ifndef G_IR_SCANNING
/* The legacy names confuse the g-ir-scanner program */
typedef struct _ScintillaClass  ScintillaClass;

GType		scintilla_get_type	(void);
GtkWidget*	scintilla_new		(void);
void		scintilla_set_id	(ScintillaObject *sci, uptr_t id);
sptr_t		scintilla_send_message	(ScintillaObject *sci,unsigned int iMessage, uptr_t wParam, sptr_t lParam);
void		scintilla_release_resources(void);
#endif

#define SCINTILLA_NOTIFY "sci-notify"

#ifdef __cplusplus
}
#endif

#endif

#endif
