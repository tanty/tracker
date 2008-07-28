#include <glib.h>
#include <glib/gtestutils.h>
#include <string.h>

#include <libtracker-common/tracker-config.h>
#include <libtracker-common/tracker-language.h>
#include <libtracker-common/tracker-parser.h>

/* 
 * len(word) > 3 : 6 words  
 * longest word: 10 chars
 */
#define SAMPLE_TEXT "Here a good collection of various words to parse 12345678"

TrackerConfig *config;
TrackerLanguage *language;

const gchar *text = "";

void
print_key (gpointer key, gpointer value, gpointer user_data)
{
        g_print ("word: %s\n", (gchar *)key);
}

void
assert_key_length (gpointer key, gpointer value, gpointer user_data)
{
        gint max_length = GPOINTER_TO_INT (user_data);

        g_assert_cmpint (strlen (key), <=, max_length);
}

/* 
 * Test max_words_to_index and min_length of the word
 */
static void 
test_parser_text_max_words_to_index (void) 
{
        GHashTable *result = NULL;

        result = tracker_parser_text (result,
                                      SAMPLE_TEXT,
                                      1,
                                      language,
                                      5, /* max words to index */
                                      18, /* max length of the word */
                                      3, /* min length of the word */
                                      FALSE, FALSE); /* Filter / Delimit */
        
        g_assert_cmpint (g_hash_table_size (result), ==, 5);

        tracker_parser_text_free (result);
}

/*
 * Test max length of the word.
 */
static void
test_parser_text_max_length (void)
{
        GHashTable *result = NULL;
        gint max_length;

        max_length = 6;
        result = tracker_parser_text (result,
                                      SAMPLE_TEXT,
                                      1,
                                      language,
                                      10, /* max words to index */
                                      max_length, /* max length of the word */
                                      3, /* min length of the word */
                                      FALSE, FALSE); /* Filter / Delimit */
        g_hash_table_foreach (result, assert_key_length, GINT_TO_POINTER (max_length));
        g_assert_cmpint (g_hash_table_size (result), ==, 7);

        tracker_parser_text_free (result);        
}

/*
 * Filter numbers 
 */
static void
test_parser_text_filter_numbers (void)
{
        GHashTable *result = NULL;

        /* Filtering numbers */
        result = tracker_parser_text (result,
                                      SAMPLE_TEXT,
                                      1,
                                      language,
                                      100, /* max words to index */
                                      100, /* max length of the word */
                                      1, /* min length of the word */
                                      TRUE, FALSE); /* Filter / Delimit */

        g_assert (!g_hash_table_lookup (result, "12345678"));

        g_assert_cmpint (g_hash_table_size (result), ==, 9);

        tracker_parser_text_free (result);        
        result = NULL;

        /* No filter */
        result = tracker_parser_text (result,
                                      SAMPLE_TEXT,
                                      1,
                                      language,
                                      100, /* max words to index */
                                      100, /* max length of the word */
                                      1, /* min length of the word */
                                      FALSE, FALSE); /* Filter / Delimit */

        g_assert_cmpint (g_hash_table_size (result), ==, 10);

        g_assert (g_hash_table_lookup (result, "12345678"));

        tracker_parser_text_free (result);        
        result = NULL;
}

static void
test_parser_stop_words (void)
{
        GHashTable *stop_words;
        
        /* Check we have the default stop words */
        stop_words = tracker_language_get_stop_words (language);
        g_assert (stop_words);
        g_assert_cmpint (g_hash_table_size (stop_words), >, 1);

        /* Set specific stop words to test */
        tracker_config_set_language (config, "en");
        g_assert (g_hash_table_lookup (stop_words, "after"));

}

int
main (int argc, char **argv) {

        int result;

	g_type_init ();
        g_thread_init (NULL);
	g_test_init (&argc, &argv, NULL);

        /* Init */
        config = tracker_config_new ();
        language = tracker_language_new (config);

        g_test_add_func ("/libtracker-common/tracker-parser/parser_text/max_words_to_index",
                         test_parser_text_max_words_to_index);

        g_test_add_func ("/libtracker-common/tracker-parser/parser_text/max_length",
                         test_parser_text_max_length);

        g_test_add_func ("/libtracker-common/tracker-parser/parser_text/filter_numbers",
                         test_parser_text_filter_numbers);

        g_test_add_func ("/libtracker-common/tracker-parser/stop_words",
                         test_parser_stop_words);

        result = g_test_run ();
        
        /* End */
        g_object_unref (config);
        g_object_unref (language);

        return result;
}
