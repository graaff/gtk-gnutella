/*
 * $Id$
 *
 * Copyright (c) 2003, Raphael Manfredi
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
 * @ingroup lib
 * @file
 *
 * Missing functions in the Glib 1.2.
 *
 * Functions that should be in glib-1.2 but are not.
 * They are all prefixed with "gm_" as in "Glib Missing".
 *
 * We also include FIXED versions of glib-1.2 routines that are broken
 * and make sure those glib versions are never called directly.
 *
 * @author Raphael Manfredi
 * @date 2003
 */

#include "common.h"

RCSID("$Id$");

#include "glib-missing.h"
#include "utf8.h"
#include "misc.h"
#include "override.h"		/* Must be the last header included */

#ifndef TRACK_MALLOC

/**
 * gm_slist_insert_after
 *
 * Insert `item' after `lnk' in list `list'.
 * If `lnk' is NULL, insertion happens at the head.
 *
 * @return new list head.
 */
GSList *gm_slist_insert_after(GSList *list, GSList *lnk, gpointer data)
{
	GSList *new;

	g_assert(list != NULL || lnk == NULL);	/* (list = NULL) => (lnk = NULL) */

	if (lnk == NULL)
		return g_slist_prepend(list, data);

	new = g_slist_alloc();
	new->data = data;

	new->next = lnk->next;
	lnk->next = new;

	return list;
}

/**
 * gm_list_insert_after
 *
 * Insert `item' after `lnk' in list `list'.
 * If `lnk' is NULL, insertion happens at the head.
 *
 * @return new list head.
 */
GList *gm_list_insert_after(GList *list, GList *lnk, gpointer data)
{
	GList *new;

	g_assert(list != NULL || lnk == NULL);	/* (list = NULL) => (lnk = NULL) */

	if (lnk == NULL)
		return g_list_prepend(list, data);

	new = g_list_alloc();
	new->data = data;

	new->prev = lnk;
	new->next = lnk->next;

	if (lnk->next)
		lnk->next->prev = new;

	lnk->next = new;

	return list;
}

#ifdef USE_GLIB1
GList *
g_list_delete_link(GList *l, GList *lnk)
{
	GList *new;

	new = g_list_remove_link(l, lnk);
	g_list_free_1(lnk);
	return new;
}
#endif /* USE_GLIB1 */

#endif /* !TRACK_MALLOC */

/**
 * @def DO_VSNPRINTF
 *
 * Perform the vsnprintf() operation for the gm_vsnprintf() and gm_snprintf()
 * routines.
 *
 * We don't use macro arguments on purpose: instead, we hardwire the following
 * that must be provided by the context, to ensure this macro is not reused
 * out of its original intended context.
 *
 * `retval' is the returned value.
 * `str' is the string where printing is done.
 * `n' is the maximum amount of chars that can be held in `str'.
 * `fmt' is the format string.
 * `args' is the arguments to be printed.
 */

#ifdef	HAS_VSNPRINTF
#define DO_VSNPRINTF() do {					\
	str[0] = '\0';							\
	retval = vsnprintf(str, n, fmt, args);	\
	if (retval < 0) {						\
		/* Old versions of vsnprintf() */ 	\
		str[n - 1] = '\0';					\
		retval = strlen(str);				\
	} else if ((size_t) retval >= n) {		\
	/* New versions (compliant with C99) */ \
		str[n - 1] = '\0';					\
		retval = n - 1;						\
	}										\
} while (0)
#else	/* !HAVE_VSNPRINTF */
#define DO_VSNPRINTF() do {							\
	gchar *printed = g_strdup_vprintf(fmt, args);	\
	size_t l = g_strlcpy(str, printed, n);			\
	retval = MIN((n - 1), l);						\
	G_FREE_NULL(printed);							\
} while (0)
#endif	/* HAVE_VSNPRINTF */


/**
 * gm_vsnprintf
 *
 * This version implements the correct FIXED semantics of the 1.2.10 glib:
 */
size_t gm_vsnprintf(gchar *str, size_t n, gchar const *fmt, va_list args)
{
	ssize_t retval;

	g_return_val_if_fail (str != NULL, 0);
	g_return_val_if_fail (fmt != NULL, 0);
	g_return_val_if_fail ((ssize_t) n > 0, 0);
	g_return_val_if_fail (n <= (size_t) INT_MAX, 0);

	DO_VSNPRINTF();

	g_assert((size_t) retval < n);

	return retval;
}

/**
 * gm_snprintf
 *
 * This version implements the correct FIXED semantics of the 1.2.10 glib:
 * It returns the length of the output string, and it is GUARANTEED to
 * be one less than `n' (last position occupied by the trailing NUL).
 */
size_t gm_snprintf(gchar *str, size_t n, gchar const *fmt, ...)
{
	va_list args;
	ssize_t retval;

	g_return_val_if_fail (str != NULL, 0);
	g_return_val_if_fail (fmt != NULL, 0);
	g_return_val_if_fail ((ssize_t) n > 0, 0);
	g_return_val_if_fail (n <= (size_t) INT_MAX, 0);

	va_start (args, fmt);
	DO_VSNPRINTF();
	va_end (args);

	g_assert((size_t) retval < n);

	return retval;
}

static gint orig_argc;
static gchar **orig_argv;
static gchar **orig_env;

/**
 * gm_savemain
 *
 * Save the original main() arguments.
 */
void gm_savemain(gint argc, gchar **argv, gchar **env)
{
	orig_argc = argc;
	orig_argv = argv;
	orig_env = env;
}

/**
 * gm_setproctitle
 *
 * Change the process title as seen by "ps".
 */
void gm_setproctitle(gchar *title)
{
	static gint sysarglen = 0;		/* Length of the exec() arguments */
	gint tlen;
	gint i;

	/*
	 * Compute the length of the exec() arguments that were given to us.
	 */

	if (sysarglen == 0) {
		gchar *s = orig_argv[0];

		s += strlen(s) + 1;			/* Go past trailing NUL */

		/*
		 * Let's see whether all the argv[] arguments were contiguous.
		 */

		for (i = 1; i < orig_argc; i++) {
			if (orig_argv[i] != s)
				break;
			s += strlen(s) + 1;		/* Yes, still contiguous */
		}

		/*
		 * Maybe the environment is contiguous as well...
		 */

		for (i = 0; orig_env[i] != NULL; i++) {
			if (orig_env[i] != s)
				break;
			s += strlen(s) + 1;		/* Yes, still contiguous */
		}

		sysarglen = s - orig_argv[0] - 1;	/* -1: leave room for NUL */

#if 0
		g_message("exec() args used %d contiguous bytes", sysarglen + 1);
#endif
	}

	tlen = strlen(title);

	if (tlen >= sysarglen) {		/* If too large, needs truncation */
		memcpy(orig_argv[0], title, sysarglen);
		(orig_argv[0])[sysarglen] = '\0';
	} else {
		memcpy(orig_argv[0], title, tlen + 1);	/* Copy trailing NUL */
		if (tlen + 1 < sysarglen)
			memset(orig_argv[0] + tlen + 1, ' ', sysarglen - tlen - 1);
	}

	/*
	 * Scrap references to the arguments.
	 */

	for (i = 1; i < orig_argc; i++)
		orig_argv[i] = NULL;
}

/**
 * gm_atoul
 *
 * @returns the nul-terminated string `str' converted to an unsigned long.
 * If successful `errorcode' will be set to 0 (zero), otherwise it will
 * contain an errno(2) code and the function returns 0 (zero).
 * If endptr is not NULL it will point to the first invalid character.
 * See strtoul(3) for more details about valid and invalid inputs.
 */
unsigned long gm_atoul(const char *str, char **endptr, int *errorcode)
{
	char *ep;
	unsigned long ret;
	int old_errno = errno;

	g_assert(NULL != str);
	g_assert(NULL != errorcode);

	errno = 0;
	ret = strtoul(str, &ep, 10);
	if (str == ep) {
		*errorcode = EINVAL;
		ret = 0;
	} else {
		if (0 != errno) {
			*errorcode = ERANGE;
			ret = 0;
		} else
			*errorcode = 0;
	}

	if (NULL != endptr)
		*endptr = ep;
	errno = old_errno;
	return ret;
}

#ifdef USE_GLIB1
/**
 * Appends len bytes of val to string. Because len is provided, val may
 * contain embedded nuls and need not be nul-terminated. 
 */
GString *g_string_append_len(GString *gs, const gchar *val,  gssize len)
{
	const gchar *p = val;

	while (len--)
		g_string_append_c(gs, *p++);

	return gs;
}
#endif	/* USE_GLIB1 */

/**
 * Creates a valid and sanitized filename from the supplied string. This is
 * necessary for platforms which have certain requirements for filenames. For
 * most Unix-like platforms anything goes but for security reasons, shell
 * meta characters are replaced by harmless characters.
 *
 * @param filename the suggested filename.
 * @param no_spaces if TRUE, spaces are replaced with underscores.
 * @param no_evil if TRUE, "evil" characters are replaced with underscores.
 *
 * @returns a newly allocated string or ``filename'' if it was a valid filename
 *		    already.
 */
gchar *
gm_sanitize_filename(const gchar *filename,
		gboolean no_spaces, gboolean no_evil)
{
	static const uni_norm_t norm = 
#if defined(__APPLE__) && defined(__MACH__) /* Darwin */
		UNI_NORM_NFD;
#else /* !Darwin */
		UNI_NORM_NFC;
#endif /* Darwin */
	gint c;
	gchar *q = NULL;
	const gchar *p, *s = filename;

	g_assert(filename != NULL);

	q = locale_to_utf8_normalized(filename, norm);
	s = q;

/** Maximum bytes in filename i.e., including NUL */
#define	FILENAME_MAXBYTES 256

	/* Make sure the filename isn't too long */
	if (strlen(s) >= FILENAME_MAXBYTES) {
		gchar *buf, *ext;
		size_t ext_size = 0;

		buf = g_malloc(FILENAME_MAXBYTES);

		/* Try to preserve the filename extension */
		ext = strrchr(s, '.');
		if (ext) {
			ext_size = strlen(ext) + 1;	/* Include NUL */
			if (ext_size >= FILENAME_MAXBYTES) {
				/*
				 * If it's too long, assume it's not extension at all.
				 * We must truncate the "extension" anyway and also
				 * preserve the UTF-8 encoding by all means.
				 */
				ext_size = 0;
				ext = NULL;
			}
		}

		g_assert(ext_size < FILENAME_MAXBYTES);
		utf8_strlcpy(buf, s, FILENAME_MAXBYTES - ext_size);

		/* Append the filename extension */
		if (ext) {
			size_t len;

			len = strlen(buf);
			g_assert(len + ext_size <= FILENAME_MAXBYTES);
			memcpy(&buf[len], ext, ext_size);
			buf[len + ext_size - 1] = '\0';
		}

		g_assert(strlen(buf) < FILENAME_MAXBYTES);
		G_FREE_NULL(q);
		s = q = buf;
	}

	/* Replace shell meta characters and likely problematic characters */
	for (p = s; (c = *(guchar *) p) != '\0'; ++p) {
		static const gchar evil[] = "$&*/\\`:;()'\"<>?|~\177";

		if (
			c < 32
			|| is_ascii_cntrl(c)
			|| c == G_DIR_SEPARATOR
			|| (c == ' ' && no_spaces)
			|| (p == s && c == '.')
			|| (no_evil && NULL != strchr(evil, c))
		) {
			if (!q) {
				q = g_strdup(s);
			}
			q[p - s] = '_';
		}
	}

	return q ? q : deconstify_gchar(s);
}

/* vi: set ts=4 sw=4 cindent: */
