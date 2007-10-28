/*
 * $Id$
 *
 * Copyright (c) 2007, Raphael Manfredi
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

#ifndef _g2_cache_h_
#define _g2_cache_h_

#include "common.h"

void g2_cache_init(void);
void g2_cache_insert(const host_addr_t addr, guint16 port);
gboolean g2_cache_lookup(const host_addr_t addr, guint16 port);
time_t g2_cache_get_timestamp(const host_addr_t addr, guint16 port);
void g2_cache_close(void);

#endif /* _g2_cache_h_ */

/* vi: set ts=4 sw=4 cindent: */