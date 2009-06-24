/* Scintilla source code edit control */
/** @file ScintillaWidget.h
 ** Definition of Scintilla widget for GTK+.
 ** Only needed by GTK+ code but is harmless on other platforms.
 **/
/* Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
 * The License.txt file describes the conditions under which this software may be distributed. */

#ifndef SCINTILLAWIDGET_H
#define SCINTILLAWIDGET_H

#if PLAT_GTK

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

#if GLIB_MAJOR_VERSION < 2
GtkType		scintilla_get_type	(void);
#else
GType		scintilla_get_type	(void);
#endif
GtkWidget*	scintilla_new		(void);
void		scintilla_set_id	(ScintillaObject *sci, uptr_t id);
sptr_t		scintilla_send_message	(ScintillaObject *sci,unsigned int iMessage, uptr_t wParam, sptr_t lParam);
void		scintilla_release_resources(void);

#if GTK_MAJOR_VERSION < 2
#define SCINTILLA_NOTIFY "notify"
#else
#define SCINTILLA_NOTIFY "sci-notify"
#endif

#ifdef __cplusplus
}
#endif

#endif

#endif
