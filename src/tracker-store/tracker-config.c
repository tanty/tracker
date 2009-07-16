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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include <gio/gio.h>

#include <libtracker-common/tracker-file-utils.h>
#include <libtracker-common/tracker-type-utils.h>

#include "tracker-config.h"

#define TRACKER_CONFIG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TRACKER_TYPE_CONFIG, TrackerConfigPrivate))

/* GKeyFile defines */
#define GROUP_GENERAL				 "General"
#define KEY_VERBOSITY				 "Verbosity"
#define KEY_LOW_MEMORY_MODE			 "LowMemoryMode"

#define GROUP_INDEXING				 "Indexing"
#define KEY_MIN_WORD_LENGTH			 "MinWordLength"
#define KEY_MAX_WORD_LENGTH			 "MaxWordLength"

#define GROUP_PERFORMANCE			 "Performance"
#define KEY_MAX_WORDS_TO_INDEX			 "MaxWordsToIndex"

/* Default values */
#define DEFAULT_VERBOSITY			 0
#define DEFAULT_LOW_MEMORY_MODE			 FALSE
#define DEFAULT_MIN_WORD_LENGTH			 3	  /* 0->30 */
#define DEFAULT_MAX_WORD_LENGTH			 30	  /* 0->200 */
#define DEFAULT_MAX_WORDS_TO_INDEX		 10000

typedef struct _TrackerConfigPrivate TrackerConfigPrivate;

struct _TrackerConfigPrivate {
	GFile	     *file;
	GFileMonitor *monitor;

	GKeyFile     *key_file;

	/* General */
	gint	      verbosity;
	gboolean      low_memory_mode;

	/* Indexing */
	gint	      min_word_length;
	gint	      max_word_length;

	/* Performance */
	gint	      max_words_to_index;
};

static void     config_finalize             (GObject       *object);
static void     config_get_property         (GObject       *object,
					     guint          param_id,
					     GValue        *value,
					     GParamSpec    *pspec);
static void     config_set_property         (GObject       *object,
					     guint          param_id,
					     const GValue  *value,
					     GParamSpec    *pspec);
static void     config_load                 (TrackerConfig *config);
static gboolean config_save                 (TrackerConfig *config);
static void     config_create_with_defaults (GKeyFile      *key_file,
					     gboolean       overwrite);

enum {
	PROP_0,

	/* General */
	PROP_VERBOSITY,
	PROP_LOW_MEMORY_MODE,

	/* Indexing */
	PROP_MIN_WORD_LENGTH,
	PROP_MAX_WORD_LENGTH,

	/* Performance */
	PROP_MAX_WORDS_TO_INDEX,
};

G_DEFINE_TYPE (TrackerConfig, tracker_config, G_TYPE_OBJECT);

static void
tracker_config_class_init (TrackerConfigClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize	   = config_finalize;
	object_class->get_property = config_get_property;
	object_class->set_property = config_set_property;

	/* General */
	g_object_class_install_property (object_class,
					 PROP_VERBOSITY,
					 g_param_spec_int ("verbosity",
							   "Log verbosity",
							   "How much logging we have "
							   "(0=errors, 1=minimal, 2=detailed, 3=debug)",
							   0,
							   3,
							   DEFAULT_VERBOSITY,
							   G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
					 PROP_LOW_MEMORY_MODE,
					 g_param_spec_boolean ("low-memory-mode",
							       "Use extra memory",
							       "Use extra memory at the "
							       "expense of indexing speed",
							       DEFAULT_LOW_MEMORY_MODE,
							       G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	/* Indexing */
	g_object_class_install_property (object_class,
					 PROP_MIN_WORD_LENGTH,
					 g_param_spec_int ("min-word-length",
							   "Minimum word length",
							   "Minimum word length used to index "
							   "(0->30)",
							   0,
							   30,
							   DEFAULT_MIN_WORD_LENGTH,
							   G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
					 PROP_MAX_WORD_LENGTH,
					 g_param_spec_int ("max-word-length",
							   "Maximum word length",
							   "Maximum word length used to index",
							   0,
							   200, /* Is this a reasonable limit? */
							   DEFAULT_MAX_WORD_LENGTH,
							   G_PARAM_READWRITE | G_PARAM_CONSTRUCT));


	/* Performance */
	g_object_class_install_property (object_class,
					 PROP_MAX_WORDS_TO_INDEX,
					 g_param_spec_int ("max-words-to-index",
							   "Maximum words to index",
							   "Maximum unique words to index "
							   "from file's content",
							   0,
							   G_MAXINT,
							   DEFAULT_MAX_WORDS_TO_INDEX,
							   G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

	g_type_class_add_private (object_class, sizeof (TrackerConfigPrivate));
}

static void
tracker_config_init (TrackerConfig *object)
{
	TrackerConfigPrivate *priv;

	priv = TRACKER_CONFIG_GET_PRIVATE (object);

	priv->key_file = g_key_file_new ();
}

static void
config_finalize (GObject *object)
{
	TrackerConfigPrivate *priv;

	priv = TRACKER_CONFIG_GET_PRIVATE (object);

	if (priv->key_file) {
		g_key_file_free (priv->key_file);
	}

	if (priv->monitor) {
		g_object_unref (priv->monitor);
	}

	if (priv->file) {
		g_object_unref (priv->file);
	}

	(G_OBJECT_CLASS (tracker_config_parent_class)->finalize) (object);
}

static void
config_get_property (GObject	*object,
		     guint	 param_id,
		     GValue	*value,
		     GParamSpec *pspec)
{
	TrackerConfigPrivate *priv;

	priv = TRACKER_CONFIG_GET_PRIVATE (object);

	switch (param_id) {
		/* General */
	case PROP_VERBOSITY:
		g_value_set_int (value, priv->verbosity);
		break;
	case PROP_LOW_MEMORY_MODE:
		g_value_set_boolean (value, priv->low_memory_mode);
		break;

		/* Indexing */
	case PROP_MIN_WORD_LENGTH:
		g_value_set_int (value, priv->min_word_length);
		break;
	case PROP_MAX_WORD_LENGTH:
		g_value_set_int (value, priv->max_word_length);
		break;

		/* Performance */
	case PROP_MAX_WORDS_TO_INDEX:
		g_value_set_int (value, priv->max_words_to_index);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

static void
config_set_property (GObject	  *object,
		     guint	   param_id,
		     const GValue *value,
		     GParamSpec	  *pspec)
{
	switch (param_id) {
		/* General */
	case PROP_VERBOSITY:
		tracker_config_set_verbosity (TRACKER_CONFIG (object),
					      g_value_get_int (value));
		break;
	case PROP_LOW_MEMORY_MODE:
		tracker_config_set_low_memory_mode (TRACKER_CONFIG (object),
						    g_value_get_boolean (value));
		break;

		/* Indexing */
	case PROP_MIN_WORD_LENGTH:
		tracker_config_set_min_word_length (TRACKER_CONFIG (object),
						    g_value_get_int (value));
		break;
	case PROP_MAX_WORD_LENGTH:
		tracker_config_set_max_word_length (TRACKER_CONFIG (object),
						    g_value_get_int (value));
		break;

		/* Performance */
	case PROP_MAX_WORDS_TO_INDEX:
		tracker_config_set_max_words_to_index (TRACKER_CONFIG (object),
						       g_value_get_int (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

static gchar *
config_dir_ensure_exists_and_return (void)
{
	gchar *directory;

	directory = g_build_filename (g_get_user_config_dir (),
				      "tracker",
				      NULL);

	if (!g_file_test (directory, G_FILE_TEST_EXISTS)) {
		g_print ("Creating config directory:'%s'\n", directory);

		if (g_mkdir_with_parents (directory, 0700) == -1) {
			g_warning ("Could not create configuration directory");
			g_free (directory);
			return NULL;
		}
	}

	return directory;
}

static void
config_create_with_defaults (GKeyFile *key_file, 
			     gboolean  overwrite)
{
	g_message ("Loading defaults into GKeyFile...");

	/* General */
	if (overwrite || !g_key_file_has_key (key_file, GROUP_GENERAL, KEY_VERBOSITY, NULL)) {
		g_key_file_set_integer (key_file, GROUP_GENERAL, KEY_VERBOSITY, 
					DEFAULT_VERBOSITY);
		g_key_file_set_comment (key_file, GROUP_GENERAL, KEY_VERBOSITY,
					" Log Verbosity (0=errors, 1=minimal, 2=detailed, 3=debug)",
					NULL);
	}

	if (overwrite || !g_key_file_has_key (key_file, GROUP_GENERAL, KEY_LOW_MEMORY_MODE, NULL)) {
		g_key_file_set_boolean (key_file, GROUP_GENERAL, KEY_LOW_MEMORY_MODE, 
					DEFAULT_LOW_MEMORY_MODE);
		g_key_file_set_comment (key_file, GROUP_GENERAL, KEY_LOW_MEMORY_MODE,
					" Minimizes memory use at the expense of indexing speed",
					NULL);
	}

	/* Indexing */
	if (overwrite || !g_key_file_has_key (key_file, GROUP_INDEXING, KEY_MIN_WORD_LENGTH, NULL)) {
		g_key_file_set_integer (key_file, GROUP_INDEXING, KEY_MIN_WORD_LENGTH,
					DEFAULT_MIN_WORD_LENGTH);
		g_key_file_set_comment (key_file, GROUP_INDEXING, KEY_MIN_WORD_LENGTH,
					" Set the minimum length of words to index (0->30, default=3)",
					NULL);
	}

	if (overwrite || !g_key_file_has_key (key_file, GROUP_INDEXING, KEY_MAX_WORD_LENGTH, NULL)) {
		g_key_file_set_integer (key_file, GROUP_INDEXING, KEY_MAX_WORD_LENGTH,
					DEFAULT_MAX_WORD_LENGTH);
		g_key_file_set_comment (key_file, GROUP_INDEXING, KEY_MAX_WORD_LENGTH,
					" Set the maximum length of words to index (0->200, default=30)",
					NULL);
	}

	/* Performance */
	if (overwrite || !g_key_file_has_key (key_file, GROUP_PERFORMANCE, KEY_MAX_WORDS_TO_INDEX, NULL)) {
		g_key_file_set_integer (key_file, GROUP_PERFORMANCE, KEY_MAX_WORDS_TO_INDEX,
					DEFAULT_MAX_WORDS_TO_INDEX);
		g_key_file_set_comment (key_file, GROUP_PERFORMANCE, KEY_MAX_WORDS_TO_INDEX,
					" Maximum unique words to index from a file's content",
					NULL);
	}
}

static gboolean
config_save_with_defaults (const gchar *filename,
			   GKeyFile    *key_file)
{
	GError *error = NULL;
	gchar  *content = NULL;

	/* Save to file */
	content = g_key_file_to_data (key_file, NULL, &error);

	if (error) {
		g_warning ("Couldn't produce default configuration, %s", error->message);
		g_clear_error (&error);
		return FALSE;
	}

	if (!g_file_set_contents (filename, content, -1, &error)) {
		g_warning ("Couldn't write default configuration, %s", error->message);
		g_clear_error (&error);
		g_free (content);
		return FALSE;
	}

	g_print ("Writing default configuration to file:'%s'\n", filename);
	g_free (content);

	return TRUE;
}

static void
config_load_int (TrackerConfig *config,
		 const gchar   *property,
		 GKeyFile      *key_file,
		 const gchar   *group,
		 const gchar   *key)
{
	GError *error = NULL;
	gint	value;

	value = g_key_file_get_integer (key_file, group, key, &error);
	if (!error) {
		g_object_set (G_OBJECT (config), property, value, NULL);
	} else {
		g_message ("Couldn't load config option '%s' (int) in group '%s', %s",
			   property, group, error->message);
		g_error_free (error);
	}
}

static void
config_load_boolean (TrackerConfig *config,
		     const gchar   *property,
		     GKeyFile	   *key_file,
		     const gchar   *group,
		     const gchar   *key)
{
	GError	 *error = NULL;
	gboolean  value;

	value = g_key_file_get_boolean (key_file, group, key, &error);
	if (!error) {
		g_object_set (G_OBJECT (config), property, value, NULL);
	} else {
		g_message ("Couldn't load config option '%s' (bool) in group '%s', %s",
			   property, group, error->message);
		g_error_free (error);
	}
}

#if 0

static void
config_load_string (TrackerConfig *config,
		    const gchar	  *property,
		    GKeyFile	  *key_file,
		    const gchar	  *group,
		    const gchar	  *key)
{
	GError *error = NULL;
	gchar  *value;

	value = g_key_file_get_string (key_file, group, key, &error);
	if (!error) {
		g_object_set (G_OBJECT (config), property, value, NULL);
	} else {
		g_message ("Couldn't load config option '%s' (string) in group '%s', %s",
			   property, group, error->message);
		g_error_free (error);
	}

	g_free (value);
}

#endif


static void
config_save_int (TrackerConfig *config,
		 const gchar   *property,
		 GKeyFile      *key_file,
		 const gchar   *group,
		 const gchar   *key)
{
	gint value;

	g_object_get (G_OBJECT (config), property, &value, NULL);
	g_key_file_set_integer (key_file, group, key, value);
}

static void
config_save_boolean (TrackerConfig *config,
		     const gchar   *property,
		     GKeyFile	   *key_file,
		     const gchar   *group,
		     const gchar   *key)
{
	gboolean value;

	g_object_get (G_OBJECT (config), property, &value, NULL);
	g_key_file_set_boolean (key_file, group, key, value);
}

#if 0

static void
config_save_string (TrackerConfig *config,
		    const gchar	  *property,
		    GKeyFile	  *key_file,
		    const gchar	  *group,
		    const gchar	  *key)
{
	gchar *value;

	g_object_get (G_OBJECT (config), property, &value, NULL);
	g_key_file_set_string (key_file, group, key, value);
	g_free (value);
}

#endif 


static void
config_changed_cb (GFileMonitor     *monitor,
		   GFile	    *file,
		   GFile	    *other_file,
		   GFileMonitorEvent event_type,
		   gpointer	     user_data)
{
	TrackerConfig *config;
	gchar	      *filename;

	config = TRACKER_CONFIG (user_data);

	/* Do we recreate if the file is deleted? */

	switch (event_type) {
	case G_FILE_MONITOR_EVENT_CHANGED:
	case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
		filename = g_file_get_path (file);
		g_message ("Config file changed:'%s', reloading settings...",
			   filename);
		g_free (filename);

		config_load (config);
		break;

	case G_FILE_MONITOR_EVENT_DELETED:
	case G_FILE_MONITOR_EVENT_CREATED:
	case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
	case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
	case G_FILE_MONITOR_EVENT_UNMOUNTED:
	default:
		break;
	}
}

static void
config_load (TrackerConfig *config)
{
	TrackerConfigPrivate *priv;
	GError		     *error = NULL;
	gchar                *basename;
	gchar		     *filename;
	gchar		     *directory;

	/* Check we have a config file and if not, create it based on
	 * the default settings.
	 */
	directory = config_dir_ensure_exists_and_return ();
	if (!directory) {
		return;
	}

	basename = g_strdup_printf ("%s.cfg", g_get_application_name ());
	filename = g_build_filename (directory, basename, NULL);
	g_free (basename);
	g_free (directory);

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	/* Add file monitoring for changes */
	if (!priv->file) {
		priv->file = g_file_new_for_path (filename);
	}

	if (!priv->monitor) {
		g_message ("Setting up monitor for changes to config file:'%s'",
			   filename);

		priv->monitor = g_file_monitor_file (priv->file,
						     G_FILE_MONITOR_NONE,
						     NULL,
						     NULL);

		g_signal_connect (priv->monitor, "changed",
				  G_CALLBACK (config_changed_cb),
				  config);
	}

	/* Load options */
	g_key_file_load_from_file (priv->key_file, 
				   filename, 
				   G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
				   &error);

	config_create_with_defaults (priv->key_file, FALSE);

	if (error) {
		config_save_with_defaults (filename, priv->key_file);
		g_clear_error (&error);
	}

	g_free (filename);

	/* General */
	config_load_int (config, "verbosity", priv->key_file, GROUP_GENERAL, KEY_VERBOSITY);
	config_load_boolean (config, "low-memory-mode", priv->key_file, GROUP_GENERAL, KEY_LOW_MEMORY_MODE);

	/* Indexing */
	config_load_int (config, "min-word-length", priv->key_file, GROUP_INDEXING, KEY_MIN_WORD_LENGTH);
	config_load_int (config, "max-word-length", priv->key_file, GROUP_INDEXING, KEY_MAX_WORD_LENGTH);

	/* Performance */
	config_load_int (config, "max-words-to-index", priv->key_file, GROUP_PERFORMANCE, KEY_MAX_WORDS_TO_INDEX);
}

static gboolean
config_save (TrackerConfig *config)
{
	TrackerConfigPrivate *priv;
	GError		     *error = NULL;
	gchar		     *filename;
	gchar		     *data;
	gsize                 size;

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	if (!priv->key_file) {
		g_critical ("Could not save config, GKeyFile was NULL, has the config been loaded?");

		return FALSE;
	}

	g_message ("Setting details to GKeyFile object...");

	/* Set properties to GKeyFile */
	config_save_int (config, "verbosity", priv->key_file, GROUP_GENERAL, KEY_VERBOSITY);
	config_save_boolean (config, "low-memory-mode", priv->key_file, GROUP_GENERAL, KEY_LOW_MEMORY_MODE);

	/* Indexing */
	config_save_int (config, "min-word-length", priv->key_file, GROUP_INDEXING, KEY_MIN_WORD_LENGTH);
	config_save_int (config, "max-word-length", priv->key_file, GROUP_INDEXING, KEY_MAX_WORD_LENGTH);

	/* Performance */
	config_save_int (config, "max-words-to-index", priv->key_file, GROUP_PERFORMANCE, KEY_MAX_WORDS_TO_INDEX);

	g_message ("Saving config to disk...");

	/* Do the actual saving to disk now */
	data = g_key_file_to_data (priv->key_file, &size, &error);
	if (error) {
		g_warning ("Could not get config data to write to file, %s",
			   error->message);
		g_error_free (error);

		return FALSE;
	}

	filename = g_file_get_path (priv->file);

	g_file_set_contents (filename, data, size, &error);
	g_free (data);

	if (error) {
		g_warning ("Could not write %" G_GSIZE_FORMAT " bytes to file '%s', %s",
			   size,
			   filename,
			   error->message);
		g_free (filename);
		g_error_free (error);

		return FALSE;
	}

	g_message ("Wrote config to '%s' (%" G_GSIZE_FORMAT " bytes)",
		   filename, 
		   size);

	g_free (filename);

	return TRUE;
}

static gboolean
config_int_validate (TrackerConfig *config,
		     const gchar   *property,
		     gint	    value)
{
#ifdef G_DISABLE_CHECKS
	GParamSpec *spec;
	GValue	    value = { 0 };
	gboolean    valid;

	spec = g_object_class_find_property (G_OBJECT_CLASS (config), property);
	g_return_val_if_fail (spec != NULL, FALSE);

	g_value_init (&value, spec->value_type);
	g_value_set_int (&value, verbosity);
	valid = g_param_value_validate (spec, &value);
	g_value_unset (&value);

	g_return_val_if_fail (valid != TRUE, FALSE);
#endif

	return TRUE;
}

/**
 * tracker_config_new:
 *
 * Creates a new GObject for handling Tracker's config file.
 *
 * Return value: A new TrackerConfig object. Must be unreferenced when
 * finished with.
 */
TrackerConfig *
tracker_config_new (void)
{
	TrackerConfig *config;

	config = g_object_new (TRACKER_TYPE_CONFIG, NULL);
	config_load (config);

	return config;
}

/**
 * tracker_config_save:
 * @config: a #TrackerConfig
 *
 * Writes the configuration stored in TrackerConfig to disk.
 *
 * Return value: %TRUE on success, %FALSE otherwise.
 */
gboolean
tracker_config_save (TrackerConfig *config)
{
	g_return_val_if_fail (TRACKER_IS_CONFIG (config), FALSE);

	return config_save (config);
}

/**
 * tracker_config_get_verbosity:
 * @config: a #TrackerConfig
 *
 * Gets the verbosity of the logging in the indexer and the daemon.
 *
 * If the verbosity is 0, there is no logging except for warnings and
 * errors.
 * If the verbosity is 1, information is displayed.
 * If the verbosity is 2, general messages are displayed.
 * If the verbosity is 3, debug messages are displayed.
 *
 * Note, you receive logging for anything less priority than the
 * verbosity level as well as the level you set. So if the verbosity
 * is 3 you receive debug, messages, info and warnings.
 *
 * Return value: An integer value from 0 to 3.
 */
gint
tracker_config_get_verbosity (TrackerConfig *config)
{
	TrackerConfigPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_CONFIG (config), DEFAULT_VERBOSITY);

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	return priv->verbosity;
}

gboolean
tracker_config_get_low_memory_mode (TrackerConfig *config)
{
	TrackerConfigPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_CONFIG (config), DEFAULT_LOW_MEMORY_MODE);

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	return priv->low_memory_mode;
}


gint
tracker_config_get_min_word_length (TrackerConfig *config)
{
	TrackerConfigPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_CONFIG (config), DEFAULT_MIN_WORD_LENGTH);

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	return priv->min_word_length;
}

gint
tracker_config_get_max_word_length (TrackerConfig *config)
{
	TrackerConfigPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_CONFIG (config), DEFAULT_MAX_WORD_LENGTH);

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	return priv->max_word_length;
}

gint
tracker_config_get_max_words_to_index (TrackerConfig *config)
{
	TrackerConfigPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_CONFIG (config), DEFAULT_MAX_WORDS_TO_INDEX);

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	return priv->max_words_to_index;
}

void
tracker_config_set_verbosity (TrackerConfig *config,
			      gint	     value)
{
	TrackerConfigPrivate *priv;

	g_return_if_fail (TRACKER_IS_CONFIG (config));

	if (!config_int_validate (config, "verbosity", value)) {
		return;
	}

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	priv->verbosity = value;
	g_object_notify (G_OBJECT (config), "verbosity");
}

void
tracker_config_set_low_memory_mode (TrackerConfig *config,
				    gboolean	   value)
{
	TrackerConfigPrivate *priv;

	g_return_if_fail (TRACKER_IS_CONFIG (config));

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	priv->low_memory_mode = value;
	g_object_notify (G_OBJECT (config), "low-memory-mode");
}

void
tracker_config_set_min_word_length (TrackerConfig *config,
				    gint	   value)
{
	TrackerConfigPrivate *priv;

	g_return_if_fail (TRACKER_IS_CONFIG (config));

	if (!config_int_validate (config, "min-word-length", value)) {
		return;
	}

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	priv->min_word_length = value;
	g_object_notify (G_OBJECT (config), "min-word-length");
}

void
tracker_config_set_max_word_length (TrackerConfig *config,
				    gint	   value)
{
	TrackerConfigPrivate *priv;

	g_return_if_fail (TRACKER_IS_CONFIG (config));

	if (!config_int_validate (config, "max-word-length", value)) {
		return;
	}

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	priv->max_word_length = value;
	g_object_notify (G_OBJECT (config), "max-word-length");
}

void
tracker_config_set_max_words_to_index (TrackerConfig *config,
				       gint	      value)
{
	TrackerConfigPrivate *priv;

	g_return_if_fail (TRACKER_IS_CONFIG (config));

	if (!config_int_validate (config, "max-words-to-index", value)) {
		return;
	}

	priv = TRACKER_CONFIG_GET_PRIVATE (config);

	priv->max_words_to_index = value;
	g_object_notify (G_OBJECT (config), "max-words-to-index");
}
