/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
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

#include <gio/gio.h>

#include "tracker-monitor.h"

/* This is the default inotify limit - 500 to allow some monitors for
 * other applications. 
 *
 * FIXME: Should we try reading
 * /proc/sys/fs/inotify/max_user_watches when there is a possiblity
 * that we don't even use inotify?
 */
#define MAX_MONITORS (guint) ((2 ^ 13) - 500)   

static GHashTable    *monitors;
static TrackerConfig *config;

gboolean 
tracker_monitor_init (TrackerConfig *_config) 
{
	g_return_val_if_fail (TRACKER_IS_CONFIG (_config), FALSE);
	
	if (!config) {
		config = g_object_ref (_config);
	}
	
	if (!monitors) {
		monitors = g_hash_table_new_full (g_str_hash,
						  g_str_equal,
						  g_free,
						  (GDestroyNotify) g_file_monitor_cancel);
	}

	return TRUE;
}

void
tracker_monitor_shutdown (void)
{
	if (monitors) {
		g_hash_table_unref (monitors);
		monitors = NULL;
	}
	
	if (config) {
		g_object_unref (config);
		config = NULL;
	}
}

gboolean
tracker_monitor_add (const gchar *path)
{
	GFile        *file;
	GFileMonitor *monitor;
	GError       *error = NULL;

	g_return_val_if_fail (path != NULL, FALSE);

	if (g_hash_table_lookup (monitors, path)) {
		return TRUE;
	}

	/* Cap the number of monitors */
	if (g_hash_table_size (monitors) >= MAX_MONITORS) {
		g_warning ("The maximum number of monitors to set (%d) "
			   "has been reached, not adding any new ones",
			   MAX_MONITORS);
		return FALSE;
	}

	/* We don't check if a file exists or not since we might want
	 * to monitor locations which don't exist yet.
	 */
	file = g_file_new_for_path (path);
	monitor = g_file_monitor_directory (file,
					    G_FILE_MONITOR_WATCH_MOUNTS,
					    NULL,
					    &error);
	g_object_unref (file);

	if (error) {
		g_warning ("Could not add monitor for path:'%s', %s", 
			   path, 
			   error->message);
		g_error_free (error);
		return FALSE;
	}

	g_debug ("Added monitor for:'%s', total monitors:%d", 
		 path,
		 g_hash_table_size (monitors));

	g_hash_table_insert (monitors, 
			     g_strdup (path), 
			     monitor);
	
	return TRUE;
}

gboolean
tracker_monitor_remove (const gchar *path,
			gboolean     delete_subdirs)
{
	GFileMonitor *monitor;

	g_return_val_if_fail (path != NULL, FALSE);

	monitor = g_hash_table_lookup (monitors, path);
	if (!monitor) {
		return TRUE;
	}

	g_hash_table_remove (monitors, path);

	g_debug ("Removed monitor for:'%s', total monitors:%d", 
		 path,
		 g_hash_table_size (monitors));

	return TRUE;
}

gboolean
tracker_monitor_is_watched (const gchar *path)
{
	g_return_val_if_fail (path != NULL, FALSE);

	return g_hash_table_lookup (monitors, path) != NULL;
}

gint
tracker_monitor_get_count (void)
{
	return g_hash_table_size (monitors);
}

