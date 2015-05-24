
#ifndef __scintilla_marshal_MARSHAL_H__
#define __scintilla_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* NONE:INT,POINTER (scintilla-marshal.list:1) */
extern void scintilla_marshal_VOID__INT_POINTER (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);
#define scintilla_marshal_NONE__INT_POINTER	scintilla_marshal_VOID__INT_POINTER

G_END_DECLS

#endif /* __scintilla_marshal_MARSHAL_H__ */

