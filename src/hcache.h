/*
 * $Id$
 *
 * Copyright (c) 2002-2003, Raphael Manfredi, Richard Eckart
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
 * @file
 *
 * Host cache management.
 */

#ifndef _hcache_h_
#define _hcache_h_

#include "gnet.h"
/*
 * Global Functions
 */

void hcache_init(void);
void hcache_shutdown(void);
void hcache_close(void);
void hcache_retrieve_all(void);

void hcache_timer(void);

const gchar *host_type_to_gchar(hcache_type_t type);
const gchar *hcache_type_to_gchar(hcache_type_t type);

gboolean hcache_add(
    hcache_type_t type, guint32 ip, guint16 port,
    gchar *what);

gboolean hcache_add_caught(
    host_type_t type, guint32 ip, guint16 port,
    gchar *what);

gboolean hcache_add_valid(
    host_type_t type, guint32 ip, guint16 port,
    gchar *what);

gboolean hcache_node_is_bad(guint32 ip);

void hcache_clear(host_type_t type);
void hcache_prune(hcache_type_t type) ;

gint hcache_size(host_type_t type);
gboolean hcache_is_low(host_type_t type);

gint hcache_fill_caught_array(
	host_type_t type, gnet_host_t *hosts, gint hcount);

void hcache_get_caught(host_type_t type, guint32 *ip, guint16 *port);
gboolean hcache_find_nearby(host_type_t type, guint32 *ip, guint16 *port);

#endif /* _hcache_h_ */

