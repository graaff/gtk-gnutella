/*
 * $Id$
 *
 * Copyright (c) 2001-2002, Richard Eckart
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

#ifndef __search_gui_h__
#define __search_gui_h__

#include <gtk/gtk.h>

#include "gui.h"
#include "gnet.h"

extern GtkWidget *default_search_clist;

void search_gui_init(void);
void search_gui_remove_search(search_t * sch);
void search_gui_view_search(search_t *sch);

/*
 * Callbacks
 */
gboolean search_gui_search_results_col_widths_changed(property_t prop);
gboolean search_gui_search_results_col_visible_changed(property_t prop);

#endif /* __search_gui_h__ */
