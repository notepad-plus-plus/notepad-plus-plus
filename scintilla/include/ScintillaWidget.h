/* Scintilla source code edit control */
/** @file ScintillaWidget.h
 ** Definition of Scintilla widget for GTK+.
 ** Only needed by GTK+ code but is harmless on other platforms.
 **/
/* Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
 * The License.txt file describes the conditions under which this software may be distributed. */

#ifndef SCINTILLAWIDGET_H
#define SCINTILLAWIDGET_H

#if defined(GTK)

#ifdef __cplusplus
extern "C" {
#endif

#define SCINTILLA(obj)          GTK_CHECK_CAST (obj, scintilla_get_type (), ScintillaObject)
#define SCINTILLA_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, scintilla_get_type (), ScintillaClass)
#define IS_SCINTILLA(obj)       GTK_CHECK_TYPE (obj, scintilla_get_type ())

typedef struct _ScintillaObject ScintillaObject;
typedef struct _ScintillaClass  ScintillaClass;

struct _ScintillaObject {
	GtkContainer cont;
	void *pscin;
};

struct _ScintillaClass {
	GtkContainerClass parent_class;

	void (* command) (ScintillaObject *ttt);
	void (* notify) (ScintillaObject *ttt);
};

GType		scintilla_get_type	(void);
GtkWidget*	scintilla_new		(void);
void		scintilla_set_id	(ScintillaObject *sci, uptr_t id);
sptr_t		scintilla_send_message	(ScintillaObject *sci,unsigned int iMessage, uptr_t wParam, sptr_t lParam);
void		scintilla_release_resources(void);

#define SCINTILLA_NOTIFY "sci-notify"

#ifdef __cplusplus
}
#endif

#endif

#endif
