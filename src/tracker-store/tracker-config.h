/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008, Nokia (urho.konttori@nokia.com)
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

#ifndef __TRACKER_STORE_CONFIG_H__
#define __TRACKER_STORE_CONFIG_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define TRACKER_TYPE_CONFIG	    (tracker_config_get_type ())
#define TRACKER_CONFIG(o)	    (G_TYPE_CHECK_INSTANCE_CAST ((o), TRACKER_TYPE_CONFIG, TrackerConfig))
#define TRACKER_CONFIG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), TRACKER_TYPE_CONFIG, TrackerConfigClass))
#define TRACKER_IS_CONFIG(o)	    (G_TYPE_CHECK_INSTANCE_TYPE ((o), TRACKER_TYPE_CONFIG))
#define TRACKER_IS_CONFIG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TRACKER_TYPE_CONFIG))
#define TRACKER_CONFIG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TRACKER_TYPE_CONFIG, TrackerConfigClass))

typedef struct _TrackerConfig	   TrackerConfig;
typedef struct _TrackerConfigClass TrackerConfigClass;

struct _TrackerConfig {
	GObject      parent;
};

struct _TrackerConfigClass {
	GObjectClass parent_class;
};

GType	       tracker_config_get_type               (void) G_GNUC_CONST;

TrackerConfig *tracker_config_new                    (void);
gboolean       tracker_config_save                   (TrackerConfig *config);
gint           tracker_config_get_verbosity          (TrackerConfig *config);
gboolean       tracker_config_get_low_memory_mode    (TrackerConfig *config);
gint           tracker_config_get_min_word_length    (TrackerConfig *config);
gint           tracker_config_get_max_word_length    (TrackerConfig *config);
gint           tracker_config_get_max_words_to_index (TrackerConfig *config);

void           tracker_config_set_verbosity          (TrackerConfig *config,
						      gint           value);
void           tracker_config_set_low_memory_mode    (TrackerConfig *config,
						      gboolean       value);
void           tracker_config_set_min_word_length    (TrackerConfig *config,
						      gint           value);
void           tracker_config_set_max_word_length    (TrackerConfig *config,
						      gint           value);
void           tracker_config_set_max_words_to_index (TrackerConfig *config,
						      gint           value);

G_END_DECLS

#endif /* __TRACKER_STORE_CONFIG_H__ */

