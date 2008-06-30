/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006, Mr Jamie McCracken (jamiemcc@gnome.org)
 * Copyright (C) 2007, Michal Pryc (Michal.Pryc@Sun.Com)
 * Copyright (C) 2008, Nokia
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */
#include "config.h"

#include <sys/statvfs.h>

#include <libtracker-common/tracker-log.h>
#include <libtracker-common/tracker-config.h>

#include "tracker-utils.h"
#include "tracker-main.h"
#include "tracker-xesam-manager.h"

extern Tracker *tracker;

gchar *
tracker_get_radix_by_suffix (const gchar *str, 
			     const gchar *suffix)
{
	g_return_val_if_fail (str, NULL);
	g_return_val_if_fail (suffix, NULL);

	if (g_str_has_suffix (str, suffix)) {
		return g_strndup (str, g_strrstr (str, suffix) - str);
	} else {
		return NULL;
	}
}

void
tracker_throttle (gint multiplier)
{
	gint throttle;

	throttle = tracker_config_get_throttle (tracker->config);

	if (throttle < 1) {
		return;
	}

 	throttle *= multiplier;

	if (throttle > 0) {
  		g_usleep (throttle);
	}
}

void
tracker_add_metadata_to_table (GHashTable  *meta_table, 
			       const gchar *key, 
			       const gchar *value)
{
	GSList *list;

	list = g_hash_table_lookup (meta_table, (gchar*) key);
	list = g_slist_prepend (list, (gchar*) value);
	g_hash_table_steal (meta_table, key);
	g_hash_table_insert (meta_table, (gchar*) key, list);
}

gboolean
tracker_is_low_diskspace (void)
{
	struct statvfs  st;
	const gchar    *data_dir;
        gint            low_disk_space_limit;

        low_disk_space_limit = tracker_config_get_low_disk_space_limit (tracker->config);

	if (low_disk_space_limit < 1) {
		return FALSE;
	}

	data_dir = tracker_get_data_dir ();

	if (statvfs (data_dir, &st) == -1) {
		static gboolean reported = 0;

		if (!reported) {
			reported = 1;
			g_critical ("Could not statvfs %s", data_dir);
		}

		return FALSE;
	}

	if (((long long) st.f_bavail * 100 / st.f_blocks) <= low_disk_space_limit) {
		g_critical ("Disk space is low!");
		return TRUE;
	}

	return FALSE;
}

gboolean
tracker_should_pause (void)
{
	return  tracker->pause_manual || 
		tracker_should_pause_on_battery () || 
		tracker_is_low_diskspace () || 
		tracker_indexer_are_databases_too_big ();
}

gboolean
tracker_should_pause_on_battery (void)
{
        if (!tracker->pause_battery) {
                return FALSE;
        }

	if (tracker->first_time_index) {
		return tracker_config_get_disable_indexing_on_battery_init (tracker->config);
	}

        return tracker_config_get_disable_indexing_on_battery (tracker->config);
}

