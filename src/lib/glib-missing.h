/*
 * $Id$
 *
 * Copyright (c) 2003, Raphael Manfredi
 *
 * Functions that should be in glib-1.2 but are not.
 * They are all prefixed with "gm_" as in "Glib Missing".
 *
 * We also include FIXED versions of glib-1.2 routines that are broken
 * and make sure those glib versions are never called directly.
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

#ifndef _glib_missing_h_
#define _glib_missing_h_

#include <sys/types.h>
#include <glib.h>

/*
 * Public interface.
 */

GSList *gm_slist_insert_after(GSList *list, GSList *lnk, gpointer data);

GList *gm_list_insert_after(GList *list, GList *lnk, gpointer data);

#ifndef USE_GLIB2
GList *g_list_delete_link(GList *l, GList *lnk);
#endif

size_t gm_vsnprintf(gchar *str, size_t n, gchar const *fmt, va_list args);
size_t gm_snprintf(gchar *str, size_t n,
	gchar const *fmt, ...) G_GNUC_PRINTF (3, 4);

void gm_savemain(gint argc, gchar **argv, gchar **env);
void gm_setproctitle(gchar *title);
unsigned long gm_atoul(const char *str, char **endptr, int *errorcode);
gchar *gm_sanitize_filename(const gchar *filename,
	gboolean no_spaces, gboolean no_evil);

#define GM_STRCONCAT_NULL(x, ...) g_strconcat(x, ## __VA_ARGS__, NULL)

/*
 * The G_*LIST_FOREACH_* macros are supposed to be used with ``func'' being
 * a function declared ``static inline'' whereas the protoype MUST match
 * ``GFunc''. ``func'' is not assigned to a variable so that the compiler
 * can prevent any function call overhead along with ``inline''.
 * These macros were rejected by the GLib maintainers so we can safely use
 * the G_ prefix.
 */

/* NB: Sub-statement func is evaluated more than once! */
#define G_LIST_FOREACH(list, func, user_data) \
	do { \
		GList *l_ = (list); \
		gpointer user_data_ = (user_data); \
		while (NULL != l_) { \
			func(l_->data, user_data_); \
			l_ = g_list_next(l_); \
		} \
	} while(0)

#define G_LIST_FOREACH_SWAPPED(list, func, user_data) \
	do { \
		GList *l_ = (list); \
		gpointer user_data_ = (user_data); \
		while (NULL != l_) { \
			func(user_data_, l_->data); \
			l_ = g_list_next(l_); \
		} \
	} while(0)

/* NB: Sub-statement func is evaluated more than once! */
#define G_SLIST_FOREACH(slist, func, user_data) \
	do { \
		GSList *sl_ = (slist); \
		gpointer user_data_ = (user_data); \
		while (NULL != sl_) { \
			func(sl_->data, user_data_); \
			sl_ = g_slist_next(sl_); \
		} \
	} while(0)

/* NB: Sub-statement func is evaluated more than once! */
#define G_SLIST_FOREACH_SWAPPED(slist, func, user_data) \
	do { \
		GSList *sl_ = (slist); \
		gpointer user_data_ = (user_data); \
		while (NULL != sl_) { \
			func(user_data_, sl_->data); \
			sl_ = g_slist_next(sl_); \
		} \
	} while(0)

/* vi: set ts=4 sw=4: */
#endif	/* _glib_missing_h_ */
