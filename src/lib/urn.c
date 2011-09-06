/*
 * $Id$
 *
 * Copyright (c) 2002-2004, 2009, Raphael Manfredi
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
 * URN handling of specific formats.
 *
 * @author Raphael Manfredi
 * @date 2002-2004, 2009
 */

#include "common.h"

#include "base16.h"
#include "base32.h"
#include "misc.h"
#include "urn.h"
#include "override.h"		/* Must be the last header included */

/**
 * Validate SHA1 starting in NUL-terminated `buf' as a proper base32 encoding
 * of a SHA1 hash, and write decoded value in `retval'.
 *
 * The SHA1 typically comes from HTTP, in a X-Gnutella-Content-URN header.
 *
 * @return TRUE if the SHA1 was valid and properly decoded, FALSE on error.
 */
gboolean
parse_base32_sha1(const char *buf, size_t size, struct sha1 *sha1)
{
	struct sha1 raw;
	size_t len;

	if (!sha1) {
		sha1 = &raw;
	}

	if (size < SHA1_BASE32_SIZE)
		return FALSE;
		
	len = base32_decode(sha1->data, SHA1_RAW_SIZE, buf, SHA1_BASE32_SIZE);
	if (SHA1_RAW_SIZE != len)
		return FALSE;

	return TRUE;
}

/**
 * Validate SHA1 starting in NUL-terminated `buf' as a proper base16 encoding
 * of a SHA1 hash, and write decoded value in `retval'.
 *
 * The SHA1 typically comes from HTTP or magnet URIs, made by people who do
 * not follow the specs which says a magnet must hold a base32-encoded SHA1.
 *
 * @return TRUE if the SHA1 was valid and properly decoded, FALSE on error.
 */
gboolean
parse_base16_sha1(const char *buf, size_t size, struct sha1 *sha1)
{
	struct sha1 raw;
	size_t len;

	if (!sha1) {
		sha1 = &raw;
	}

	if (size < SHA1_BASE16_SIZE)
		return FALSE;
		
	len = base16_decode(sha1->data, SHA1_RAW_SIZE, buf, SHA1_BASE16_SIZE);
	if (SHA1_RAW_SIZE != len)
		return FALSE;

	return TRUE;
}

/**
 * Locate the start of "urn:sha1:" or "urn:bitprint:" indications and extract
 * the SHA1 out of it, placing it in the supplied `digest' buffer.
 *
 * @return whether we successfully extracted the SHA1.
 */
gboolean
urn_get_sha1(const char *buf, struct sha1 *sha1)
{
	const char *p;
	size_t len;

	/*
	 * We handle both "urn:sha1:" and "urn:bitprint:".  In the latter case,
	 * the first 32 bytes of the bitprint is the SHA1.
	 */

	if (
		NULL == (p = is_strcaseprefix(buf, "urn:sha1:")) &&
		NULL == (p = is_strcaseprefix(buf, "urn:bitprint:"))
	)
		return FALSE;

	/*
	 * Following the specifications, these should be base32-encoded.
	 */

	len = clamp_strlen(p, SHA1_BASE32_SIZE);
	if (parse_base32_sha1(p, len, sha1))
		return TRUE;

	/*
	 * Maybe it was generated by clueless people who use an hexadecimal
	 * representation?
	 */

	len = clamp_strlen(p, SHA1_BASE16_SIZE);
	return parse_base16_sha1(p, len, sha1);
}

/**
 * Locate the start of "urn:bitprint:" indications and extract
 * the SHA1 and TTH out of it, placing them in the supplied buffers.
 *
 * @return whether we successfully extracted the bitprint, i.e. the two
 * hashes.
 */
gboolean
urn_get_bitprint(const char *buf, size_t size,
	struct sha1 *sha1, struct tth *tth)
{
	static const char prefix[] = "urn:bitprint:";
	size_t len;
	const char *p;
	gboolean base16_tth = FALSE;

	g_assert(0 == size || NULL != buf);
	g_assert(sha1);
	g_assert(tth);

	/*
	 * Because some clueless sites list magnets with hexadecimal-encoded
	 * values, we attempt to parse both base32 and base16 encoded hashes.
	 *
	 * Note that we expect both hashes to be similarily encoded.
	 */

	if (size < CONST_STRLEN(prefix) + BITPRINT_BASE32_SIZE)
		return FALSE;
	p = is_strcaseprefix(buf, prefix);
	if (NULL == p)
		return FALSE;
	if (!parse_base32_sha1(p, SHA1_BASE32_SIZE, sha1)) {
		if (
			size >= CONST_STRLEN(prefix) + BITPRINT_BASE16_SIZE &&
			parse_base16_sha1(p, SHA1_BASE16_SIZE, sha1)
		) {
			p += SHA1_BASE16_SIZE;
			base16_tth = TRUE;		/* SHA1 was hexa, expects TTH as hexa */
		} else {
			return FALSE;
		}
	} else {
		p += SHA1_BASE32_SIZE;
	}
	if ('.' != *p++) {
		return FALSE;
	}
	if (base16_tth) {
		len = base16_decode(tth->data, TTH_RAW_SIZE, p, TTH_BASE16_SIZE);
	} else {
		len = base32_decode(tth->data, TTH_RAW_SIZE, p, TTH_BASE32_SIZE);
	}
	if (len != TTH_RAW_SIZE) {
		return FALSE;
	}
	return TRUE;
}

/**
 * Extract the TTH out of a "urn:tree:tiger" URN.
 * Note that the TTH is expected to be solely base32-encoded here.
 *
 * @return whether TTH was successfully extracted.
 */
gboolean
urn_get_tth(const char *buf, size_t size, struct tth *tth)
{
	static const char prefix[] = "urn:tree:tiger";
	size_t len;
	const char *p;

	g_assert(0 == size || NULL != buf);
	g_assert(tth);

	if (size < CONST_STRLEN(prefix) + 1 /* ":" */ + TTH_BASE32_SIZE) {
		return FALSE;
	}
	p = is_strcaseprefix(buf, prefix);
	if (NULL == p) {
		return FALSE;
	}
	if ('/' == *p) {
		/* RAZA puts a slash after "tiger" */
		if (size < CONST_STRLEN(prefix) + 2 /* "/:" */ + TTH_BASE32_SIZE){
			return FALSE;
		}
		p++;
	}
	if (':' != *p++) {
		return FALSE;
	}
	len = base32_decode(tth->data, TTH_RAW_SIZE, p, TTH_BASE32_SIZE);
	if (len != TTH_RAW_SIZE) {
		return FALSE;
	}
	return TRUE;
}

/**
 * This is the same as urn_get_sha1(), only the leading "urn:" part
 * is missing (typically a URN embedded in a GGEP "u").
 *
 * `buf' MUST start with "sha1:" or "bitprint:" indications.  Since the
 * leading "urn:" part is missing, we cannot be lenient.
 *
 * Extract the SHA1 out of it, placing it in the supplied `digest' buffer.
 *
 * @return whether we successfully extracted the SHA1.
 */
gboolean
urn_get_sha1_no_prefix(const char *buf, struct sha1 *sha1)
{
	const char *p;
	size_t len;

	/*
	 * We handle both "sha1:" and "bitprint:".  In the latter case,
	 * the first 32 bytes of the bitprint is the SHA1.
	 */

	if (
		NULL == (p = is_strcaseprefix(buf, "sha1:")) &&
		NULL == (p = is_strcaseprefix(buf, "bitprint:"))
	)
		return FALSE;

	len = clamp_strlen(p, SHA1_BASE32_SIZE);
	if (parse_base32_sha1(p, len, sha1))
		return TRUE;

	/*
	 * Maybe it was generated by clueless people who use an hexadecimal
	 * representation?
	 */

	len = clamp_strlen(p, SHA1_BASE16_SIZE);
	return parse_base16_sha1(p, len, sha1);
}

/*
 * Emacs stuff:
 * Local Variables: ***
 * c-indentation-style: "bsd" ***
 * fill-column: 80 ***
 * tab-width: 4 ***
 * indent-tabs-mode: nil ***
 * End: ***
 * vi: set ts=4 sw=4 cindent:
 */
