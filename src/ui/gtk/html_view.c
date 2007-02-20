/*
 * $Id$
 *
 * Copyright (c) 2007 Christian Biere
 *
 *----------------------------------------------------------------------
 * This file is part of gtk-gnutella.
 *
 *  gtk-gnutella is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  gtk-gnutella is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gtk-gnutella; if not, write to the Free Software
 *  Foundation, Inc.:
 *      59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *----------------------------------------------------------------------
 */

/**
 * @ingroup gtk
 * @file
 *
 * Simple HTML view.
 *
 * @author Christian Biere
 * @date 2007
 */

#include "gui.h"
#include <gdk/gdkkeysyms.h>

RCSID("$Id$")

#include "lib/glib-missing.h"
#include "lib/html.h"
#include "lib/misc.h"
#include "lib/utf8.h"
#include "lib/walloc.h"

#define STYLE_TAG(x) static const gchar STYLE_TAG_ ## x [] = #x ;
STYLE_TAG(ANCHOR)
STYLE_TAG(ANCHOR_EXTERN)
STYLE_TAG(BOLD)
STYLE_TAG(CENTER)
STYLE_TAG(HEADING)
STYLE_TAG(ITALIC)
STYLE_TAG(MONOSPACE)
STYLE_TAG(UNDERLINE)
STYLE_TAG(WORD_WRAP)
#undef STYLE_TAG

struct html_view {
#if GTK_CHECK_VERSION(2,0,0)
	GtkTextView *widget;
#else
	GtkText *widget;
#endif
	GSList *to_free;
};

struct html_context {
	struct html_view *html_view;
	struct html_output *output;
	GString *title;
	gchar *href;

#if GTK_CHECK_VERSION(2,0,0)
	GtkTextMark *start[NUM_HTML_TAG];
#endif	/* Gtk+ >= 2.0 */
};

static struct html_context *
html_context_alloc(void)
{
	static const struct html_context zero_html_context;
	struct html_context *ctx;

	ctx = walloc(sizeof *ctx);
	*ctx = zero_html_context;
	return ctx;
}

static void
html_context_free(struct html_context **ctx_ptr)
{
	struct html_context *ctx = *ctx_ptr;
	if (ctx) {
		if (ctx->title) {
			g_string_free(ctx->title, TRUE);
			ctx->title = NULL;
		}
		html_output_free(&ctx->output);
		*ctx_ptr = NULL;
	}
}
	
static struct html_view *
html_view_alloc(void)
{
	static const struct html_view zero_html_view;
	struct html_view *ctx;

	ctx = walloc(sizeof *ctx);
	*ctx = zero_html_view;
	return ctx;
}

static void
html_output_print(struct html_output *output, const struct array *text)
{
	struct html_context *ctx;
   
	if (text->size <= 0)
		return;

	ctx = html_output_get_udata(output);
	if (ctx->title) {
		g_string_append_len(ctx->title, text->data, text->size);
		return;
	}
#if GTK_CHECK_VERSION(2,0,0)
	{
		GtkTextBuffer *buffer;
		GtkTextIter iter;

		buffer = gtk_text_view_get_buffer(ctx->html_view->widget);
		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert(buffer, &iter, text->data, text->size);
	}
#else
	{
		struct array str;
		gchar *to_free;

		if (locale_is_utf8()) {
			str = array_init(text->data, text->size);
			to_free = NULL;
		} else {
			to_free = g_strndup(text->data, text->size);
			str = array_from_string(lazy_utf8_to_ui_string(to_free));
		}
		gtk_text_insert(ctx->html_view->widget,
			NULL, NULL, NULL, str.data, str.size);
		G_FREE_NULL(to_free);
	}
#endif

}

static const gchar *
get_style_for_href(const gchar *href)
{
	unsigned char c;

	g_return_val_if_fail(href, NULL);
	do {
		c = *href++;
	} while (is_ascii_alnum(c) || '-' == c);
	return ':' == c ? STYLE_TAG_ANCHOR_EXTERN : STYLE_TAG_ANCHOR;
}

static void
html_output_tag(struct html_output *output, const struct array *tag)
{
	static gboolean initialized;
	static gchar centre_line[5];
	static gchar list_item_prefix[7];
	struct html_context *ctx;
	const gchar *style, *text, *attr;
	enum html_tag id;
	gboolean closing;
#if GTK_CHECK_VERSION(2,0,0)
	GtkTextBuffer *buffer;
#endif

	if (!initialized) {
		static gchar bullet[5];

		initialized = TRUE;
		utf8_encode_char(locale_is_utf8() ? 0x2022 : 0x002D,
			bullet, sizeof bullet);
		utf8_encode_char(0xFE4E, centre_line, sizeof centre_line);
		concat_strings(list_item_prefix, sizeof list_item_prefix,
			" ", bullet, " ", (void *) 0);
	}
	
	style = NULL;
	text = NULL;
	attr = NULL;
	closing = html_tag_is_closing(tag);
	ctx = html_output_get_udata(output);
	id = html_parse_tag(tag);
 
#if GTK_CHECK_VERSION(2,0,0)
	buffer = gtk_text_view_get_buffer(ctx->html_view->widget);
#endif

	switch (id) {
	case HTML_TAG_BODY:
		style = STYLE_TAG_WORD_WRAP;
		break;
	case HTML_TAG_A:
		if (closing) {
#if GTK_CHECK_VERSION(2,0,0)
			if (ctx->start[id] && ctx->href) {
				GtkTextIter start, end;
				GtkTextTag *anchor;

				anchor = gtk_text_buffer_create_tag(buffer, NULL, (void *) 0);
				g_object_set_data(G_OBJECT(anchor), "href", ctx->href);
				gtk_text_buffer_get_iter_at_mark(buffer,
					&start, ctx->start[id]);
				gtk_text_buffer_get_end_iter(buffer, &end);
				gtk_text_buffer_apply_tag(buffer, anchor, &start, &end);
				style = get_style_for_href(ctx->href);
				ctx->href = NULL;
			}
#else
#endif
		} else {
			struct array value;

			value = html_get_attribute(tag, HTML_ATTR_HREF);
			if (value.data && value.size > 0) {
#if GTK_CHECK_VERSION(2,0,0)
				GtkTextIter iter;

				ctx->href = g_strndup(value.data, value.size);
				ctx->html_view->to_free = g_slist_prepend(
											ctx->html_view->to_free, ctx->href);
				gtk_text_buffer_get_end_iter(buffer, &iter);
				ctx->start[id] = gtk_text_buffer_create_mark(buffer,
										NULL, &iter, TRUE);
				g_object_set_data(G_OBJECT(ctx->start[id]), "href", ctx->href);
#else
#endif
			}
			value = html_get_attribute(tag, HTML_ATTR_NAME);
			if (value.data && value.size > 0) {
#if GTK_CHECK_VERSION(2,0,0)
				GtkTextTagTable *table;
				gchar name[256];
				size_t n;

				n = sizeof name - 2;
				n = MIN(value.size, n);
				name[0] = '#';
				memcpy(&name[1], value.data, n);
				name[n + 1] = '\0';
				
				table = gtk_text_buffer_get_tag_table(buffer);
				if (NULL == gtk_text_tag_table_lookup(table, name)) {
					GtkTextIter iter;

					gtk_text_buffer_get_end_iter(buffer, &iter);
					gtk_text_buffer_create_mark(buffer, name, &iter, TRUE);
				}
#else
#endif
			}
		}
		break;
	case HTML_TAG_B:
	case HTML_TAG_STRONG:
	case HTML_TAG_THEAD:
		style = STYLE_TAG_BOLD;
		break;
	case HTML_TAG_TH:
		if (closing)
			text = "\t";
		break;
	case HTML_TAG_EM:
		style = STYLE_TAG_UNDERLINE;
		break;
	case HTML_TAG_I:
	case HTML_TAG_Q:
		style = STYLE_TAG_ITALIC;
		break;
	case HTML_TAG_IMG:
		if (!closing) {
			struct array value;
			static gchar alt[1024];
			
			value = html_get_attribute(tag, HTML_ATTR_ALT);
			if (value.data) {
				gm_snprintf(alt, sizeof alt, "\n[image alt=\"%.*s\"]\n",
					(int)value.size, value.data);
				text = alt;
			}
			value = html_get_attribute(tag, HTML_ATTR_SRC);
			if (value.data) {
#if GTK_CHECK_VERSION(2,0,0)
				GdkPixbuf *pixbuf;
				gchar *filename;
				GtkTextIter iter;

				filename = g_strndup(value.data, value.size);
				pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
				if (pixbuf) {
					gtk_text_buffer_get_end_iter(buffer, &iter);
					gtk_text_buffer_insert_pixbuf(buffer, &iter, pixbuf);
				} else {
					static gchar msg[1024];
					gm_snprintf(msg, sizeof msg,
						"\n[Image not found (\"%s\")]\n", filename);
					text = msg;
				}
				G_FREE_NULL(filename);
#else
#endif
			}
			if (!text) {
				text = "\n[image]\n";
			}
		}
		attr = STYLE_TAG_BOLD;
		break;
	case HTML_TAG_TD:
		if (closing)
			text = "\t";
		break;
	case HTML_TAG_P:
	case HTML_TAG_DIV:
		text = closing ? "\n\n" : "\n";
		break;
	case HTML_TAG_DL:
	case HTML_TAG_TABLE:
	case HTML_TAG_TR:
	case HTML_TAG_UL:
	case HTML_TAG_OL:
	case HTML_TAG_BR:
		text = "\n";
		break;
	case HTML_TAG_DT:
	case HTML_TAG_LI:
		if (closing) {
			text = "\n";
		} else {
			text = list_item_prefix;
			attr = STYLE_TAG_BOLD;
		}
		break;
	case HTML_TAG_CODE:
	case HTML_TAG_KBD:
	case HTML_TAG_PRE:
	case HTML_TAG_TT:
		style = STYLE_TAG_MONOSPACE;
		break;
	case HTML_TAG_H1:
	case HTML_TAG_H2:
	case HTML_TAG_H3:
	case HTML_TAG_H4:
	case HTML_TAG_H5:
	case HTML_TAG_H6:
		style = STYLE_TAG_HEADING;
		text = closing ? "\n\n" : "\n";
		break;
	case HTML_TAG_TITLE:
		if (closing) {
			if (ctx->title) {
				GtkWidget *window;

				window = gtk_widget_get_toplevel(
							GTK_WIDGET(ctx->html_view->widget));
				gtk_window_set_title(GTK_WINDOW(window), ctx->title->str);
				g_string_free(ctx->title, TRUE);
				ctx->title = NULL;
			}
		} else {
			ctx->title = g_string_new("");
		}
		break;
	case HTML_TAG_HR:
#if 0 && GTK_CHECK_VERSION(2,0,0)
		{
			GtkTextIter iter;

			gtk_text_buffer_get_end_iter(buffer, &iter);
			ctx->start[id] = gtk_text_buffer_create_mark(buffer, NULL,
								&iter, TRUE);
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
					centre_line, (-1), STYLE_TAG_CENTER, (void *) 0);
		}
		style = STYLE_TAG_HEADING;
		closing = TRUE;
		text = "\n";
#else
#endif
		break;
	case HTML_TAG_COMMENT:
#if 0 
		{
			GtkTextIter iter;

			/* Comments can be made visible this way */
			ctx->start[id] = gtk_text_buffer_create_mark(buffer, NULL,
					&iter, TRUE);
			gtk_text_buffer_get_end_iter(buffer, &iter);
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
					tag->data, tag->size, STYLE_TAG_ITALIC, (void *) 0);
		}
		closing = TRUE;
		text = "\n";
#endif
		break;
	case HTML_TAG_HTML:
	case HTML_TAG_HEAD:
	case HTML_TAG_META:
	case HTML_TAG_SPAN:
	case HTML_TAG_COL:
	case HTML_TAG_DD:
	case HTML_TAG_TBODY:
	case HTML_TAG_DOCTYPE:
	case HTML_TAG_UNKNOWN:
		break;
	case NUM_HTML_TAG:
		g_assert_not_reached();
	}

	if (style) {
		if (closing) {
#if GTK_CHECK_VERSION(2,0,0)
			if (ctx->start[id]) {
				GtkTextIter start, end;
		
				gtk_text_buffer_get_iter_at_mark(buffer,
						&start, ctx->start[id]);
				gtk_text_buffer_get_end_iter(buffer, &end);
				gtk_text_buffer_apply_tag_by_name(buffer, style, &start, &end);
				ctx->start[id] = NULL;
			}
#else
#endif
		} else {
#if GTK_CHECK_VERSION(2,0,0)
			GtkTextIter iter;
			gtk_text_buffer_get_end_iter(buffer, &iter);
			ctx->start[id] = gtk_text_buffer_create_mark(buffer,
										NULL, &iter, TRUE);
#else
#endif
		}
	}
	if (text) {
#if GTK_CHECK_VERSION(2,0,0)
		GtkTextIter iter;

		gtk_text_buffer_get_end_iter(buffer, &iter);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter,
			text, (-1), attr, (void *) 0);
#else
		gtk_text_insert(ctx->html_view->widget, NULL, NULL, NULL, text, (-1));
#endif
	}
}

#if GTK_CHECK_VERSION(2,0,0)
static GdkCursor *hand_cursor, *regular_cursor;

static void
set_cursor_if_appropriate(GtkTextView *text_view, gint x, gint y)
{
	GSList *tags, *sl;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	gboolean hovering = FALSE;

	buffer = gtk_text_view_get_buffer(text_view);
	gtk_text_view_get_iter_at_location(text_view, &iter, x, y);
	tags = gtk_text_iter_get_tags(&iter);

	for (sl = tags; NULL != sl; sl = g_slist_next(sl)) {
		if (g_object_get_data(G_OBJECT(sl->data), "href")) {
			hovering = TRUE;
			break;
		}
	}
	g_slist_free(tags);

	gdk_window_set_cursor(
		gtk_text_view_get_window(text_view, GTK_TEXT_WINDOW_TEXT),
		hovering ? hand_cursor : regular_cursor);
}

static gboolean
visibility_notify_event(GtkWidget *text_view, GdkEventVisibility *event)
{
	gint wx, wy, bx, by;

	(void) event;
	gdk_window_get_pointer(text_view->window, &wx, &wy, NULL);
	gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), 
		GTK_TEXT_WINDOW_WIDGET, wx, wy, &bx, &by);
	set_cursor_if_appropriate(GTK_TEXT_VIEW(text_view), bx, by);
	return FALSE;
}


static gboolean
motion_notify_event(GtkWidget *text_view, GdkEventMotion *event)
{
	gint x, y;

	gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), 
		GTK_TEXT_WINDOW_WIDGET, event->x, event->y, &x, &y);
	set_cursor_if_appropriate(GTK_TEXT_VIEW(text_view), x, y);
	gdk_window_get_pointer(text_view->window, NULL, NULL, NULL);
	return FALSE;
}

/* Looks at all tags covering the position of iter in the text view, 
 * and if one of them is a link, follow it by showing the page identified
 * by the data attached to it.
 */
static void
follow_if_link(GtkWidget *text_view, GtkTextIter *iter)
{
	GSList *tags, *sl;

	tags = gtk_text_iter_get_tags(iter);
	for (sl = tags; NULL != sl; sl = g_slist_next(sl)) {
		const gchar *href;

		href = g_object_get_data(G_OBJECT(sl->data), "href");
		if (href) {
			GtkTextBuffer *buffer;
			GtkTextMark *target;

			buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
			target = gtk_text_buffer_get_mark(buffer, href);
			if (target) {
				gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view),
					target, 0.0, TRUE, 0.0, 0.0);
			}
			break;
		}
	}
	g_slist_free(tags);
}

/* Links can be activated by pressing Enter.
 */
static gboolean
key_press_event(GtkWidget *text_view, GdkEventKey *event)
{
	GtkTextIter iter;
	GtkTextBuffer *buffer;

	switch (event->keyval) {
	case GDK_Return: 
	case GDK_KP_Enter:
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
		gtk_text_buffer_get_iter_at_mark(buffer, &iter,
				gtk_text_buffer_get_insert(buffer));
		follow_if_link(text_view, &iter);
		break;
	}
	return FALSE;
}

/* Links can also be activated by clicking.
 */
static gboolean
event_after(GtkWidget *text_view, GdkEvent *ev)
{
	GtkTextIter start, end, iter;
	GtkTextBuffer *buffer;
	GdkEventButton *event;
	gint x, y;

	if (ev->type != GDK_BUTTON_RELEASE)
		return FALSE;

	event = cast_to_gpointer(ev);
	if (event->button != 1)
		return FALSE;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

	/* we shouldn't follow a link if the user has selected something */
	gtk_text_buffer_get_selection_bounds(buffer, &start, &end);
	if (gtk_text_iter_get_offset(&start) == gtk_text_iter_get_offset(&end)) {
		gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), 
			GTK_TEXT_WINDOW_WIDGET, event->x, event->y, &x, &y);
		gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(text_view),
			&iter, x, y);
		follow_if_link(text_view, &iter);
	}
  	return FALSE;
}
#endif	/* Gtk+ >= 2.0 */


static struct html_context *
html_view_load(struct html_view *html_view)
{
	struct html_context *ctx;
	GtkWidget *widget;

	g_return_val_if_fail(html_view, NULL);
	g_return_val_if_fail(html_view->widget, NULL);

	widget = GTK_WIDGET(html_view->widget);
	ctx = html_context_alloc();

#if GTK_CHECK_VERSION(2,0,0)
	if (!hand_cursor) {
		hand_cursor = gdk_cursor_new(GDK_HAND2);
	}
	if (!regular_cursor) {
		regular_cursor = gdk_cursor_new(GDK_XTERM);
	}

	ctx->html_view = html_view;
	gtk_widget_show_all(gtk_widget_get_toplevel(widget));

	g_signal_connect(widget, "motion-notify-event", 
		G_CALLBACK(motion_notify_event), NULL);
	g_signal_connect(widget, "visibility-notify-event", 
		G_CALLBACK(visibility_notify_event), NULL);
	g_signal_connect(widget, "key-press-event", 
		G_CALLBACK(key_press_event), NULL);
	g_signal_connect(widget, "event-after", 
		G_CALLBACK(event_after), NULL);
		  
 	gtk_text_view_set_buffer(GTK_TEXT_VIEW(widget), NULL);
	{
		GtkTextBuffer *buffer;
		
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));

		gtk_text_buffer_create_tag(buffer, STYLE_TAG_WORD_WRAP,
				"wrap_mode",		GTK_WRAP_WORD,
				(void *) 0);
		gtk_text_buffer_create_tag(buffer, STYLE_TAG_MONOSPACE,
				"family",			"monospace",
				NULL);
		gtk_text_buffer_create_tag(buffer,	STYLE_TAG_ANCHOR,
				"foreground",		"blue", 
				"underline",		PANGO_UNDERLINE_SINGLE, 
				(void *) 0);
		gtk_text_buffer_create_tag(buffer,	STYLE_TAG_ANCHOR_EXTERN,
				"foreground",		"red", 
				"underline",		PANGO_UNDERLINE_SINGLE, 
				(void *) 0);
		gtk_text_buffer_create_tag(buffer, STYLE_TAG_BOLD,
				"weight",			PANGO_WEIGHT_BOLD,
				(void *) 0);
		gtk_text_buffer_create_tag(buffer, STYLE_TAG_ITALIC,
				"style",			PANGO_STYLE_ITALIC,
				(void *) 0);
		gtk_text_buffer_create_tag(buffer, STYLE_TAG_CENTER,
				"justification",	GTK_JUSTIFY_CENTER,
				(void *) 0);
		gtk_text_buffer_create_tag(buffer, STYLE_TAG_UNDERLINE,
				"underline",		PANGO_UNDERLINE_SINGLE,
				(void *) 0);
		gtk_text_buffer_create_tag(buffer, STYLE_TAG_HEADING,
				"weight",			PANGO_WEIGHT_BOLD,
				"size",				15 * PANGO_SCALE,
				(void *) 0);
	}
#else	/* Gtk+ < 2.0 */

	ctx->html_view = html_view;
	gtk_text_freeze(widget);
	gtk_text_set_word_wrap(widget, TRUE);
	
#endif	/* Gtk+ >= 2.0 */

	ctx->output = html_output_alloc();
	html_output_set_udata(ctx->output, ctx);
	html_output_set_print(ctx->output, html_output_print);
	html_output_set_tag(ctx->output, html_output_tag);
	
#if GTK_CHECK_VERSION(2,0,0)
#else
	gtk_text_thaw(widget);
#endif

	return ctx;
}

struct html_view *
html_view_load_file(GtkWidget *widget, int fd)
{
	struct html_view *html_view;
	struct html_context *ctx;

	g_return_val_if_fail(widget, NULL);
	g_return_val_if_fail(fd >= 0, NULL);

	html_view = html_view_alloc();
	html_view->widget = cast_to_gpointer(widget);

	ctx = html_view_load(html_view);
	if (ctx) {
		html_load_file(ctx->output, fd);
		html_context_free(&ctx);
	}
	return html_view;
}

struct html_view *
html_view_load_memory(GtkWidget *widget, const struct array memory)
{
	struct html_view *html_view;
	struct html_context *ctx;
	
	g_return_val_if_fail(widget, NULL);
	g_return_val_if_fail(memory.data, NULL);

	html_view = html_view_alloc();
	html_view->widget = cast_to_gpointer(widget);

	ctx = html_view_load(html_view);
	if (ctx) {
		html_load_memory(ctx->output, memory);
		html_context_free(&ctx);
	}
	return html_view;
}

void
html_view_clear(struct html_view *html_view)
{
	g_return_if_fail(html_view);
	g_return_if_fail(html_view->widget);

#if GTK_CHECK_VERSION(2,0,0)
	{
		GtkTextBuffer *buffer;
		GtkTextIter start, end;
		
		buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(html_view->widget));
		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_get_end_iter(buffer, &end);
		gtk_text_buffer_delete(buffer, &start, &end);
		gtk_text_buffer_remove_all_tags(buffer, &start, &end);
	}
#else
	{
		GtkText *view;

		view = GTK_TEXT(html_view->widget);
		gtk_text_freeze(view);
		gtk_text_set_point(view, 0);
		gtk_text_forward_delete(view, gtk_text_get_length(view));
		gtk_text_thaw(view);
	}
#endif
}

void
html_view_free(struct html_view **html_view_ptr)
{
	struct html_view *html_view = *html_view_ptr;

	if (html_view) {
		html_view_clear(html_view);
		if (html_view->to_free) {
			GSList *iter;

			for (iter = html_view->to_free; iter; iter = g_slist_next(iter)) {
				G_FREE_NULL(iter->data);
			}
			g_slist_free(html_view->to_free);
		}
		wfree(html_view, sizeof *html_view);
		*html_view_ptr = NULL;
	}
}

/* vi: set ts=4 sw=4 cindent: */
