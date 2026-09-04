// Scintilla source code edit control
// Wrappers.h - Encapsulation of GLib, GObject, Pango, Cairo, GTK, and GDK types
// Copyright 2022 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef WRAPPERS_H
#define WRAPPERS_H

namespace Scintilla::Internal {

// GLib

struct GFreeReleaser {
	template <class T>
	void operator()(T *object) noexcept {
		g_free(object);
	}
};

using UniqueStr = std::unique_ptr<gchar, GFreeReleaser>;

// GObject

struct GObjectReleaser {
	// Called by unique_ptr to destroy/free the object
	template <class T>
	void operator()(T *object) noexcept {
		g_object_unref(object);
	}
};

// Pango

using UniquePangoContext = std::unique_ptr<PangoContext, GObjectReleaser>;
using UniquePangoLayout = std::unique_ptr<PangoLayout, GObjectReleaser>;
using UniquePangoFontMap = std::unique_ptr<PangoFontMap, GObjectReleaser>;

struct FontDescriptionReleaser {
	void operator()(PangoFontDescription *fontDescription) noexcept {
		pango_font_description_free(fontDescription);
	}
};

using UniquePangoFontDescription = std::unique_ptr<PangoFontDescription, FontDescriptionReleaser>;

struct FontMetricsReleaser {
	void operator()(PangoFontMetrics *metrics) noexcept {
		pango_font_metrics_unref(metrics);
	}
};

using UniquePangoFontMetrics = std::unique_ptr<PangoFontMetrics, FontMetricsReleaser>;

struct LayoutIterReleaser {
	// Called by unique_ptr to destroy/free the object
	void operator()(PangoLayoutIter *iter) noexcept {
		pango_layout_iter_free(iter);
	}
};

using UniquePangoLayoutIter = std::unique_ptr<PangoLayoutIter, LayoutIterReleaser>;

// Cairo

struct CairoReleaser {
	void operator()(cairo_t *context) noexcept {
		cairo_destroy(context);
	}
};

using UniqueCairo = std::unique_ptr<cairo_t, CairoReleaser>;

struct CairoSurfaceReleaser {
	void operator()(cairo_surface_t *psurf) noexcept {
		cairo_surface_destroy(psurf);
	}
};

using UniqueCairoSurface = std::unique_ptr<cairo_surface_t, CairoSurfaceReleaser>;

// GTK

using UniqueIMContext = std::unique_ptr<GtkIMContext, GObjectReleaser>;

// GDK

struct GdkEventReleaser {
	void operator()(GdkEvent *ev) noexcept {
		gdk_event_free(ev);
	}
};

using UniqueGdkEvent = std::unique_ptr<GdkEvent, GdkEventReleaser>;

inline void UnRefCursor(GdkCursor *cursor) noexcept {
#if GTK_CHECK_VERSION(3,0,0)
	g_object_unref(cursor);
#else
	gdk_cursor_unref(cursor);
#endif
}

[[nodiscard]] inline GdkWindow *WindowFromWidget(GtkWidget *w) noexcept {
	return gtk_widget_get_window(w);
}

}

#endif
